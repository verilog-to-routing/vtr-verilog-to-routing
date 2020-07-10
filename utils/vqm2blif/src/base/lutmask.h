#ifndef LUTMASK_H
#define LUTMASK_H

#include "vqm2blif_util.h"

typedef vector< bitset<6> > tt_vec;

typedef enum lut_print_mode { BLIF_PRINT, VERBOSE_PRINT, NO_LPMODE } lp_mode;

//============================================================================================
//============================================================================================

class BlifLut {

	string lutmask;

	tt_vec truth_table;

	bitset <6> col_mask;

	string inports[6];
	string outport;

public:

	string inst_name;	//optional instantiation name, for file readability and debug information

	//	*****CONSTRUCTORS*****
	// - Instantiates the LUT based on input data. 
	BlifLut ();	//default constructor, all variables remain empty and must be set
	BlifLut ( const char* );	//LUTmask-only constructor, ports must be set

	BlifLut ( const char*, const char*, const char*, const char*, 
					const char*, const char*, const char*, const char*);
	// complete constructor, takes in LUTmask and port information
	// and sets the truth table and column mask appropriately.

	void set_lutmask ( const char* );	//sets the lutmask and elaborates the truth table
	void set_col_mask ( const char* );	//sets the column mask to a specified const char*
	void set_inport ( int, const char* );	//assigns an inport name and sets the column mask
	void set_outport ( const char* );	//assigns the outport name
	void reset_ports();	//empties all port names and resets the column mask

	string get_inport( int );
	string get_outport();

	//	*****PRINT FUNCTIONS*****
	// - Each print the LUT data in to a different source in a specified format.
	void print ( lp_mode, ostream&, t_boolean eblif_format );		//passed a file to print the data to.
};

//============================================================================================
//============================================================================================

BlifLut::BlifLut() {

	lutmask.clear();	//empty the lutmask
	truth_table.clear();	//empty the truth table
	reset_ports();	//reset all ports names and the column mask

}
//============================================================================================
//============================================================================================

BlifLut::BlifLut ( const char* lmask ){
/*	LUTmask-Only Constructor 
 *	- Specifies the truth table of the LUT upon instantiation.
 *	- Port names (and thus the Column Mask) must be set using set_inport() and set_outport()
 *  
 *	ARGUMENTS
 *  lmask:
 *	LUTmask string to be evaluated into a truth table for the LUT.
 */

	set_lutmask ( lmask );
	reset_ports ();

}

//============================================================================================
//============================================================================================

BlifLut::BlifLut ( const char* lmask, const char* a, const char* b, const char* c, const char* d, 
									const char* e, const char* f, const char* out){
/*	LUTmask and Port Constructor 
 *	- Specifies the truth table, port names, and which ports are used in the LUT upon instantiation.
 *  
 *	ARGUMENTS
 *  lmask:
 *	LUTmask string to be evaluated into a truth table for the LUT.
 *  a, b, c, d, e, f:
 *	Inport names. If a port is to be left unused, pass "". The column mask is updated accordingly.
 */

	set_lutmask ( lmask );

	//NOTE: At least two inports must be set before printing.
	if (strlen(a) > 0){
		set_inport( 5, a );
	}
	if (strlen(b) > 0){
		set_inport( 4, b );
	}
	if (strlen(c) > 0){
		set_inport( 3, c );
	}
	if (strlen(d) > 0){
		set_inport( 2, d );
	}
	if (strlen(e) > 0){
		set_inport( 1, e );
	}
	if (strlen(f) > 0){
		set_inport( 0, f );
	}

	//NOTE: The outport is the only mandatory port. If left open, it must be set using set_outport().
	if (strlen(out) > 0){
		set_outport( out );
	}

}

//============================================================================================
//============================================================================================

void BlifLut::set_lutmask ( const char* lmask ){
/*	Set LUTmask Function
 *	- This function elaborates a lutmask string and fills the private member "truth_table" 
 *	  appropriately. 
 *
 *	ARGUMENTS:
 *  lmask
 *	A 16-character string of hexidecimal numbers detailing the truth table of a LUT.
 */

	VTR_ASSERT(strlen(lmask) == 16);	//Currently, only 6-input LUTs are supported.
	//6-input LUT => 64 rows in the truth table (2^6) => 16 hex chars in the lutmask (64/4)

	lutmask = lmask;
	truth_table.clear();

	char byte;
	stringstream ss;	//used to convert hexadecimal strings to decimal integers
	int a;

	bitset<4> bin_char;	//the string is read one hex character (4 truth table rows) at a time
	bitset<6> tt_row;		//each row has 6 inputs
	
	for (unsigned int i = lutmask.length() ; i > 0 ; i--){

		byte = lutmask.at(i - 1);	//grabs the next-least significant hex character

		ss << hex << byte;	//converts the hex character...
		ss >> a;	//...into an integer (decimal) value

		//NOTE: a is stored as a [sizeof(int)]-bit binary number in memory!

		bin_char = a;	//assigns the bitstring to the least-significant 4 bits of a
		
		for (int j = 0; j < 4; j++){
			//cycle through the bits in the byte, testing each one
			if (bin_char.test(j)){
				//convert the corresponding truth table row number to binary
				tt_row = 4*(lutmask.length() - i) + j; 
				truth_table.push_back(tt_row);
			}
		}

		ss.clear();	//clear the stringstream to load another character
	} 
}

//============================================================================================
//============================================================================================

void BlifLut::set_col_mask ( const char* cmask ){

	VTR_ASSERT(strlen(cmask) == 6);	//Currently, only 6-input LUTs are supported.
	for (int i = 0; i < 6; i++){
		if (cmask[i] == '1'){
			col_mask[ 5-i ] = 1;
		} else if (cmask[i] == '0'){
			col_mask[ 5-i ] = 0;
		} else {
			cout << "ERROR: Invalid column mask " << cmask << endl ;
			exit(1);
		}
	}

}

//============================================================================================
//============================================================================================

void BlifLut::set_inport (int index, const char* name){

	//NOTE: Ports are printed as ".names [5] [4] [3] [2] [1] [0] out"
	VTR_ASSERT((index >= 0)&&(index <= 5));
	VTR_ASSERT(strlen(name) > 0);
	inports[index] = name;	//assign the name of the port
	col_mask.set(index);	//indicate in the column mask that it's been used

}

//============================================================================================
//============================================================================================

void BlifLut::set_outport (const char* name){

	//NOTE: Ports are printed as ".names [5] [4] [3] [2] [1] [0] out"
	VTR_ASSERT(strlen(name) > 0);
	outport = name;

}

//============================================================================================
//============================================================================================

void BlifLut::reset_ports(){
	for (int i = 0; i < 6; i++ ){
		inports[i].clear();	//empty all the inport names
	}
	outport.clear();	//empty the outport name
	col_mask.reset();	//set the column mask to "000000"
}

//============================================================================================
//============================================================================================

string BlifLut::get_inport (int index){
	if (inports[index].length() > 0)
		return inports[index];
	return "";
}

//============================================================================================
//============================================================================================

string BlifLut::get_outport (){
	if (outport.length() > 0)
		return outport;
	return "";
}

//============================================================================================
//============================================================================================
void BlifLut::print (lp_mode print_mode, ostream& out_file, t_boolean eblif_format){
    // Print LUT to a pre-opened file indicated by out_file.

	//assert the LUT has inputs, a lutmask, and a truth table established
	VTR_ASSERT(col_mask.any());
	VTR_ASSERT(!lutmask.empty());
	VTR_ASSERT(!truth_table.empty());

	VTR_ASSERT((print_mode == VERBOSE_PRINT)||(print_mode == BLIF_PRINT));

    if (print_mode == VERBOSE_PRINT){
        //Print verbose introduction
        if (!inst_name.empty()){
            out_file << "Name: " << inst_name << endl;
        }
        out_file << "Lutmask: " << lutmask << endl ;
        out_file << "Colmask: " << col_mask << endl ;
        out_file << "Ports: ";
    } else {
        //Print blif introduction
        if (!inst_name.empty() && !eblif_format){
            out_file << "\n# " << inst_name << endl;
        }
        out_file << ".names ";
    }
    
    //Print port names
    for (int i = 5; i >= 0; i--){
        //only print used ports
        if (col_mask.test(i)){
            if (!inports[i].empty()){
                out_file << inports[i] << " ";
            } else {
                cout << "ERROR: LUT Input used and not named." << endl;
                exit(1);
            }
        }
    }

    //Every LUT must have an output!
    VTR_ASSERT(!outport.empty());
    out_file << outport << endl;

    //Print the truth table
    for(tt_vec::iterator it = truth_table.begin(); it != truth_table.end(); it++){
        //Each entry in ttvec is a row of the truth table
        for (int i = 5; i >= 0; i--){
            //Only print the columns of used inputs
            if (col_mask.test(i)){
                out_file << it->test(i);	//outputs 1 or 0 depending on the bit test.
            }
        }
        out_file << " 1" << endl ;
    }
    if (!inst_name.empty() && eblif_format){
        out_file << ".cname " << inst_name << "\n" << endl;
    }
}

//============================================================================================
//============================================================================================
typedef vector<BlifLut> lutvec;

#endif
