//**************************************************************************
// author: Alexander Bochem
// date:   07.05.2010
//**************************************************************************
//input:	RLE encoded pixel data
//output:	blob attributes merged with bounding box criteria
//			 
//description: 
//  - read RLE encoded pixel data from input fifo 
//	- merge Runs based on bounding box criteria
//  - write Blobs attributes to output fifo
//**************************************************************************
//change:	13.05.2010
//			condition checks in state CHECK_PIXEL implemented to merge adjacent
//			runs into BLOB containers
//
//			15.05.2010
//			included BLOB_RANGE and CONT_RANGE for adjacency check
//			eliminates multiple detection of BLOBs
//
//			26.05.2010
//			extended to detect up to six BLOBs
//
//			27.05.2010
//			add average X and Y edge to output for
//			threshold adjustment
//
//			29.05.2010
//			include collecting data for center of mass method in RLE merging process
//			change module name to RLE_BlobMerging
//
//			31.05.2010
//			include computation for center of mass
//**************************************************************************

`define LSB_X 1'b0	
`define MSB_X 4'b1010 
`define LSB_Y 4'b1011
`define  MSB_Y 5'b10101
`define LSB_RunLength 5'b10110
`define MSB_RunLength 5'b11111 
`define LSB_RunSumValues 6'b100000
`define MSB_RunSumValues  6'b11111
`define LSB_RunSumXpositions 7'b1000000
`define MSB_RunSumXpositions 7'b1011111
`define LSB_RunSumYpositions  7'b1100000
`define MSB_RunSumYpositions 7'b1111111
`define LSB_EOFflag 1'b0
`define MSB_EOFflag 5'b11111
`define CONT_RANGE   5'b01010
`define BLOB_RANGE   5'b01010


`define FALSE 1'b0 
`define TRUE 1'b1
`define INIT 5'b00000
`define IDLE 5'b00001
`define CHECK_CONDITIONS 5'b00010
`define WRITE_FIFO 5'b00011
`define WAIT_FIFO 5'b00100
`define COMPUTE_CENTER 5'b00101
`define VALIDATE_CHECK_ADJACENT_CONTAINER 5'b00110
`define READ_FIFO 5'b00111
`define CONFIG_WRITE 5'b01000
`define  WRITE_WAIT 5'b01001
`define SELECT_EMPTY_CONTAINER 5'b01010
`define CHECK_ADJACENT_CONTAINTER 5'b01011
`define CHECK_CONT_ADJACENCY 5'b01100
`define MERGE_CONTAINER 5'b01101
`define WRITE_BLOB_0 4'b0000 
`define WRITE_BLOB_1 4'b0001 
`define WRITE_BLOB_2 4'b0010 
`define WRITE_BLOB_3 4'b0011 
`define WRITE_BLOB_4 4'b0100 
`define WRITE_BLOB_5 4'b0101
`define EOF 32'b00000000000000000000000000000000
`define MERGE_CONT_1_2 5'b01110
`define MERGE_CONT_1_3 5'b01111
`define MERGE_CONT_1_4 5'b10000
`define MERGE_CONT_1_5 5'b10001
`define MERGE_CONT_2_3 5'b10010
`define MERGE_CONT_2_4 5'b10011
`define MERGE_CONT_2_5 5'b10100
`define MERGE_CONT_3_4 5'b10101
`define MERGE_CONT_3_5 5'b10110
`define MERGE_CONT_4_5 5'b10111
`define MERGE_CONT_1_6 5'b11000
`define MERGE_CONT_2_6 5'b11001
`define MERGE_CONT_3_6 5'b11010
`define MERGE_CONT_4_6 5'b11011
`define MERGE_CONT_5_6 5'b11100



module RLE_BlobMerging(clk,
//iCOMclock,
    iReset,
    iReadFifoEmpty,
    iReadFifoData,
    iWriteFifoFull,		
    oReadFifoRequest,
    oWriteBlobData, 
    oWriteRequest,		
    oAvgSizeXaxis, 
    oAvgSizeYaxis
);

    input clk; 				//module clock
    //input iCOMclock;			//clock for COM sub-module
    input iReset;				//module reset signal
    input iReadFifoEmpty;		//fifo empty signal from input fifo
    input [127:0]iReadFifoData;	//data bus from input fifo [32b y-w., 32b x-w., 32 pixel-w.,10b length, 11b Y, 11b X]
    input iWriteFifoFull;		//fifo full signal from output fifo
    
    output  oReadFifoRequest;	//read request to input fifo
    output  [75:0]oWriteBlobData; //data bus to output fifo [10b index, 11b bb y, 11b bb x, 11b com y, 11b com x, 11b len y, 11b len x]
    output  oWriteRequest;		 //write request to output fifo
    output [10:0] oAvgSizeXaxis; //average size of X axis for detected BLOBs
    output [10:0] oAvgSizeYaxis;  //average size of Y axis for detected BLOBs
    	
    reg oReadFifoRequest;	//read request to input fifo
    reg [75:0]oWriteBlobData; //data bus to output fifo [10b index, 11b bb y, 11b bb x, 11b com y, 11b com x, 11b len y, 11b len x]
    reg oWriteRequest;		 //write request to output fifo
    reg [10:0] oAvgSizeXaxis; //average size of X axis for detected BLOBs
    reg [10:0] oAvgSizeYaxis;  //average size of Y axis for detected BLOBs

/*
parameter LSB_X=0, 	MSB_X=10; //start and end bit for x coordiante
parameter LSB_Y=11, MSB_Y=21; //start and end bit for y coordiante
parameter LSB_RunLength=22, MSB_RunLength=31; //start and end bit for RUN length
parameter LSB_RunSumValues=32, MSB_RunSumValues=63;
parameter LSB_RunSumXpositions=64, MSB_RunSumXpositions=95;
parameter LSB_RunSumYpositions=96, MSB_RunSumYpositions=127;
parameter LSB_EOFflag=0, MSB_EOFflag=31;//start and end bit for EOF flag
parameter CONT_RANGE = 5'd10;
parameter BLOB_RANGE = 5'd10;
parameter MIN_X=11'd0, MAX_X=11'd640, MIN_Y=11'd0, MAX_Y=11'd480;
parameter COM_DELAY_TIME=3'd5;
*/

//internal registers
reg [17:0] checkResult; //each flage represents one result of conditional checks
reg [9:0]  run_length;  //temporary store the length of a detected run
reg [10:0] run_start_x; //temporary store starting X position of run
reg [10:0] run_start_y; //temporary store starting Y position of run
reg [31:0] run_sum_x_positions; //temporary store weighted sum of x positions in run
reg [31:0] run_sum_y_positions; //temporary store weighted sum of y postions in run
reg [31:0] run_sum_values;      //temporary store sum of weights in run
reg [3:0] write_result_pointer;
reg [4:0] state;
reg RunAdded;
reg [14:0] ContainerAdjacentResult;
reg [3:0] countDetectedBlobs;
reg [10:0] avgSizeXaxis;
reg [10:0] avgSizeYaxis;
reg enableCOMcomputation; //flag enables sub-modules for center point computation with COM
reg [3:0] delayCounterCOM; //counts up to COM_DELAY_TIME to allow sub-module finish computation

//BLOB containers
//CONTAINER 1
reg[10:0] blob1minX, blob1minY, blob1maxX, blob1maxY; //bounding box attributes
reg [10:0] blob1X_bb_center, blob1Y_bb_center;  //center points for bounding box attributes
reg [35:0] blob1X_com_center, blob1Y_com_center;  //center points for center of mass attributes
reg [35:0] sumBLOB_Xpositions_1; //summed X positions of all pixels belonging to BLOB, weightend by pixel value
reg [35:0] sumBLOB_Ypositions_1; //summed Y positions of all pixels belonging to BLOB, weightend by pixel value
reg [35:0] sumBLOB_Pixels_1;	 //number of pixels belonging to BLOB
reg blob1empty;		//flag information if container is empty

//CONTAINER 2
reg[10:0] blob2minX, blob2minY, blob2maxX, blob2maxY; //bounding box attributes
reg [10:0] blob2X_bb_center, blob2Y_bb_center;  //center points for bounding box attributes
reg [35:0] blob2X_com_center, blob2Y_com_center;  //center points for center of mass attributes
reg [35:0] sumBLOB_Xpositions_2; //summed X positions of all pixels belonging to BLOB, weightend by pixel value
reg [35:0] sumBLOB_Ypositions_2; //summed Y positions of all pixels belonging to BLOB, weightend by pixel value
reg [35:0] sumBLOB_Pixels_2;	 //number of pixels belonging to BLOB
reg blob2empty;		//flag information if container is empty

//CONTAINER 3
reg[10:0] blob3minX, blob3minY, blob3maxX, blob3maxY; //bounding box attributes
reg [10:0] blob3X_bb_center, blob3Y_bb_center;  //center points for bounding box attributes
reg [35:0] blob3X_com_center, blob3Y_com_center;  //center points for center of mass attributes
reg [35:0] sumBLOB_Xpositions_3; //summed X positions of all pixels belonging to BLOB, weightend by pixel value
reg [35:0] sumBLOB_Ypositions_3; //summed Y positions of all pixels belonging to BLOB, weightend by pixel value
reg [35:0] sumBLOB_Pixels_3;	 //number of pixels belonging to BLOB
reg blob3empty;		//flag information if container is empty

//CONTAINER 4
reg[10:0] blob4minX, blob4minY, blob4maxX, blob4maxY; //bounding box attributes
reg [10:0] blob4X_bb_center, blob4Y_bb_center;  //center points for bounding box attributes
reg [35:0] blob4X_com_center, blob4Y_com_center;  //center points for center of mass attributes
reg [35:0] sumBLOB_Xpositions_4; //summed X positions of all pixels belonging to BLOB, weightend by pixel value
reg [35:0] sumBLOB_Ypositions_4; //summed Y positions of all pixels belonging to BLOB, weightend by pixel value
reg [35:0] sumBLOB_Pixels_4;	 //number of pixels belonging to BLOB
reg blob4empty;		//flag information if container is empty

//CONTAINER 5
reg[10:0] blob5minX, blob5minY, blob5maxX, blob5maxY; //bounding box attributes
reg [10:0] blob5X_bb_center, blob5Y_bb_center;  //center points for bounding box attributes
reg [35:0] blob5X_com_center, blob5Y_com_center;  //center points for center of mass attributes
reg [35:0] sumBLOB_Xpositions_5; //summed X positions of all pixels belonging to BLOB, weightend by pixel value
reg [35:0] sumBLOB_Ypositions_5; //summed Y positions of all pixels belonging to BLOB, weightend by pixel value
reg [35:0] sumBLOB_Pixels_5;	 //number of pixels belonging to BLOB
reg blob5empty;		//flag information if container is empty

//CONTAINER 6
reg[10:0] blob6minX, blob6minY, blob6maxX, blob6maxY; //bounding box attributes
reg [10:0] blob6X_bb_center, blob6Y_bb_center;  //center points for bounding box attributes
reg [35:0] blob6X_com_center, blob6Y_com_center;  //center points for center of mass attributes
reg [35:0] sumBLOB_Xpositions_6; //summed X positions of all pixels belonging to BLOB, weightend by pixel value
reg [35:0] sumBLOB_Ypositions_6; //summed Y positions of all pixels belonging to BLOB, weightend by pixel value
reg [35:0] sumBLOB_Pixels_6;	 //number of pixels belonging to BLOB
reg blob6empty;		//flag information if container is empty

//divider varaible

//reg [49:0]divider_res_x;
wire [10:0]divider_res_x;
//reg [49:0]divider_rem_x;
wire [10:0]divider_rem_x;

wire [10:0]divider_res_y;
//reg [49:0]divider_res_y;
wire [10:0]divider_rem_y;
//reg [49:0]divider_rem_y;

wire [10:0] ex_avgSizeXaxis;
//reg [49:0] ex_avgSizeXaxis;
assign ex_avgSizeXaxis = avgSizeXaxis;

wire [10:0]ex_avgSizeYaxis;
//reg [49:0]ex_avgSizeYaxis;
assign ex_avgSizeYaxis = avgSizeYaxis;


wire [3:0]ex_countDetectedBlobs;
//reg [49:0]ex_countDetectedBlobs;
assign ex_countDetectedBlobs = countDetectedBlobs;
//implement the divider
divider inst_x (
		   .opa(ex_avgSizeXaxis), 
		   .opb(ex_countDetectedBlobs), 
		   .quo(divider_res_x),
		   .rem(divider_rem_x));

divider inst_y ( 
		   .opa(ex_avgSizeYaxis), 
		   .opb(ex_countDetectedBlobs), 
		   .quo(divider_res_y),
		   .rem(divider_rem_y));
		   
//read RLE encoded pixel data from input fifo and merge adjacent runs to blobs
//compute blobs center points, on EOF flag
//store center points to output fifo, if EOF flag is read from Fifo
always@(posedge clk) begin
	
	//reset internal registers on reset signal
	if(iReset) begin
		state <= `INIT;
	end
	else begin
	//module state machine
		case(state)
		
	//intialize internal register
		`INIT: begin
			checkResult <= 18'b000000000000000000;
			run_length  <= 10'b0000000000;
			run_start_x <= 11'b00000000000;
			run_start_y <= 11'b00000000000;

			RunAdded <= `FALSE;
			
			//set defaults for CONTAINER 1
			blob1maxX <= 11'b00000000000;
			blob1minX <= 11'b00000000000;
			blob1maxY <= 11'b00000000000;
			blob1minY <= 11'b00000000000;
//			sumBLOB_Xpositions_1=0;
//			sumBLOB_Ypositions_1=0;
//			sumBLOB_Pixels_1=0;			
			blob1empty <= `TRUE;
						
			//set defaults for CONTAINER 2
			blob2maxX <= 11'b00000000000;
			blob2minX <= 11'b00000000000;
			blob2maxY <= 11'b00000000000;
			blob2minY <= 11'b00000000000;	
//			sumBLOB_Xpositions_2=0;
//			sumBLOB_Ypositions_2=0;
//			sumBLOB_Pixels_2=0;			
			blob2empty <= `TRUE;		

			//set defaults for CONTAINER 3
			blob3maxX <= 11'b00000000000;
			blob3minX <= 11'b00000000000;
			blob3maxY <= 11'b00000000000;
			blob3minY <= 11'b00000000000;
//			sumBLOB_Xpositions_3=0;
//			sumBLOB_Ypositions_3=0;
//			sumBLOB_Pixels_3=0;
			blob3empty <= `TRUE;
			
			//set defaults for CONTAINER 4
			blob4maxX <= 11'b00000000000;
			blob4minX <= 11'b00000000000;
			blob4maxY <= 11'b00000000000;
			blob4minY <= 11'b00000000000;
//			sumBLOB_Xpositions_4=0;
//			sumBLOB_Ypositions_4=0;
//			sumBLOB_Pixels_4=0;			
			blob4empty <= `TRUE;
								

			//set defaults for CONTAINER 5
			blob5maxX <= 11'b00000000000;
			blob5minX <= 11'b00000000000;
			blob5maxY <= 11'b00000000000;
			blob5minY <= 11'b00000000000;
//			sumBLOB_Xpositions_5=0;
//			sumBLOB_Ypositions_5=0;
//			sumBLOB_Pixels_5=0;			
			blob5empty <= `TRUE;					

			//set defaults for CONTAINER 6
			blob6maxX <= 11'b00000000000;
			blob6minX <= 11'b00000000000;
			blob6maxY <= 11'b00000000000;
			blob6minY <= 11'b00000000000;
//			sumBLOB_Xpositions_6<=0;
//			sumBLOB_Ypositions_6<=0;
//			sumBLOB_Pixels_6<=0;				
			blob6empty <= `TRUE;	
			
			
			blob1X_com_center <= 35'b00000000000000000000000000000000000;
			blob1Y_com_center <= 35'b00000000000000000000000000000000000;
			blob2X_com_center <= 35'b00000000000000000000000000000000000;
			blob2Y_com_center <= 35'b00000000000000000000000000000000000;
			blob3X_com_center <= 35'b00000000000000000000000000000000000;
			blob3Y_com_center <= 35'b00000000000000000000000000000000000;
			blob4X_com_center <= 35'b00000000000000000000000000000000000;
			blob4Y_com_center <= 35'b00000000000000000000000000000000000;
			blob5X_com_center <= 35'b00000000000000000000000000000000000;
			blob5Y_com_center <= 35'b00000000000000000000000000000000000;
			blob6X_com_center <= 35'b00000000000000000000000000000000000;
			blob6Y_com_center <= 35'b00000000000000000000000000000000000;
			//blob4X_com_center
			//blob4X_com_center
		
		
			avgSizeXaxis<=11'b00000000000;
			avgSizeYaxis<=11'b00000000000;
			countDetectedBlobs<=4'b0000;	
			enableCOMcomputation<=`FALSE;		
						
			state <= `IDLE;
		end
	
	//read run data from input fifo
		`IDLE: begin
			if(!iReadFifoEmpty) begin
				oReadFifoRequest <= `TRUE;
				state <= `READ_FIFO;
			end
			else begin
				state <= `IDLE;
			end
		end
		
	//receive data from input fifo
		`READ_FIFO: begin
			oReadFifoRequest <= `FALSE;
			
			//check for EOF flag
			if(iReadFifoData[`MSB_EOFflag:`LSB_EOFflag]==`EOF)begin				
				write_result_pointer<=4'b0000;
				
				//start center point computation for COM
				enableCOMcomputation<=`TRUE;
				delayCounterCOM<=4'b0000; //reset delay time counter
								
				state <= `COMPUTE_CENTER;
			end
			else begin
				run_length  <= iReadFifoData[31:22];
				run_start_x <= iReadFifoData[10:0];
				run_start_y <= iReadFifoData[21:11];
				run_sum_x_positions <= iReadFifoData[95:64];
				run_sum_y_positions <= iReadFifoData[127:96];
				run_sum_values <= iReadFifoData[63:32];
				checkResult <= 18'b000000000000000000;
				RunAdded <= `FALSE;
				state <= `CHECK_CONDITIONS;
			end
		end
		
	//perform condition checks
		`CHECK_CONDITIONS: begin
			
			//check which BLOB containers are empty
			checkResult[0] <= blob1empty;
			checkResult[3] <= blob2empty;
			checkResult[6] <= blob3empty;
			checkResult[9] <= blob4empty;
			checkResult[12] <= blob5empty;
			checkResult[15] <= blob6empty;
			
			
			//check if run is adjacent on X values to blob 1 container
			if( (((run_start_x+`BLOB_RANGE) >= blob1minX) & (run_start_x <= (blob1maxX+`BLOB_RANGE))) ||
				((run_start_x+run_length+`BLOB_RANGE) >= blob1minX) & ((run_start_x+run_length) <= (blob1maxX+`BLOB_RANGE)) ) begin
				checkResult[1] <= `TRUE;
			end
			//check if run is adjacent on X values to blob 2 container
//			if( ((run_start_x+`BLOB_RANGE) >= blob2minX) & (run_start_x <= blob2maxX) &
//				((run_start_x+run_length) >= blob2minX) & ((run_start_x+run_length) <= (blob2maxX+`BLOB_RANGE)) ) begin
			if( (((run_start_x+`BLOB_RANGE) >= blob2minX) & (run_start_x <= (blob2maxX+`BLOB_RANGE))) ||
				((run_start_x+run_length+`BLOB_RANGE) >= blob2minX) & ((run_start_x+run_length) <= (blob2maxX+`BLOB_RANGE)) ) begin
				checkResult[4] <= `TRUE;
			end			
			//check if run is adjacent on X values to blob 3 container
//			if( ((run_start_x+`BLOB_RANGE) >= blob3minX) & (run_start_x <= blob3maxX) &
//				((run_start_x+run_length) >= blob3minX) & ((run_start_x+run_length) <= (blob3maxX+`BLOB_RANGE)) ) begin
			if( (((run_start_x+`BLOB_RANGE) >= blob3minX) & (run_start_x <= (blob3maxX+`BLOB_RANGE))) ||
				((run_start_x+run_length+`BLOB_RANGE) >= blob3minX) & ((run_start_x+run_length) <= (blob3maxX+`BLOB_RANGE)) ) begin
				checkResult[7] <= `TRUE;
			end		
			//check if run is adjacent on X values to blob 4 container
//			if( ((run_start_x+`BLOB_RANGE) >= blob4minX) & (run_start_x <= blob4maxX) &
//				((run_start_x+run_length) >= blob4minX) & ((run_start_x+run_length) <= (blob4maxX+`BLOB_RANGE)) ) begin
			if( (((run_start_x+`BLOB_RANGE) >= blob4minX) & (run_start_x <= (blob4maxX+`BLOB_RANGE))) ||
				((run_start_x+run_length+`BLOB_RANGE) >= blob4minX) & ((run_start_x+run_length) <= (blob4maxX+`BLOB_RANGE)) ) begin
				checkResult[10] <= `TRUE;
			end			
			//check if run is adjacent on X values to blob 5 container
//			if( ((run_start_x+`BLOB_RANGE) >= blob5minX) & (run_start_x <= blob5maxX) &
//				((run_start_x+run_length) >= blob5minX) & ((run_start_x+run_length) <= (blob5maxX+`BLOB_RANGE)) ) begin
			if( (((run_start_x+`BLOB_RANGE) >= blob5minX) & (run_start_x <= (blob5maxX+`BLOB_RANGE))) ||
				((run_start_x+run_length+`BLOB_RANGE) >= blob5minX) & ((run_start_x+run_length) <= (blob5maxX+`BLOB_RANGE)) ) begin
				checkResult[13] <= `TRUE;
			end
			
			//check if run is adjacent on X values to blob 6 container
			if( (((run_start_x+`BLOB_RANGE) >= blob6minX) & (run_start_x <= (blob6maxX+`BLOB_RANGE))) ||
				((run_start_x+run_length+`BLOB_RANGE) >= blob6minX) & ((run_start_x+run_length) <= (blob6maxX+`BLOB_RANGE)) ) begin
				checkResult[16] <= `TRUE;
			end
						
			//check if run is adjacent on Y values to blob 1 container
			if( (run_start_y == (blob1maxY +1)) || (run_start_y == blob1maxY) ) begin
				checkResult[2] <= `TRUE;
			end
			//check if run is adjacent on Y values to blob 2 container
			if( (run_start_y == (blob2maxY +1)) || (run_start_y == blob2maxY) ) begin
				checkResult[5] <= `TRUE;
			end
			//check if run is adjacent on Y values to blob 3 container
			if( (run_start_y == (blob3maxY +1)) || (run_start_y == blob3maxY) ) begin
				checkResult[8] <= `TRUE;
			end
			//check if run is adjacent on Y values to blob 4 container
			if( (run_start_y == (blob4maxY +1)) || (run_start_y == blob4maxY) ) begin
				checkResult[11] <= `TRUE;
			end
			//check if run is adjacent on Y values to blob 5 container
			if( (run_start_y == (blob5maxY +1)) || (run_start_y == blob5maxY) ) begin
				checkResult[14] <= `TRUE;
			end	
			//check if run is adjacent on Y values to blob 5 container
			if( (run_start_y == (blob6maxY +1)) || (run_start_y == blob6maxY) ) begin
				checkResult[17] <= `TRUE;
			end	
						
			state <= `CHECK_ADJACENT_CONTAINTER;								
		end

	       //validate results from condition check for adjacency
		`CHECK_ADJACENT_CONTAINTER: begin
				
            if(
                   ((~checkResult[0])&checkResult[1]&checkResult[2])
                || ((~checkResult[3])&checkResult[4]&checkResult[5]) 
                || ((~checkResult[6])&checkResult[7]&checkResult[8]) 
                || ((~checkResult[9])&checkResult[10]&checkResult[11]) 
                || ((~checkResult[12])&checkResult[13]&checkResult[14])             
                || ((~checkResult[15])&checkResult[16]&checkResult[17]) 
            )  
            begin         
				RunAdded <= `TRUE;
			end
								
			//run adjacent to container 1
			if( (~checkResult[0])&checkResult[1]&checkResult[2] ) 
			begin
				if(run_start_x < blob1minX) blob1minX <= run_start_x;
				if((run_start_x+run_length) > blob1maxX) blob1maxX <= (run_start_x+run_length);				
				if(blob1maxY < run_start_y) blob1maxY <= run_start_y;				
			end

			//run adjacent to container 2
			if( (~checkResult[3])&checkResult[4]&checkResult[5] ) 
			begin
				if(run_start_x < blob2minX) blob2minX <= run_start_x;
				if((run_start_x+run_length) > blob2maxX) blob2maxX <= (run_start_x+run_length);
				if(blob2maxY < run_start_y) blob2maxY <= run_start_y;				
			end
			
			//run adjacent to container 3
			if( (~checkResult[6])&checkResult[7]&checkResult[8] ) 
			begin
				if(run_start_x < blob3minX) blob3minX <= run_start_x;
				if((run_start_x+run_length) > blob3maxX) blob3maxX <= (run_start_x+run_length);
				if(blob3maxY < run_start_y) blob3maxY <= run_start_y;
			end
			
			//run adjacent to container 4
			if( (~checkResult[9])&checkResult[10]&checkResult[11] ) 
			begin
				if(run_start_x < blob4minX) blob4minX <= run_start_x;
				if((run_start_x+run_length) > blob4maxX) blob4maxX <= (run_start_x+run_length);
				if(blob4maxY < run_start_y) blob4maxY <= run_start_y;				
			end
			
			//run adjacent to container 5
			if( (~checkResult[12])&checkResult[13]&checkResult[14] ) 
			begin
				if(run_start_x < blob5minX) blob5minX <= run_start_x;
				if((run_start_x+run_length) > blob5maxX) blob5maxX <= (run_start_x+run_length);				
				if(blob5maxY < run_start_y) blob5maxY <= run_start_y;
			end										

			//run adjacent to container 6
			if( (~checkResult[15])&checkResult[16]&checkResult[17] ) 
			begin
				if(run_start_x < blob6minX) blob6minX <= run_start_x;
				if((run_start_x+run_length) > blob6maxX) blob6maxX <= (run_start_x+run_length);
				if(blob6maxY < run_start_y) blob6maxY <= run_start_y;			
			end		
						
			//read next run from input fifo
			state <= `VALIDATE_CHECK_ADJACENT_CONTAINER;
		end
	
	//check if run could be added to container
	`VALIDATE_CHECK_ADJACENT_CONTAINER: begin
		//if run has been added, continue with next run
		if(RunAdded) begin
			state <= `CHECK_CONT_ADJACENCY;
		end
		//if run not adjacent, put into empty container
		else begin
			state <= `SELECT_EMPTY_CONTAINER;
		end
	end
	
	//if run was not adjacent, add to new container
		`SELECT_EMPTY_CONTAINER: begin

			//first cont empty
			if( checkResult[0] ) begin
				blob1minX <= run_start_x;
				blob1maxX <= run_start_x+run_length;
				blob1minY <= run_start_y;
				blob1maxY <= run_start_y;
				
//				sumBLOB_Xpositions_1<=run_sum_x_positions;
//				sumBLOB_Ypositions_1<=run_sum_y_positions;
//				sumBLOB_Pixels_1<=run_sum_values;				
								
				blob1empty<=`FALSE;
			end
			
			//run not adjacent to existing blobs and container 2 is empty
			//second cont empty
			else if( checkResult[3]) begin
				blob2minX <= run_start_x;
				blob2maxX <= run_start_x+run_length;
				blob2minY <= run_start_y;
				blob2maxY <= run_start_y;
				
//				sumBLOB_Xpositions_2<=run_sum_x_positions;
//				sumBLOB_Ypositions_2<=run_sum_y_positions;
//				sumBLOB_Pixels_2<=run_sum_values;
							
				blob2empty<=`FALSE;		
			end

			//run not adjacent to existing blobs and container 3 is empty
			//third cont empty
			if( checkResult[6]) begin
				blob3minX <= run_start_x;
				blob3maxX <= run_start_x+run_length;
				blob3minY <= run_start_y;
				blob3maxY <= run_start_y;

//				sumBLOB_Xpositions_3<=run_sum_x_positions;
//				sumBLOB_Ypositions_3<=run_sum_y_positions;
//				sumBLOB_Pixels_3<=run_sum_values;
								
				blob3empty<=`FALSE;		
			end

			//run not adjacent to existing blobs and container 4 is empty		
			//fourth cont empty
			else if( checkResult[9])begin							   
				blob4minX <= run_start_x;
				blob4maxX <= run_start_x+run_length;
				blob4minY <= run_start_y;
				blob4maxY <= run_start_y;

//				sumBLOB_Xpositions_4<=run_sum_x_positions;
//				sumBLOB_Ypositions_4<=run_sum_y_positions;
//				sumBLOB_Pixels_4<=run_sum_values;
								
				blob4empty<=`FALSE;		
			end		
			
			//run not adjacent to existing blobs and container 5 is empty
			//fifth cont empty
			else if( checkResult[12]) begin 
				blob5minX <= run_start_x;
				blob5maxX <= run_start_x+run_length;
				blob5minY <= run_start_y;
				blob5maxY <= run_start_y;
				
//				sumBLOB_Xpositions_5<=run_sum_x_positions;
//				sumBLOB_Ypositions_5<=run_sum_y_positions;
//				sumBLOB_Pixels_5<=run_sum_values;				
				
				blob5empty<=`FALSE;		
			end				

			else if( checkResult[15]) begin 
				blob6minX <= run_start_x;
				blob6maxX <= run_start_x+run_length;
				blob6minY <= run_start_y;
				blob6maxY <= run_start_y;
				
//				sumBLOB_Xpositions_6<=run_sum_x_positions;
//				sumBLOB_Ypositions_6<=run_sum_y_positions;
//				sumBLOB_Pixels_6<=run_sum_values;
								
				blob6empty<=`FALSE;		
			end	
						
			state <= `CHECK_CONT_ADJACENCY;
		end
	
	//compute center points for non-empty containers
	//center point computation with BB and COM
		`COMPUTE_CENTER: begin	
			//compute center point for blob in Container 1
			blob1X_bb_center <= ((blob1minX + blob1maxX)>>1'b1);
			blob1Y_bb_center <= ((blob1minY + blob1maxY)>>1'b1);
			//compute center point for blob in Container 2
			blob2X_bb_center <= ((blob2minX + blob2maxX)>>1'b1);
			blob2Y_bb_center <= ((blob2minY + blob2maxY)>>1'b1);
			//compute center point for blob in Container 3
			blob3X_bb_center <= ((blob3minX + blob3maxX)>>1'b1);
			blob3Y_bb_center <= ((blob3minY + blob3maxY)>>1'b1);
			//compute center point for blob in Container 4
			blob4X_bb_center <= ((blob4minX + blob4maxX)>>1'b1);
			blob4Y_bb_center <= ((blob4minY + blob4maxY)>>1'b1);
			//compute center point for blob in Container 5
			blob5X_bb_center <= ((blob5minX + blob5maxX)>>1'b1);
			blob5Y_bb_center <= ((blob5minY + blob5maxY)>>1'b1);
			//compute center point for blob in Container 6
			blob6X_bb_center <= ((blob6minX + blob6maxX)>>1'b1);
			blob6Y_bb_center <= ((blob6minY + blob6maxY)>>1'b1);
								
			//increase delay time counter for COM computation
			//delayCounterCOM<=delayCounterCOM+3'd1;
			
			state<=`CONFIG_WRITE;
		end
				
		`CONFIG_WRITE: begin	
			//increase delay time counter for COM computation
			//delayCounterCOM<=delayCounterCOM+3'd1;
			
			//if(delayCounterCOM > COM_DELAY_TIME) begin
				//selector for write progress
				case(write_result_pointer)
				
				`WRITE_BLOB_0: begin
					//if(blob1empty==`FALSE && blob1Y > 1) begin
					if(blob1empty==`FALSE) begin
						//oWriteBlobData={10'd1, blob1Y, blob1X,(blob1maxY-blob1minY),(blob1maxX-blob1minX)};
						oWriteBlobData[9:0]<=10'b0000000001;   	       
						oWriteBlobData[20:10]<=blob1Y_bb_center; 
						oWriteBlobData[31:21]<=blob1X_bb_center;
						oWriteBlobData[42:32]<=blob1Y_com_center[10:0];
						oWriteBlobData[53:43]<=blob1X_com_center[10:0];
						oWriteBlobData[64:54]<=(blob1maxY-blob1minY);
						oWriteBlobData[75:65]<=(blob1maxX-blob1minX);
						avgSizeYaxis<=(blob1maxY-blob1minY);
						avgSizeXaxis<=(blob1maxX-blob1minX);
						countDetectedBlobs<=countDetectedBlobs+1'b1;
						state<=`WRITE_FIFO;
					end
					else begin
						write_result_pointer<=write_result_pointer+1'b1;
						state<=`CONFIG_WRITE;
					end
				end
				
				`WRITE_BLOB_1: begin
					//if(blob2empty==`FALSE && blob2Y > 1) begin
					if(blob2empty==`FALSE) begin
						//oWriteBlobData={10'd2, blob2Y, blob2X,(blob2maxY-blob2minY),(blob2maxX-blob2minX)};
						oWriteBlobData[9:0]<=10'b0000000001;   	       
						oWriteBlobData[20:10]<=blob2Y_bb_center; 
						oWriteBlobData[31:21]<=blob2X_bb_center;
						oWriteBlobData[42:32]<=blob2Y_com_center[10:0];
						oWriteBlobData[53:43]<=blob2X_com_center[10:0];
						oWriteBlobData[64:54]<=(blob2maxY-blob2minY);
						oWriteBlobData[75:65]<=(blob2maxX-blob2minX);
						avgSizeYaxis<=avgSizeYaxis+(blob2maxY-blob2minY);
						avgSizeXaxis<=avgSizeXaxis+(blob2maxX-blob2minX);
						countDetectedBlobs<=countDetectedBlobs+1'b1;
						state<=`WRITE_FIFO;
					end
					else begin
						write_result_pointer<=write_result_pointer+4'b0001;
						state<=`CONFIG_WRITE;
					end
				end
				
				`WRITE_BLOB_2: begin
					//if(blob3empty==`FALSE && blob3Y > 1)  begin
					if(blob3empty==`FALSE) begin
						//oWriteBlobData={10'd3, blob3Y, blob3X,(blob3maxY-blob3minY),(blob3maxX-blob3minX)};
						oWriteBlobData[9:0]<=10'b0000000001;   	       
						oWriteBlobData[20:10]<=blob3Y_bb_center; 
						oWriteBlobData[31:21]<=blob3X_bb_center;
						oWriteBlobData[42:32]<=blob3Y_com_center[10:0];
						oWriteBlobData[53:43]<=blob3X_com_center[10:0];
						oWriteBlobData[64:54]<=(blob3maxY-blob3minY);
						oWriteBlobData[75:65]<=(blob3maxX-blob3minX);
						avgSizeYaxis<=avgSizeYaxis+(blob3maxY-blob3minY);
						avgSizeXaxis<=avgSizeXaxis+(blob3maxX-blob3minX);
						countDetectedBlobs<=countDetectedBlobs+1'b1;
						state<=`WRITE_FIFO;
					end
					else begin
						write_result_pointer<=write_result_pointer+4'b0001;
						state<=`CONFIG_WRITE;
					end		
				end
				
				`WRITE_BLOB_3: begin
					//if(blob4empty==`FALSE && blob4Y > 1) begin
					if(blob4empty==`FALSE) begin
						//oWriteBlobData={10'd4, blob4Y, blob4X,(blob4maxY-blob4minY),(blob4maxX-blob4minX)};
						oWriteBlobData[9:0]<=10'b0000000001;   	       
						oWriteBlobData[20:10]<=blob4Y_bb_center; 
						oWriteBlobData[31:21]<=blob4X_bb_center;
						oWriteBlobData[42:32]<=blob4Y_com_center[10:0];
						oWriteBlobData[53:43]<=blob4X_com_center[10:0];
						oWriteBlobData[64:54]<=(blob4maxY-blob4minY);
						oWriteBlobData[75:65]<=(blob4maxX-blob4minX);
						avgSizeYaxis<=avgSizeYaxis+(blob4maxY-blob4minY);
						avgSizeXaxis<=avgSizeXaxis+(blob4maxX-blob4minX);
						countDetectedBlobs<=countDetectedBlobs+1'b1;
						state<=`WRITE_FIFO;
					end
					else begin
						write_result_pointer<=write_result_pointer+1'b1;
						state<=`CONFIG_WRITE;
					end	
				end
				
				`WRITE_BLOB_4: begin
					//if(blob5empty==`FALSE && blob5Y > 1) begin
					if(blob5empty==`FALSE) begin
						//oWriteBlobData={10'd5, blob5Y, blob5X,(blob5maxY-blob5minY),(blob5maxX-blob5minX)};
						oWriteBlobData[9:0]<=10'b0000000001;   	       
						oWriteBlobData[20:10]<=blob5Y_bb_center; 
						oWriteBlobData[31:21]<=blob5X_bb_center;
						oWriteBlobData[42:32]<=blob5Y_com_center[10:0];
						oWriteBlobData[53:43]<=blob5X_com_center[10:0];
						oWriteBlobData[64:54]<=(blob5maxY-blob5minY);
						oWriteBlobData[75:65]<=(blob5maxX-blob5minX);
						avgSizeYaxis<=avgSizeYaxis+(blob5maxY-blob5minY);
						avgSizeXaxis<=avgSizeXaxis+(blob5maxX-blob5minX);
						countDetectedBlobs<=countDetectedBlobs+1'b1;
						state<=`WRITE_FIFO;
					end
					else begin
						write_result_pointer<=write_result_pointer+1'b1;
						state<=`CONFIG_WRITE;
					end
				end

				`WRITE_BLOB_5: begin
					//if(blob6empty==`FALSE && blob6Y > 1) begin
					if(blob6empty==`FALSE) begin
						//oWriteBlobData={10'd6, blob6Y, blob6X,(blob6maxY-blob6minY),(blob6maxX-blob6minX)};
						oWriteBlobData[9:0]<=10'b0000000001;   	       
						oWriteBlobData[20:10]<=blob6Y_bb_center; 
						oWriteBlobData[31:21]<=blob6X_bb_center;
						oWriteBlobData[42:32]<=blob6Y_com_center[10:0];
						oWriteBlobData[53:43]<=blob6X_com_center[10:0];
						oWriteBlobData[64:54]<=(blob6maxY-blob6minY);
						oWriteBlobData[75:65]<=(blob6maxX-blob6minX);
						
						avgSizeYaxis<=avgSizeYaxis+(blob6maxY-blob6minY);
						avgSizeXaxis<=avgSizeXaxis+(blob6maxX-blob6minX);
						countDetectedBlobs<=countDetectedBlobs+1'b1;
						state<=`WRITE_FIFO;
					end
					else begin
						write_result_pointer<=write_result_pointer+1'b1;
						state<=`CONFIG_WRITE;
					end
				end
							
				default: begin
					oAvgSizeXaxis<=divider_res_x;
					oAvgSizeYaxis<=divider_res_y;						
					//oWriteBlobData<=32'h00000000;
					oWriteBlobData<=0;
					state<=`INIT;//continue processing pixel from new frame				
				end
				//NO DEFAULT(DIVIDER, CONG)						
				endcase
				//end
			//end

		end


				
		`WRITE_FIFO: begin
			if(!iWriteFifoFull) begin
				oWriteRequest<=`TRUE;
				state<=`WRITE_WAIT;
			end
		end
		
		`WRITE_WAIT: begin
			oWriteRequest<=`FALSE;
			write_result_pointer<=write_result_pointer+1'b1;
			state<=`CONFIG_WRITE;			
		end
		
	`CHECK_CONT_ADJACENCY: begin	
//MERGE TO CONTAINER 1
			if(blob2empty==`FALSE && blob1empty==`FALSE) begin
				if( ((blob2minY-blob1minY) < `CONT_RANGE) || ((blob1minY-blob2minY) < `CONT_RANGE) ||
					((blob2minY-blob1maxY) < `CONT_RANGE) || ((blob1maxY-blob2minY) < `CONT_RANGE) ||
					((blob2maxY-blob1minY) < `CONT_RANGE) || ((blob1minY-blob2maxY) < `CONT_RANGE) ||
					((blob2maxY-blob1maxY) < `CONT_RANGE) || ((blob1maxY-blob2maxY) < `CONT_RANGE)) begin
					
					if( ((blob2minX-blob1minX) < `CONT_RANGE) || ((blob1minX-blob2minX) < `CONT_RANGE) ||
						((blob2minX-blob1maxX) < `CONT_RANGE) || ((blob1maxX-blob2minX) < `CONT_RANGE) ||
						((blob2maxX-blob1minX) < `CONT_RANGE) || ((blob1minX-blob2maxX) < `CONT_RANGE) ||
						((blob2maxX-blob1maxX) < `CONT_RANGE) || ((blob1maxX-blob2maxX) < `CONT_RANGE)) begin
						ContainerAdjacentResult[0]<=`TRUE;				
					end
				end
			end
			if(blob3empty==`FALSE && blob1empty==`FALSE) begin
								//check adjacency on Y axis
				if( ((blob3minY-blob1minY) < `CONT_RANGE) || ((blob1minY-blob3minY) < `CONT_RANGE) ||
					((blob3minY-blob1maxY) < `CONT_RANGE) || ((blob1maxY-blob3minY) < `CONT_RANGE) ||
					((blob3maxY-blob1minY) < `CONT_RANGE) || ((blob1minY-blob3maxY) < `CONT_RANGE) ||
					((blob3maxY-blob1maxY) < `CONT_RANGE) || ((blob1maxY-blob3maxY) < `CONT_RANGE)) begin
					
					if( ((blob3minX-blob1minX) < `CONT_RANGE) || ((blob1minX-blob3minX) < `CONT_RANGE) ||
						((blob3minX-blob1maxX) < `CONT_RANGE) || ((blob1maxX-blob3minX) < `CONT_RANGE) ||
						((blob3maxX-blob1minX) < `CONT_RANGE) || ((blob1minX-blob3maxX) < `CONT_RANGE) ||
						((blob3maxX-blob1maxX) < `CONT_RANGE) || ((blob1maxX-blob3maxX) < `CONT_RANGE)) begin
						ContainerAdjacentResult[1]<=`TRUE;				
					end
				end
			end
			if(blob4empty==`FALSE && blob1empty==`FALSE) begin
				//check adjacency on Y axis
				//Merge Container 1 and 4 if adjacent
				//check adjacency on Y axis
				if( ((blob4minY-blob1minY) < `CONT_RANGE) || ((blob1minY-blob4minY) < `CONT_RANGE) ||
					((blob4minY-blob1maxY) < `CONT_RANGE) || ((blob1maxY-blob4minY) < `CONT_RANGE) ||
					((blob4maxY-blob1minY) < `CONT_RANGE) || ((blob1minY-blob4maxY) < `CONT_RANGE) ||
					((blob4maxY-blob1maxY) < `CONT_RANGE) || ((blob1maxY-blob4maxY) < `CONT_RANGE)) begin
					
					if( ((blob4minX-blob1minX) < `CONT_RANGE) || ((blob1minX-blob4minX) < `CONT_RANGE) ||
						((blob4minX-blob1maxX) < `CONT_RANGE) || ((blob1maxX-blob4minX) < `CONT_RANGE) ||
						((blob4maxX-blob1minX) < `CONT_RANGE) || ((blob1minX-blob4maxX) < `CONT_RANGE) ||
						((blob4maxX-blob1maxX) < `CONT_RANGE) || ((blob1maxX-blob4maxX) < `CONT_RANGE)) begin
						ContainerAdjacentResult[2]<=`TRUE;				
					end
				end
			end
			if(blob5empty==`FALSE && blob1empty==`FALSE) begin
				if( ((blob5minY-blob1minY) < `CONT_RANGE) || ((blob1minY-blob5minY) < `CONT_RANGE) ||
					((blob5minY-blob1maxY) < `CONT_RANGE) || ((blob1maxY-blob5minY) < `CONT_RANGE) ||
					((blob5maxY-blob1minY) < `CONT_RANGE) || ((blob1minY-blob5maxY) < `CONT_RANGE) ||
					((blob5maxY-blob1maxY) < `CONT_RANGE) || ((blob1maxY-blob5maxY) < `CONT_RANGE)) begin
					
					if( ((blob5minX-blob1minX) < `CONT_RANGE) || ((blob1minX-blob5minX) < `CONT_RANGE) ||
						((blob5minX-blob1maxX) < `CONT_RANGE) || ((blob1maxX-blob5minX) < `CONT_RANGE) ||
						((blob5maxX-blob1minX) < `CONT_RANGE) || ((blob1minX-blob5maxX) < `CONT_RANGE) ||
						((blob5maxX-blob1maxX) < `CONT_RANGE) || ((blob1maxX-blob5maxX) < `CONT_RANGE)) begin
						ContainerAdjacentResult[3]<=`TRUE;				
					end
				end
			end			
			if(blob6empty==`FALSE && blob1empty==`FALSE) begin
				if( ((blob6minY-blob1minY) < `CONT_RANGE) || ((blob1minY-blob6minY) < `CONT_RANGE) ||
					((blob6minY-blob1maxY) < `CONT_RANGE) || ((blob1maxY-blob6minY) < `CONT_RANGE) ||
					((blob6maxY-blob1minY) < `CONT_RANGE) || ((blob1minY-blob6maxY) < `CONT_RANGE) ||
					((blob6maxY-blob1maxY) < `CONT_RANGE) || ((blob1maxY-blob6maxY) < `CONT_RANGE)) begin
					
					if( ((blob6minX-blob1minX) < `CONT_RANGE) || ((blob1minX-blob6minX) < `CONT_RANGE) ||
						((blob6minX-blob1maxX) < `CONT_RANGE) || ((blob1maxX-blob6minX) < `CONT_RANGE) ||
						((blob6maxX-blob1minX) < `CONT_RANGE) || ((blob1minX-blob6maxX) < `CONT_RANGE) ||
						((blob6maxX-blob1maxX) < `CONT_RANGE) || ((blob1maxX-blob6maxX) < `CONT_RANGE)) begin
						ContainerAdjacentResult[10]<=`TRUE;				
					end
				end
			end	
						
//MERGE TO CONTAINER 2			
			if(blob3empty==`FALSE && blob2empty==`FALSE) begin
				if( ((blob2minY-blob3minY) < `CONT_RANGE) || ((blob3minY-blob2minY) < `CONT_RANGE) ||
					((blob2minY-blob3maxY) < `CONT_RANGE) || ((blob3maxY-blob2minY) < `CONT_RANGE) ||
					((blob2maxY-blob3minY) < `CONT_RANGE) || ((blob3minY-blob2maxY) < `CONT_RANGE) ||
					((blob2maxY-blob3maxY) < `CONT_RANGE) || ((blob3maxY-blob2maxY) < `CONT_RANGE) )begin
					
					if( ((blob2minX-blob3minX) < `CONT_RANGE) || ((blob3minX-blob2minX) < `CONT_RANGE) ||
						((blob2minX-blob3maxX) < `CONT_RANGE) || ((blob3maxX-blob2minX) < `CONT_RANGE) ||
						((blob2maxX-blob3minX) < `CONT_RANGE) || ((blob3minX-blob2maxX) < `CONT_RANGE) ||
						((blob2maxX-blob3maxX) < `CONT_RANGE) || ((blob3maxX-blob2maxX) < `CONT_RANGE) )begin
						ContainerAdjacentResult[4]<=`TRUE;				
					end	
				end
			end	
			if(blob4empty==`FALSE && blob2empty==`FALSE) begin
				if( ((blob2minY-blob4minY) < `CONT_RANGE) || ((blob4minY-blob2minY) < `CONT_RANGE) ||
					((blob2minY-blob4maxY) < `CONT_RANGE) || ((blob4maxY-blob2minY) < `CONT_RANGE) ||
					((blob2maxY-blob4minY) < `CONT_RANGE) || ((blob4minY-blob2maxY) < `CONT_RANGE) ||
					((blob2maxY-blob4maxY) < `CONT_RANGE) || ((blob4maxY-blob2maxY) < `CONT_RANGE) )begin
					
					if( ((blob2minX-blob4minX) < `CONT_RANGE) || ((blob4minX-blob2minX) < `CONT_RANGE) ||
						((blob2minX-blob4maxX) < `CONT_RANGE) || ((blob4maxX-blob2minX) < `CONT_RANGE) ||
						((blob2maxX-blob4minX) < `CONT_RANGE) || ((blob4minX-blob2maxX) < `CONT_RANGE) ||
						((blob2maxX-blob4maxX) < `CONT_RANGE) || ((blob4maxX-blob2maxX) < `CONT_RANGE) )begin
						ContainerAdjacentResult[5]<=`TRUE;				
					end
				end
			end	
			if(blob5empty==`FALSE && blob2empty==`FALSE) begin
				if( ((blob2minY-blob5minY) < `CONT_RANGE) || ((blob5minY-blob2minY) < `CONT_RANGE) ||
					((blob2minY-blob5maxY) < `CONT_RANGE) || ((blob5maxY-blob2minY) < `CONT_RANGE) ||
					((blob2maxY-blob5minY) < `CONT_RANGE) || ((blob5minY-blob2maxY) < `CONT_RANGE) ||
					((blob2maxY-blob5maxY) < `CONT_RANGE) || ((blob5maxY-blob2maxY) < `CONT_RANGE) )begin
					
					if( ((blob2minX-blob5minX) < `CONT_RANGE) || ((blob5minX-blob2minX) < `CONT_RANGE) ||
						((blob2minX-blob5maxX) < `CONT_RANGE) || ((blob5maxX-blob2minX) < `CONT_RANGE) ||
						((blob2maxX-blob5minX) < `CONT_RANGE) || ((blob5minX-blob2maxX) < `CONT_RANGE) ||
						((blob2maxX-blob5maxX) < `CONT_RANGE) || ((blob5maxX-blob2maxX) < `CONT_RANGE) )begin
						ContainerAdjacentResult[6]<=`TRUE;				
					end
				end
			end	
			if(blob6empty==`FALSE && blob2empty==`FALSE) begin
				if( ((blob2minY-blob6minY) < `CONT_RANGE) || ((blob6minY-blob2minY) < `CONT_RANGE) ||
					((blob2minY-blob6maxY) < `CONT_RANGE) || ((blob6maxY-blob2minY) < `CONT_RANGE) ||
					((blob2maxY-blob6minY) < `CONT_RANGE) || ((blob6minY-blob2maxY) < `CONT_RANGE) ||
					((blob2maxY-blob6maxY) < `CONT_RANGE) || ((blob6maxY-blob2maxY) < `CONT_RANGE) )begin
					
					if( ((blob2minX-blob6minX) < `CONT_RANGE) || ((blob6minX-blob2minX) < `CONT_RANGE) ||
						((blob2minX-blob6maxX) < `CONT_RANGE) || ((blob6maxX-blob2minX) < `CONT_RANGE) ||
						((blob2maxX-blob6minX) < `CONT_RANGE) || ((blob6minX-blob2maxX) < `CONT_RANGE) ||
						((blob2maxX-blob6maxX) < `CONT_RANGE) || ((blob6maxX-blob2maxX) < `CONT_RANGE) )begin
						ContainerAdjacentResult[11]<=`TRUE;				
					end
				end
			end	
						
//MERGE CONTAINER 3
			if(blob4empty==`FALSE && blob3empty==`FALSE) begin
				if( ((blob4minY-blob3minY) < `CONT_RANGE) || ((blob3minY-blob4minY) < `CONT_RANGE) ||
					((blob4minY-blob3maxY) < `CONT_RANGE) || ((blob3maxY-blob4minY) < `CONT_RANGE) ||
					((blob4maxY-blob3minY) < `CONT_RANGE) || ((blob3minY-blob4maxY) < `CONT_RANGE) ||
					((blob4maxY-blob3maxY) < `CONT_RANGE) || ((blob3maxY-blob4maxY) < `CONT_RANGE)) begin
					
					if( ((blob4minX-blob3minX) < `CONT_RANGE) || ((blob3minX-blob4minX) < `CONT_RANGE) ||
						((blob4minX-blob3maxX) < `CONT_RANGE) || ((blob3maxX-blob4minX) < `CONT_RANGE) ||
						((blob4maxX-blob3minX) < `CONT_RANGE) || ((blob3minX-blob4maxX) < `CONT_RANGE) ||
						((blob4maxX-blob3maxX) < `CONT_RANGE) || ((blob3maxX-blob4maxX) < `CONT_RANGE)) begin
						ContainerAdjacentResult[7]<=`TRUE;				
					end
				end
			end	
			if(blob5empty==`FALSE && blob3empty==`FALSE) begin
				if( ((blob5minY-blob3minY) < `CONT_RANGE) || ((blob3minY-blob5minY) < `CONT_RANGE) ||
					((blob5minY-blob3maxY) < `CONT_RANGE) || ((blob3maxY-blob5minY) < `CONT_RANGE) ||
					((blob5maxY-blob3minY) < `CONT_RANGE) || ((blob3minY-blob5maxY) < `CONT_RANGE) ||
					((blob5maxY-blob3maxY) < `CONT_RANGE) || ((blob3maxY-blob5maxY) < `CONT_RANGE)) begin
					
					if( ((blob5minX-blob3minX) < `CONT_RANGE) || ((blob3minX-blob5minX) < `CONT_RANGE) ||
						((blob5minX-blob3maxX) < `CONT_RANGE) || ((blob3maxX-blob5minX) < `CONT_RANGE) ||
						((blob5maxX-blob3minX) < `CONT_RANGE) || ((blob3minX-blob5maxX) < `CONT_RANGE) ||
						((blob5maxX-blob3maxX) < `CONT_RANGE) || ((blob3maxX-blob5maxX) < `CONT_RANGE)) begin
						ContainerAdjacentResult[8]<=`TRUE;				
					end
				end
			end	
			if(blob6empty==`FALSE && blob3empty==`FALSE) begin
				if( ((blob6minY-blob3minY) < `CONT_RANGE) || ((blob3minY-blob6minY) < `CONT_RANGE) ||
					((blob6minY-blob3maxY) < `CONT_RANGE) || ((blob3maxY-blob6minY) < `CONT_RANGE) ||
					((blob6maxY-blob3minY) < `CONT_RANGE) || ((blob3minY-blob6maxY) < `CONT_RANGE) ||
					((blob6maxY-blob3maxY) < `CONT_RANGE) || ((blob3maxY-blob6maxY) < `CONT_RANGE)) begin
					
					if( ((blob6minX-blob3minX) < `CONT_RANGE) || ((blob3minX-blob6minX) < `CONT_RANGE) ||
						((blob6minX-blob3maxX) < `CONT_RANGE) || ((blob3maxX-blob6minX) < `CONT_RANGE) ||
						((blob6maxX-blob3minX) < `CONT_RANGE) || ((blob3minX-blob6maxX) < `CONT_RANGE) ||
						((blob6maxX-blob3maxX) < `CONT_RANGE) || ((blob3maxX-blob6maxX) < `CONT_RANGE)) begin
						ContainerAdjacentResult[12]<=`TRUE;				
					end
				end
			end	
			
//MERGE CONTAINER 4
			if(blob5empty==`FALSE && blob4empty==`FALSE) begin
				if( ((blob4minY-blob5minY) < `CONT_RANGE) || ((blob5minY-blob4minY) < `CONT_RANGE) ||
					((blob4minY-blob5maxY) < `CONT_RANGE) || ((blob5maxY-blob4minY) < `CONT_RANGE) ||
					((blob4maxY-blob5minY) < `CONT_RANGE) || ((blob5minY-blob4maxY) < `CONT_RANGE) ||
					((blob4maxY-blob5maxY) < `CONT_RANGE) || ((blob5maxY-blob4maxY) < `CONT_RANGE)) begin
					
					if( ((blob4minX-blob5minX) < `CONT_RANGE) || ((blob5minX-blob4minX) < `CONT_RANGE) ||
						((blob4minX-blob5maxX) < `CONT_RANGE) || ((blob5maxX-blob4minX) < `CONT_RANGE) ||
						((blob4maxX-blob5minX) < `CONT_RANGE) || ((blob5minX-blob4maxX) < `CONT_RANGE) ||
						((blob4maxX-blob5maxX) < `CONT_RANGE) || ((blob5maxX-blob4maxX) < `CONT_RANGE)) begin
						ContainerAdjacentResult[9]<=`TRUE;				
					end
				end
			end	
			if(blob6empty==`FALSE && blob4empty==`FALSE) begin
				if( ((blob4minY-blob6minY) < `CONT_RANGE) || ((blob6minY-blob4minY) < `CONT_RANGE) ||
					((blob4minY-blob6maxY) < `CONT_RANGE) || ((blob6maxY-blob4minY) < `CONT_RANGE) ||
					((blob4maxY-blob6minY) < `CONT_RANGE) || ((blob6minY-blob4maxY) < `CONT_RANGE) ||
					((blob4maxY-blob6maxY) < `CONT_RANGE) || ((blob6maxY-blob4maxY) < `CONT_RANGE)) begin
					
					if( ((blob4minX-blob6minX) < `CONT_RANGE) || ((blob6minX-blob4minX) < `CONT_RANGE) ||
						((blob4minX-blob6maxX) < `CONT_RANGE) || ((blob6maxX-blob4minX) < `CONT_RANGE) ||
						((blob4maxX-blob6minX) < `CONT_RANGE) || ((blob6minX-blob4maxX) < `CONT_RANGE) ||
						((blob4maxX-blob6maxX) < `CONT_RANGE) || ((blob6maxX-blob4maxX) < `CONT_RANGE)) begin
						ContainerAdjacentResult[13]<=`TRUE;				
					end
				end
			end	
//MERGE CONTAINER 5
			if(blob6empty==`FALSE && blob5empty==`FALSE) begin
				if( ((blob5minY-blob6minY) < `CONT_RANGE) || ((blob6minY-blob5minY) < `CONT_RANGE) ||
					((blob5minY-blob6maxY) < `CONT_RANGE) || ((blob6maxY-blob5minY) < `CONT_RANGE) ||
					((blob5maxY-blob6minY) < `CONT_RANGE) || ((blob6minY-blob5maxY) < `CONT_RANGE) ||
					((blob5maxY-blob6maxY) < `CONT_RANGE) || ((blob6maxY-blob5maxY) < `CONT_RANGE)) begin
					
					if( ((blob5minX-blob6minX) < `CONT_RANGE) || ((blob6minX-blob5minX) < `CONT_RANGE) ||
						((blob5minX-blob6maxX) < `CONT_RANGE) || ((blob6maxX-blob5minX) < `CONT_RANGE) ||
						((blob5maxX-blob6minX) < `CONT_RANGE) || ((blob6minX-blob5maxX) < `CONT_RANGE) ||
						((blob5maxX-blob6maxX) < `CONT_RANGE) || ((blob6maxX-blob5maxX) < `CONT_RANGE)) begin
						ContainerAdjacentResult[14]<=`TRUE;				
					end
				end
			end	
															
			state<= `MERGE_CONTAINER;
		end
		
		`MERGE_CONTAINER: begin
			if(ContainerAdjacentResult>0) begin		
			
				if(ContainerAdjacentResult[14]==1'b1) begin
					state<= `MERGE_CONT_5_6;
				end		
				else if(ContainerAdjacentResult[13]==1'b1) begin
					state<= `MERGE_CONT_4_6;
				end		
				else if(ContainerAdjacentResult[9]==1'b1) begin
					state<= `MERGE_CONT_4_5;
				end
				else if(ContainerAdjacentResult[12]==1'b1) begin
					state<= `MERGE_CONT_3_6;
				end				
				else if(ContainerAdjacentResult[8]==1'b1) begin
					state<= `MERGE_CONT_3_5;
				end
				else if(ContainerAdjacentResult[7]==1'b1) begin
					state<= `MERGE_CONT_3_4;
				end					
				else if(ContainerAdjacentResult[11]==1'b1) begin
					state<= `MERGE_CONT_2_6;
				end				
				else if(ContainerAdjacentResult[6]==1'b1) begin
					state<= `MERGE_CONT_2_5;
				end
				else if(ContainerAdjacentResult[5]==1'b1) begin
					state<= `MERGE_CONT_2_4;
				end
				else if(ContainerAdjacentResult[4]==1'b1) begin
					state<= `MERGE_CONT_2_3;
				end			
				else if(ContainerAdjacentResult[10]==1'b1) begin
					state<= `MERGE_CONT_1_6;
				end				
				else if(ContainerAdjacentResult[3]==1'b1) begin
					state<= `MERGE_CONT_1_5;
				end
				else if(ContainerAdjacentResult[2]==1'b1) begin
					state<= `MERGE_CONT_1_4;
				end
				else if(ContainerAdjacentResult[1]==1'b1) begin
					state<= `MERGE_CONT_1_3;
				end
				else if(ContainerAdjacentResult[0]==1'b1) begin
					state<= `MERGE_CONT_1_2;
				end
				
				
			end
			else begin											
				state<=`IDLE;		
			end
		end
		
		`MERGE_CONT_1_2: begin
			if(blob2maxX > blob1maxX) blob1maxX <= blob2maxX;
			if(blob2maxY > blob1maxY) blob1maxY <= blob2maxY; 
			if(blob2minX < blob1minX) blob1minX <= blob2minX;
			if(blob2minY < blob1minY) blob1minY <= blob2minY;
			ContainerAdjacentResult[0]<=`FALSE;
			blob2empty<=`TRUE;
						
			state<= `MERGE_CONTAINER;
		end

		`MERGE_CONT_1_3: begin
			if(blob3maxX > blob1maxX) blob1maxX <= blob3maxX;
			if(blob3maxY > blob1maxY) blob1maxY <= blob3maxY; 
			if(blob3minX < blob1minX) blob1minX <= blob3minX;
			if(blob3minY < blob1minY) blob1minY <= blob3minY;
			ContainerAdjacentResult[1]<=`FALSE;
			blob3empty<=`TRUE;
			
			state<= `MERGE_CONTAINER;
		end
		
		`MERGE_CONT_1_4: begin
			if(blob4maxX > blob1maxX) blob1maxX <= blob4maxX;
			if(blob4maxY > blob1maxY) blob1maxY <= blob4maxY; 
			if(blob4minX < blob1minX) blob1minX <= blob4minX;
			if(blob4minY < blob1minY) blob1minY <= blob4minY;
			ContainerAdjacentResult[2]<=`FALSE;
			blob4empty<=`TRUE;

			state<= `MERGE_CONTAINER;
		end
		
		`MERGE_CONT_1_5: begin
			if(blob5maxX > blob1maxX) blob1maxX <= blob5maxX;
			if(blob5maxY > blob1maxY) blob1maxY <= blob5maxY; 
			if(blob5minX < blob1minX) blob1minX <= blob5minX;
			if(blob5minY < blob1minY) blob1minY <= blob5minY;
			ContainerAdjacentResult[3]<=`FALSE;
			blob5empty<=`TRUE;
			
			state<= `MERGE_CONTAINER;
		end

		`MERGE_CONT_1_6: begin
			if(blob6maxX > blob1maxX) blob1maxX <= blob6maxX;
			if(blob6maxY > blob1maxY) blob1maxY <= blob6maxY; 
			if(blob6minX < blob1minX) blob1minX <= blob6minX;
			if(blob6minY < blob1minY) blob1minY <= blob6minY;
			ContainerAdjacentResult[10]<=`FALSE;
			blob6empty<=`TRUE;
						
			state<= `MERGE_CONTAINER;
		end
		
		`MERGE_CONT_2_3: begin
			if(blob3maxX > blob2maxX) blob2maxX <= blob3maxX;
			if(blob3maxY > blob2maxY) blob2maxY <= blob3maxY; 
			if(blob3minX < blob2minX) blob2minX <= blob3minX;
			if(blob3minY < blob2minY) blob2minY <= blob3minY;
			ContainerAdjacentResult[4]<=`FALSE;
			blob3empty<=`TRUE;
			
			state<= `MERGE_CONTAINER;
		end
		
		`MERGE_CONT_2_4: begin
			if(blob4maxX > blob2maxX) blob2maxX <= blob4maxX;
			if(blob4maxY > blob2maxY) blob2maxY <= blob4maxY; 
			if(blob4minX < blob2minX) blob2minX <= blob4minX;
			if(blob4minY < blob2minY) blob2minY <= blob4minY;
			ContainerAdjacentResult[5]<=`FALSE;
			blob4empty<=`TRUE;
			
			state<= `MERGE_CONTAINER;
		end
		
		`MERGE_CONT_2_5: begin
			if(blob5maxX > blob2maxX) blob2maxX <= blob5maxX;
			if(blob5maxY > blob2maxY) blob2maxY <= blob5maxY; 
			if(blob5minX < blob2minX) blob2minX <= blob5minX;
			if(blob5minY < blob2minY) blob2minY <= blob5minY;
			ContainerAdjacentResult[6]<=`FALSE;
			blob5empty<=`TRUE;

			state<= `MERGE_CONTAINER;
		end
		
		`MERGE_CONT_2_6: begin
			if(blob6maxX > blob2maxX) blob2maxX <= blob6maxX;
			if(blob6maxY > blob2maxY) blob2maxY <= blob6maxY; 
			if(blob6minX < blob2minX) blob2minX <= blob6minX;
			if(blob6minY < blob2minY) blob2minY <= blob6minY;
			ContainerAdjacentResult[11]<=`FALSE;
			blob6empty<=`TRUE;
			
			state<= `MERGE_CONTAINER;
		end		

		`MERGE_CONT_3_4: begin
			if(blob4maxX > blob3maxX) blob3maxX <= blob4maxX;
			if(blob4maxY > blob3maxY) blob3maxY <= blob4maxY; 
			if(blob4minX < blob3minX) blob3minX <= blob4minX;
			if(blob4minY < blob3minY) blob3minY <= blob4minY;
			ContainerAdjacentResult[7]<=`FALSE;
			blob4empty<=`TRUE;

			state<= `MERGE_CONTAINER;
		end
		
		`MERGE_CONT_3_5: begin
			if(blob5maxX > blob3maxX) blob3maxX <= blob5maxX;
			if(blob5maxY > blob3maxY) blob3maxY <= blob5maxY; 
			if(blob5minX < blob3minX) blob3minX <= blob5minX;
			if(blob5minY < blob3minY) blob3minY <= blob5minY;
			ContainerAdjacentResult[8]<=`FALSE;
			blob5empty<=`TRUE;

			state<= `MERGE_CONTAINER;
		end
		
		`MERGE_CONT_3_6: begin
			if(blob6maxX > blob3maxX) blob3maxX <= blob6maxX;
			if(blob6maxY > blob3maxY) blob3maxY <= blob6maxY; 
			if(blob6minX < blob3minX) blob3minX <= blob6minX;
			if(blob6minY < blob3minY) blob3minY <= blob6minY;
			ContainerAdjacentResult[12]<=`FALSE;
			blob6empty<=`TRUE;

			state<= `MERGE_CONTAINER;
		end		

		`MERGE_CONT_4_5: begin
			if(blob5maxX > blob4maxX) blob4maxX <= blob5maxX;
			if(blob5maxY > blob4maxY) blob4maxY <= blob5maxY; 
			if(blob5minX < blob4minX) blob4minX <= blob5minX;
			if(blob5minY < blob4minY) blob4minY <= blob5minY;
			ContainerAdjacentResult[9]<=`FALSE;
			blob5empty<=`TRUE;

			state<= `MERGE_CONTAINER;
		end		
		
		`MERGE_CONT_4_6: begin
			if(blob6maxX > blob4maxX) blob4maxX <= blob6maxX;
			if(blob6maxY > blob4maxY) blob4maxY <= blob6maxY; 
			if(blob6minX < blob4minX) blob4minX <= blob6minX;
			if(blob6minY < blob4minY) blob4minY <= blob6minY;
			ContainerAdjacentResult[13]<=`FALSE;
			blob6empty<=`TRUE;
			
			state<= `MERGE_CONTAINER;
		end	
		
		`MERGE_CONT_5_6: begin
			if(blob6maxX > blob5maxX) blob5maxX <= blob6maxX;
			if(blob6maxY > blob5maxY) blob5maxY <= blob6maxY; 
			if(blob6minX < blob5minX) blob5minX <= blob6minX;
			if(blob6minY < blob5minY) blob5minY <= blob6minY;
			ContainerAdjacentResult[14]<=`FALSE;
			blob6empty<=`TRUE;

			state<= `MERGE_CONTAINER;
		end	
												
		endcase
	end
end


//perform center of mass computation
//delay of 1 clock tick
//CenterOfMass BlobCenterComputation1(
//						.iCLK(iCOMclock), 
//						.iEnable(enableCOMcomputation), 
//						.inSumPixels(sumBLOB_Pixels_1),
//						.inSumPosX(sumBLOB_Xpositions_1), 
//						.inSumPosY(sumBLOB_Ypositions_1), 
//						.outBlobXcenter(blob1X_com_center), 
//						.outBlobYcenter(blob1Y_com_center));
//						
////perform center of mass computation
////delay of 1 clock tick
//CenterOfMass BlobCenterComputation2(
//						.iCLK(iCOMclock), 
//						.iEnable(enableCOMcomputation), 
//						.inSumPixels(sumBLOB_Pixels_2),
//						.inSumPosX(sumBLOB_Xpositions_2), 
//						.inSumPosY(sumBLOB_Ypositions_2), 
//						.outBlobXcenter(blob2X_com_center), 
//						.outBlobYcenter(blob2Y_com_center));
//						
////perform center of mass computation
////delay of 1 clock tick
//CenterOfMass BlobCenterComputation3(
//						.iCLK(iCOMclock), 
//						.iEnable(enableCOMcomputation), 
//						.inSumPixels(sumBLOB_Pixels_3),
//						.inSumPosX(sumBLOB_Xpositions_3), 
//						.inSumPosY(sumBLOB_Ypositions_3), 
//						.outBlobXcenter(blob3X_com_center), 
//						.outBlobYcenter(blob3Y_com_center));
//
////perform center of mass computation
////delay of 1 clock tick
//CenterOfMass BlobCenterComputation4(
//						.iCLK(iCOMclock), 
//						.iEnable(enableCOMcomputation), 
//						.inSumPixels(sumBLOB_Pixels_4),
//						.inSumPosX(sumBLOB_Xpositions_4), 
//						.inSumPosY(sumBLOB_Ypositions_4), 
//						.outBlobXcenter(blob4X_com_center), 
//						.outBlobYcenter(blob4Y_com_center));
//						
////perform center of mass computation
////delay of 1 clock tick
//CenterOfMass BlobCenterComputation5(
//						.iCLK(iCOMclock), 
//						.iEnable(enableCOMcomputation), 
//						.inSumPixels(sumBLOB_Pixels_5),
//						.inSumPosX(sumBLOB_Xpositions_5), 
//						.inSumPosY(sumBLOB_Ypositions_5), 
//						.outBlobXcenter(blob5X_com_center), 
//						.outBlobYcenter(blob5Y_com_center));
//						
//						
////perform center of mass computation
////delay of 1 clock tick
//CenterOfMass BlobCenterComputation6(
//						.iCLK(iCOMclock), 
//						.iEnable(enableCOMcomputation), 
//						.inSumPixels(sumBLOB_Pixels_6),
//						.inSumPosX(sumBLOB_Xpositions_6), 
//						.inSumPosY(sumBLOB_Ypositions_6), 
//						.outBlobXcenter(blob6X_com_center), 
//						.outBlobYcenter(blob6Y_com_center));

endmodule


module divider(//clk, 
	opa, opb, quo, rem 
	//testy , testy2, testy_diff, dividend_test
	);   
   input  [10:0]  opa;
   input  [3:0]  opb;
   output [10:0]  quo, rem;
   //input         clk;
//output [49:0] testy;
//output [49:0] testy2;
//output [49:0] testy_diff;
//output [49:0] dividend_test;


//assign testy_diff = diff27;
//assign testy = quotient26;
//assign testy2 = divider_copy26;
//assign dividend_test = dividend_copy26;



   reg [10:0] quo, rem;

//  """"""""|
//     1011 |  <----  dividend_copy
// -0011    |  <----  divider_copy
//  """"""""|    0  Difference is negative: copy dividend and put 0 in quotient.
//     1011 |  <----  dividend_copy
//  -0011   |  <----  divider_copy
//  """"""""|   00  Difference is negative: copy dividend and put 0 in quotient.
//     1011 |  <----  dividend_copy
//   -0011  |  <----  divider_copy
//  """"""""|  001  Difference is positive: use difference and put 1 in quotient.
//            quotient (numbers above)   

   reg [10:0]    quotient0;
   reg [10:0]    dividend_copy0, diff0;
   reg [10:0]    divider_copy0;
   wire [10:0]   remainder0;

   reg [10:0]    quotient1;
   reg [10:0]    dividend_copy1, diff1;
   reg [10:0]    divider_copy1;
   wire [10:0]   remainder1;
 
   reg [10:0]    quotient2;
   reg [10:0]    dividend_copy2, diff2;
   reg [10:0]    divider_copy2;
   wire [10:0]   remainder2;
   
   reg [10:0]    quotient3;
   reg [10:0]    dividend_copy3, diff3;
   reg [10:0]    divider_copy3;
   wire [10:0]   remainder3;
   
   reg [10:0]    quotient4;
   reg [10:0]    dividend_copy4, diff4;
   reg [10:0]    divider_copy4;
   wire [10:0]   remainder4;
   
   reg [10:0]    quotient5;
   reg [10:0]    dividend_copy5, diff5;
   reg [10:0]    divider_copy5;
   wire [10:0]   remainder5;
   
   reg [10:0]    quotient6;
   reg [10:0]    dividend_copy6, diff6;
   reg [10:0]    divider_copy6;
   wire [10:0]   remainder6;
   
   reg [10:0]    quotient7;
   reg [10:0]    dividend_copy7, diff7;
   reg [10:0]    divider_copy7;
   wire [10:0]   remainder7;
   
   reg [10:0]    quotient8;
   reg [10:0]    dividend_copy8, diff8;
   reg [10:0]    divider_copy8;
   wire [10:0]   remainder8;
   
always @ (opa or opb)
begin
//stage initial
 quotient0 = 11'b00000000000;
 dividend_copy0 = opa;
 divider_copy0 = {opb,7'b0000000};
 
 //stage1
 diff1 = dividend_copy0 - divider_copy0;
 quotient1 [10:1] = quotient0[9:0] ;
 if (!diff1[10]) // if diff1[10] == 0 (diff is positive, use difference )
 begin
 dividend_copy1 = diff1;
 quotient1[0] = 1'b1;
 end
else   // diff was negative, use old dividend
begin
dividend_copy1 = dividend_copy0;
 quotient1[0] = 1'b0;

end
 divider_copy1 = (divider_copy0 >> 1);
//stage2
 diff2 = dividend_copy1 - divider_copy1;
 quotient2[10:1]  = quotient1 [9:0] ;
 if (!diff2[10])
 begin
 dividend_copy2 = diff2;
 quotient2[0] = 1'b1;
 end
 else
begin
dividend_copy2 = dividend_copy1;
 quotient2[0] = 1'b0;

end
 divider_copy2 = divider_copy1 >> 1;
 
 //stage3
 diff3 = dividend_copy2 - divider_copy2;
 quotient3[10:1]  = quotient2 [9:0] ;
 if (!diff3[10])
 begin
 dividend_copy3 = diff3;
 quotient3[0] = 1'b1;
 end
 else
begin
dividend_copy3 = dividend_copy2;
 quotient3[0] = 1'b0;

end
 divider_copy3 = divider_copy2 >> 1;
 
 //stage4
 diff4 = dividend_copy3 - divider_copy3;
 quotient4[10:1]  = quotient3 [9:0] ;
 if (!diff4[10])
 begin
 dividend_copy4 = diff4;
 quotient4[0] = 1'b1;
 end
 else
begin
dividend_copy4 = dividend_copy3;
 quotient4[0] = 1'b0;

end
 divider_copy4 = divider_copy3 >> 1;
  //stage5
 diff5 = dividend_copy4 - divider_copy4;
 quotient5[10:1]  = quotient4 [9:0] ;
 if (!diff5[10])
 begin
 dividend_copy5 = diff5;
 quotient5[0] = 1'b1;
 end
 else
begin
dividend_copy5 = dividend_copy4;
 quotient5[0] = 1'b0;

end
 divider_copy5 = divider_copy4 >> 1;
  //stage6
 diff6 = dividend_copy5 - divider_copy5;
 quotient6[10:1]  = quotient5 [9:0] ;
 if (!diff6[10])
 begin
 dividend_copy6 = diff6;
 quotient6[0] = 1'b1;
 end
 else
begin
dividend_copy6 = dividend_copy5;
 quotient6[0] = 1'b0;

end
 divider_copy6 = divider_copy5>> 1;
 
  //stage7
 diff7 = dividend_copy6 - divider_copy6;
 quotient7[10:1]  = quotient6 [9:0] ;
 if (!diff7[10])
 begin
 dividend_copy7 = diff7;
 quotient7[0] = 1'b1;
 end
 else
begin
dividend_copy7 = dividend_copy6;
 quotient7[0] = 1'b0;

end
 divider_copy7 = divider_copy6>> 1;
  //stage8
 diff8 = dividend_copy7 - divider_copy7;
 quotient8[10:1]  = quotient7 [9:0] ;
 if (!diff8[10])
 begin
 dividend_copy8 = diff8;
 quotient8[0] = 1'b1;
 end
 else
begin
dividend_copy8 = dividend_copy7;
 quotient8[0] = 1'b0;

end
 divider_copy8 = divider_copy7>> 1;
 
quo = quotient8;
rem =  dividend_copy8;

end

   //integer i;

   /*
always @(opa,opb) 
begin
    for (i=-1; i<8; i=i+1) 
begin
if (i==-1) 
begin
            // initialization
quotient = 10'd0;
dividend_copy = opa;
divider_copy = {opb,7'd0};
end 
else 
begin
diff = dividend_copy - divider_copy;
quotient = quotient ;

if( !diff[10] ) 
begin
dividend_copy = diff;
quotient[0] = 1'd1;
end
divider_copy = divider_copy >> 1;
end
end
end
*/

endmodule

