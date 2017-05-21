/* 
* Copyright (C) 2002-2015 XimpleWare, info@ximpleware.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
/*VTD-XML is protected by US patent 7133857, 7260652, an 7761459*/
#include "indexHandler.h"
static Long reverseLong(Long l);
static int reverseInt(int i);

#define OFFSET_ADJUSTMENT 32

static Long adjust(Long l,int i);
static Long reverseLong(Long l){
	Long t = (((l & -0x0100000000000000L) >> 56) & 0xffL)
		| ((l & 0xff000000000000L) >> 40) 
		| ((l & 0xff0000000000L) >> 24) 
		| ((l & 0xff00000000L) >> 8) 
		| ((l & 0xff000000L) << 8) 
		| ((l & 0xff0000L) << 24) 
		| ((l & 0xff00L) << 40) 
		| ((l & 0xffL) << 56);
	return t;
}

static int reverseInt(int i){
	int t = (((i & -0x01000000) >> 24) & 0xff) 
		| ((i & 0xff0000) >> 8) 
		| ((i & 0xff00) << 8) 
		| ((i & 0xff) << 24);
	return t;
}

Boolean isLittleEndian(){
	int a = 0xaabbccdd;
	if ( ((Byte *)(&a))[0] == (Byte)0xdd)
		return TRUE;
	return FALSE;
}
/*writeIndex writes VTD+XML into a file
  This function throws index_write_exception*/
Boolean _writeIndex_L3(Byte version, 
				int encodingType, 
				Boolean ns, 
				Boolean byteOrder, 
				int nestDepth, 
				int LCLevel, 
				int rootIndex, 
				UByte* xmlDoc, 
				int docOffset, 
				int docLen, 
				FastLongBuffer* vtdBuffer, 
				FastLongBuffer* l1Buffer, 
				FastLongBuffer* l2Buffer, 
				FastIntBuffer* l3Buffer, 
				FILE *f){
					int i;
					Byte ba[4];
					Long l;
					Boolean littleEndian = isLittleEndian();
			if (xmlDoc == NULL 
				|| docLen <= 0 
				|| vtdBuffer == NULL
				|| l1Buffer == NULL 
				|| l2Buffer == NULL 
				|| l3Buffer == NULL
				|| f == NULL)
			{
				throwException2(invalid_argument, "writeIndex's argument invalid");
				return FALSE;	
			}

			if (vtdBuffer->size == 0){
				throwException2(index_write_exception,"vTDBuffer can't be zero in size");
			}
			
			//UPGRADE_TODO: Class 'java.io.DataOutputStream' was converted to 'System.IO.BinaryWriter' which has a different behavior. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1073_javaioDataOutputStream'"
			//System.IO.BinaryWriter dos = new System.IO.BinaryWriter(os);
			// first 4 bytes
			
			ba[0] = (Byte) version; // version # is 1 
			ba[1] = (Byte) encodingType;
            if (littleEndian == FALSE)
                ba[2] = (Byte)(ns ? 0xe0 : 0xa0); // big endien
            else
                ba[2] = (Byte)(ns ? 0xc0 : 0x80);
			ba[3] = (Byte) nestDepth;
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			// second 4 bytes
			ba[0] = 0;
			ba[1] = 4;
			ba[2] = (Byte) ((rootIndex & 0xff00) >> 8);
			ba[3] = (Byte) (rootIndex & 0xff);
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			// 2 reserved 32-bit words set to zero
			ba[1] = ba[2] = ba[3] = 0;
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			// write XML doc in bytes	
			l = docLen;
			if (fwrite((UByte*) (&l), 1 , 8,f)!=8){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
            //dos.Write(xmlDoc, docOffset, docLen);
			if (fwrite((UByte*)(xmlDoc+docOffset), 1, docLen,f)!=docLen){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			//dos.Write(xmlDoc, docOffset, docLen);
			// zero padding to make it integer multiple of 64 bits
			if ((docLen & 0x07) != 0)
			{
				int t = (((docLen >> 3) + 1) << 3) - docLen;
				for (; t > 0; t--){
					if (fwrite(ba,1,1,f)!=1){
						throwException2(index_write_exception, "fwrite error occurred");
						return FALSE;
					};
				}
			}
			// write VTD
            
			//dos.Write((long)vtdBuffer.size());
			l = vtdBuffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < vtdBuffer->size; i++)
			{
				l = longAt(vtdBuffer,i);
				if (fwrite((UByte*) &l, 1 , 8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}
			// write L1 
			//dos.Write((long)l1Buffer.size());
			l = l1Buffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l1Buffer->size; i++)
			{
				l = longAt(l1Buffer,i);
				if (fwrite((UByte*) &l, 1 ,8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}
			// write L2
			l = l2Buffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l2Buffer->size; i++)
			{
				l = longAt(l2Buffer,i);
				if (fwrite((UByte*) &l, 1 , 8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}
			// write L3
			l = l3Buffer->size;
			if (fwrite((UByte*) &l, 1 , 8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l3Buffer->size; i++)
			{
				int s = intAt(l3Buffer,i);
				if (fwrite((UByte*)&s, 1 , 4, f)!=4){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}
			// pad zero if # of l3 entry is odd
			if ((l3Buffer->size & 1) != 0){
				if (fwrite(ba,1,1,f)!=1){
						throwException2(index_write_exception, "fwrite error occurred");
						return FALSE;
					};
			}
			//fclose(f);
			return TRUE;
}
/*readIndex loads VTD+XML into VTDGen
  this function throws index_read_exception*/
Boolean _readIndex(FILE *f, VTDGen *vg){
	int intLongSwitch;
	int endian;
	Byte ba[4];
	int LCLevel;
	Long l;
	int size;
	Byte *XMLDoc;
	Boolean littleEndian = isLittleEndian();
	if (f == NULL || vg == NULL)
	{
		throwException2(invalid_argument,"Invalid argument(s) for readIndex()");
		return FALSE;
	}
	
	// first byte
	if (fread(ba,1,1,f) != 1 ){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
    
	if (ba[0]!=1){
		throwException2(index_read_exception,"version # error");
		return FALSE;
	}
	// no check on version number for now
	// second byte
	if (fread(ba,1,1,f) != 1){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}

	vg->encoding = ba[0];
	
	// third byte
	if (fread(ba,1,1,f) != 1){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	if ((ba[0] & 0x80) != 0)
		intLongSwitch = 1;
	//use ints
	else
		intLongSwitch = 0;
	if ((ba[0] & 0x40) != 0)
		vg->ns = TRUE;
	else
		vg->ns = FALSE;
	if ((ba[0] & 0x20) != 0)
		endian = 1;
	else
		endian = 0;

	if ((ba[0] & 0x1f) != 0)
		throwException2(index_read_exception,"Last 5 bits of the third byte should be zero");
	// fourth byte
	if (fread(ba,1,1,f) != 1){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	vg->VTDDepth = ba[0];

	// 5th and 6th byte
	if (fread(ba,1,2,f) != 2){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	LCLevel = (((int) ba[0]) << 8) | ba[1];
	if (LCLevel != 4 && LCLevel !=6)
	{
		throwException2(index_read_exception,"LC levels must be at least 3");
		return FALSE;
	}
	if (LCLevel ==4)
		vg->shallowDepth = TRUE;
	else
		vg->shallowDepth = FALSE;
	// 7th and 8th byte
	if (fread(ba,1,2,f) != 2){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	vg->rootIndex = (((int) ba[0]) << 8) | ba[1];

	// skip a long
	if (fread(&l,1,8,f) != 8){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	//Console.WriteLine(" l ==>" + l);
	if (fread(&l,1,8,f) != 8){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	//Console.WriteLine(" l ==>" + l);
	if (fread(&l,1,8,f) != 8){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	//Console.WriteLine(" l ==>" + l);
	
	// read XML size
	if (littleEndian && (endian == 0)
		|| (littleEndian == FALSE) && (endian == 1))
		size = (int)l;
	else
		size = (int)reverseLong(l);


	// read XML bytes
	XMLDoc = malloc(size);
	if (XMLDoc == NULL){
		throwException2(out_of_mem,"Byte array allocation failed");
		return FALSE;
	}
	if (fread(XMLDoc,1,size,f)!= size){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	//dis.Read(XMLDoc,0,size);
	if ((size & 0x7) != 0)
	{
		int t = (((size >> 3) + 1) << 3) - size;
		while (t > 0)
		{
			if (fread(ba,1,1,f) != 1){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			t--;
		}
	}

	setDoc(vg,XMLDoc,size);

	if ( (littleEndian && (endian == 0))
		|| (littleEndian == FALSE && endian == 1))
	{
		int vtdSize,l1Size,l2Size,l3Size,l4Size,l5Size;
		// read vtd records
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		vtdSize = (int) l;
		while (vtdSize > 0)
		{
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			appendLong(vg->VTDBuffer ,l);
			vtdSize--;
		}
		// read L1 LC records
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		l1Size = (int) l;
		while (l1Size > 0)
		{
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			appendLong(vg->l1Buffer,l);
			l1Size--;
		}
		// read L2 LC records
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		l2Size = (int) l;
		while (l2Size > 0)
		{
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			appendLong(vg->l2Buffer, l);
			l2Size--;
		}
		// read L3 LC records
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		l3Size = (int) l;
		if (vg->shallowDepth){
			if (intLongSwitch == 1)
			{
				//l3 uses ints
				while (l3Size > 0)
				{
					if (fread(&size,1,4,f) != 4){
						throwException2(index_read_exception,"fread error occurred");
						return FALSE;
					}
					appendInt(vg->l3Buffer,size);
					l3Size--;
				}
			}
			else
			{
				while (l3Size > 0)
				{
					if (fread(&l,1,8,f) != 8){
						throwException2(index_read_exception,"fread error occurred");
						return FALSE;
					}
					appendLong((FastLongBuffer *)vg->l3Buffer,(int)(l >> 32));
					l3Size--;
				}
			}
		} else {
			while (l3Size > 0)
			{
				if (fread(&l,1,8,f) != 8){
					throwException2(index_read_exception,"fread error occurred");
					return FALSE;
				}
				appendLong(vg->_l3Buffer, l);
				l3Size--;
			}

			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			l4Size = (int) l;
			while (l4Size > 0)
			{
				if (fread(&l,1,8,f) != 8){
					throwException2(index_read_exception,"fread error occurred");
					return FALSE;
				}
				appendLong(vg->_l4Buffer, l);
				l4Size--;
			}
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			l5Size = (int) l;
			if (intLongSwitch == 1)
			{
				//l3 uses ints
				while (l5Size > 0)
				{
					if (fread(&size,1,4,f) != 4){
						throwException2(index_read_exception,"fread error occurred");
						return FALSE;
					}
					appendInt(vg->_l5Buffer,size);
					l5Size--;
				}
			}
			else
			{
				while (l5Size > 0)
				{
					if (fread(&l,1,8,f) != 8){
						throwException2(index_read_exception,"fread error occurred");
						return FALSE;
					}
					appendLong((FastLongBuffer *)vg->_l5Buffer,(int)(l >> 32));
					l5Size--;
				}
			}

		}
	}
	else
	{
		// read vtd records
		int vtdSize,l1Size,l2Size,l3Size,l4Size,l5Size;
		// read vtd records
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		vtdSize = (int) reverseLong(l);
		while (vtdSize > 0)
		{
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			appendLong(vg->VTDBuffer,reverseLong(l));
			vtdSize--;
		}
		// read L1 LC records
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		l1Size = (int) reverseLong(l);
		while (l1Size > 0)
		{
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			appendLong(vg->l1Buffer,reverseLong(l));
			l1Size--;
		}
		// read L2 LC records
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		l2Size = (int) reverseLong(l);
		while (l2Size > 0)
		{
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			appendLong(vg->l2Buffer, reverseLong(l));
			l2Size--;
		}
		// read L3 LC records
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		l3Size = (int) reverseLong(l);
		if (vg->shallowDepth){
			if (intLongSwitch == 1)
			{
				//l3 uses ints
				while (l3Size > 0)
				{
					if (fread(&size,1,4,f) != 4){
						throwException2(index_read_exception,"fread error occurred");
						return FALSE;
					}
					appendInt(vg->l3Buffer,reverseInt(size));
					l3Size--;
				}
			}
			else
			{
				while (l3Size > 0)
				{
					if (fread(&l,1,8,f) != 8){
						throwException2(index_read_exception,"fread error occurred");
						return FALSE;
					}
					appendInt(vg->l3Buffer,reverseInt((int) (l >> 32)));
					l3Size--;
				}
			}
		}else{
			while (l3Size > 0){
				if (fread(&l,1,8,f) != 8){
					throwException2(index_read_exception,"fread error occurred");
					return FALSE;
				}
				appendLong(vg->_l3Buffer, reverseLong(l));
				l3Size--;
			}
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			l4Size = (int) reverseLong(l);
			while (l4Size > 0)
			{
				if (fread(&l,1,8,f) != 8){
					throwException2(index_read_exception,"fread error occurred");
					return FALSE;
				}
				appendLong(vg->_l4Buffer, reverseLong(l));
				l4Size--;
			}
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			l5Size = (int) reverseLong(l);
			if (intLongSwitch == 1)
			{
				//l3 uses ints
				while (l5Size > 0)
				{
					if (fread(&size,1,4,f) != 4){
						throwException2(index_read_exception,"fread error occurred");
						return FALSE;
					}
					appendInt(vg->_l5Buffer,reverseInt(size));
					l5Size--;
				}
			}
			else
			{
				while (l5Size > 0)
				{
					if (fread(&l,1,8,f) != 8){
						throwException2(index_read_exception,"fread error occurred");
						return FALSE;
					}
					appendInt(vg->_l5Buffer,reverseInt((int) (l >> 32)));
					l5Size--;
				}
			}
		}
	}
	//fclose(f);
	return TRUE;
	
}

Boolean _readIndex2(UByte *ba, int len, VTDGen *vg){
	int intLongSwitch;
	int endian;
	int LCLevel;
	Long l;
	int size,adj;
	int count;
	int t=0;
	Boolean littleEndian = isLittleEndian();
	if (ba == NULL || vg == NULL)
	{
		throwException2(invalid_argument,"Invalid argument(s) for readIndex()");
		return FALSE;
	}
	
	// first byte
	if (len < 32){
		throwException2(index_read_exception,"Invalid Index error");
		return FALSE;
	}	

	vg->encoding = ba[1];
	adj = OFFSET_ADJUSTMENT;
	if (vg->encoding >= FORMAT_UTF_16BE){
		adj = OFFSET_ADJUSTMENT >> 1;
	}
	
	if ((ba[2] & 0x80) != 0)
		intLongSwitch = 1;
	//use ints
	else
		intLongSwitch = 0;
	if ((ba[2] & 0x40) != 0)
		vg->ns = TRUE;
	else
		vg->ns = FALSE;
	if ((ba[2] & 0x20) != 0)
		endian = 1;
	else
		endian = 0;
	if ((ba[0] & 0x1f) != 0)
		throwException2(index_read_exception,"Last 5 bits of the third byte should be zero");
	// fourth byte
	
	vg->VTDDepth = ba[3];

	// 5th and 6th byte
	
	LCLevel = (((int) ba[4]) << 8) | ba[5];
	if (LCLevel !=4 && LCLevel !=6)
	{
		throwException2(index_read_exception,"LC levels must be at least 3");
		return FALSE;
	}
	if (LCLevel ==4)
		vg->shallowDepth = TRUE;
	else
		vg->shallowDepth = FALSE;

	// 7th and 8th byte
	vg->rootIndex = (((int) ba[6]) << 8) | ba[7];

	//Console.WriteLine(" l ==>" + l);
	l=((Long*)(ba+24))[0];
	//Console.WriteLine(" l ==>" + l);
	
	// read XML size
	if (littleEndian && (endian == 0)
		|| (littleEndian == FALSE) && (endian == 1))
		size = (int)l;
	else
		size = (int)reverseLong(l);

	if (len < 32+size){
		throwException2(index_read_exception,"Invalid Index error");
		return FALSE;
	}
	setDoc2(vg,ba,len,32,size);
	//dis.Read(XMLDoc,0,size);
	if ((size & 0x7) != 0)
	{
		t = (((size >> 3) + 1) << 3) - size;
	}

	count = 32 + size +t;
	if ( (littleEndian && (endian == 0))
		|| (littleEndian == FALSE && endian == 1))
	{
		int vtdSize,l1Size,l2Size,l3Size,l4Size,l5Size;
		// read vtd records
		if (len < count+8){
			throwException2(index_read_exception,"Invalid Index error");
			return FALSE;
		}
		l = ((Long*)(ba+count))[0];
		count+=8;
		vtdSize = (int) l;
		while (vtdSize > 0)
		{
			if (len < count+8){
				throwException2(index_read_exception,"Invalid Index error");
				return FALSE;
			}
			l = ((Long*)(ba+count))[0];
			count+=8;
			appendLong(vg->VTDBuffer ,adjust(l,adj));
			vtdSize--;
		}
		// read L1 LC records
		if (len < count+8){
			throwException2(index_read_exception,"Invalid Index error");
			return FALSE;
		}
		l = ((Long*)(ba+count))[0];
		count+=8;
		l1Size = (int) l;
		while (l1Size > 0)
		{
			if (len < count+8){
				throwException2(index_read_exception,"Invalid Index error");
				return FALSE;
			}
			l = ((Long*)(ba+count))[0];
			count+=8;
			appendLong(vg->l1Buffer,l);
			l1Size--;
		}
		// read L2 LC records
		if (len < count+8){
			throwException2(index_read_exception,"Invalid Index error");
			return FALSE;
		}
		l = ((Long*)(ba+count))[0];
		count+=8;
		l2Size = (int) l;
		while (l2Size > 0)
		{
			if (len < count+8){
				throwException2(index_read_exception,"Invalid Index error");
				return FALSE;
			}
			l = ((Long*)(ba+count))[0];
			count+=8;
			appendLong(vg->l2Buffer, l);
			l2Size--;
		}
		// read L3 LC records
		if (len < count+8){
			throwException2(index_read_exception,"Invalid Index error");
			return FALSE;
		}
		l = ((Long*)(ba+count))[0];
		count+=8;
		l3Size = (int) l;
		if (vg->shallowDepth){
			if (intLongSwitch == 1)
			{
				//l3 uses ints
				while (l3Size > 0)
				{
					int i;
					if (len < count+4){
						throwException2(index_read_exception,"Invalid Index error");
						return FALSE;
					}
					i = ((int*)(ba+count))[0];
					count+=4;
					appendInt(vg->l3Buffer,i);
					l3Size--;
				}
			}
			else
			{
				while (l3Size > 0)
				{
					if (len < count+8){
						throwException2(index_read_exception,"Invalid Index error");
						return FALSE;
					}
					l = ((Long*)(ba+count))[0];
					count+=8;
					appendLong((FastLongBuffer *)vg->l3Buffer,(int)(l >> 32));
					l3Size--;
				}
			}
		}else {			
			while (l3Size > 0)
			{
				if (len < count+8){
					throwException2(index_read_exception,"Invalid Index error");
					return FALSE;
				}
				l = ((Long*)(ba+count))[0];
				count+=8;
				appendLong(vg->_l3Buffer, l);
				l3Size--;
			}

			l = ((Long*)(ba+count))[0];
			count+=8;
			l4Size = (int) l;
			while (l4Size > 0)
			{
				if (len < count+8){
					throwException2(index_read_exception,"Invalid Index error");
					return FALSE;
				}
				l = ((Long*)(ba+count))[0];
				count+=8;
				appendLong(vg->_l4Buffer, l);
				l4Size--;
			}
			l = ((Long*)(ba+count))[0];
			count+=8;
			l5Size = (int) l;
			if (intLongSwitch == 1)
			{
				//l3 uses ints
				while (l5Size > 0)
				{
					int i;
					if (len < count+4){
						throwException2(index_read_exception,"Invalid Index error");
						return FALSE;
					}
					i = ((int*)(ba+count))[0];
					count+=4;
					appendInt(vg->_l5Buffer,i);
					l5Size--;
				}
			}
			else
			{
				while (l5Size > 0)
				{
					if (len < count+8){
						throwException2(index_read_exception,"Invalid Index error");
						return FALSE;
					}
					l = ((Long*)(ba+count))[0];
					count+=8;
					appendLong((FastLongBuffer *)vg->_l5Buffer,(int)(l >> 32));
					l5Size--;
				}
			}

		}
	}
	else
	{
		// read vtd records
		int vtdSize, l1Size, l2Size, l3Size, l4Size, l5Size;
		// read vtd records
		if (len < count+8){
			throwException2(index_read_exception,"Invalid Index error");
			return FALSE;
		}
		l = ((Long*)(ba+count))[0];
		count+=8;
		vtdSize = (int) reverseLong(l);
		while (vtdSize > 0)
		{
			if (len < count+8){
				throwException2(index_read_exception,"Invalid Index error");
				return FALSE;
			}
			l = ((Long*)(ba+count))[0];
			count+=8;
			appendLong(vg->VTDBuffer,adjust(reverseLong(l),adj));
			vtdSize--;
		}
		// read L1 LC records
		if (len < count+8){
			throwException2(index_read_exception,"Invalid Index error");
			return FALSE;
		}
		l = ((Long*)(ba+count))[0];
		count+=8;
		l1Size = (int) reverseLong(l);
		while (l1Size > 0)
		{
			if (len < count+8){
				throwException2(index_read_exception,"Invalid Index error");
				return FALSE;
			}
			l = ((Long*)(ba+count))[0];
			count+=8;
			appendLong(vg->l1Buffer,reverseLong(l));
			l1Size--;
		}
		// read L2 LC records
		if (len < count+8){
			throwException2(index_read_exception,"Invalid Index error");
			return FALSE;
		}
		l = ((Long*)(ba+count))[0];
		count+=8;
		l2Size = (int) reverseLong(l);
		while (l2Size > 0)
		{
			if (len < count+8){
				throwException2(index_read_exception,"Invalid Index error");
				return FALSE;
			}
			l = ((Long*)(ba+count))[0];
			count+=8;
			appendLong(vg->l2Buffer, reverseLong(l));
			l2Size--;
		}
		// read L3 LC records
		if (len < count+8){
			throwException2(index_read_exception,"Invalid Index error");
			return FALSE;
		}
		l = ((Long*)(ba+count))[0];
		count+=8;
		l3Size = (int) reverseLong(l);
		if (vg->shallowDepth){
			if (intLongSwitch == 1)
			{
				//l3 uses ints
				int i;
				while (l3Size > 0)
				{
					if (len < count+4){
						throwException2(index_read_exception,"Invalid Index error");
						return FALSE;
					}
					i = ((int*)(ba+count))[0];
					count+=4;
					appendInt(vg->l3Buffer,reverseInt(i));
					l3Size--;
				}
			}
			else
			{
				while (l3Size > 0)
				{
					if (len < count+8){
						throwException2(index_read_exception,"Invalid Index error");
						return FALSE;
					}
					l = ((Long*)(ba+count))[0];
					count+=8;
					appendInt(vg->l3Buffer,reverseInt((int) (l >> 32)));
					l3Size--;
				}
			}
		}else{
			while (l3Size > 0)
			{
				if (len < count+8){
					throwException2(index_read_exception,"Invalid Index error");
					return FALSE;
				}
				l = ((Long*)(ba+count))[0];
				count+=8;
				appendLong(vg->_l3Buffer, reverseLong(l));
				l3Size--;
			}
			// read L3 LC records
			if (len < count+8){
				throwException2(index_read_exception,"Invalid Index error");
				return FALSE;
			}
			l = ((Long*)(ba+count))[0];
			count+=8;
			l4Size = (int) reverseLong(l);

			while (l4Size > 0)
			{
				if (len < count+8){
					throwException2(index_read_exception,"Invalid Index error");
					return FALSE;
				}
				l = ((Long*)(ba+count))[0];
				count+=8;
				appendLong(vg->_l4Buffer, reverseLong(l));
				l4Size--;
			}
			// read L3 LC records
			if (len < count+8){
				throwException2(index_read_exception,"Invalid Index error");
				return FALSE;
			}
			l = ((Long*)(ba+count))[0];
			count+=8;
			l5Size = (int) reverseLong(l);

			if (intLongSwitch == 1)
			{
				//l3 uses ints
				int i;
				while (l5Size > 0)
				{
					if (len < count+4){
						throwException2(index_read_exception,"Invalid Index error");
						return FALSE;
					}
					i = ((int*)(ba+count))[0];
					count+=4;
					appendInt(vg->_l5Buffer,reverseInt(i));
					l5Size--;
				}
			}
			else
			{
				while (l5Size > 0)
				{
					if (len < count+8){
						throwException2(index_read_exception,"Invalid Index error");
						return FALSE;
					}
					l = ((Long*)(ba+count))[0];
					count+=8;
					appendInt(vg->_l5Buffer,reverseInt((int) (l >> 32)));
					l5Size--;
				}
			}
		}
	}
	//fclose(f);
	return TRUE;
}
Long adjust(Long l,int i){
       Long l1 = (l & 0xffffffffLL)+ i;
       Long l2 = l & 0xffffffff00000000L;
       return l1|l2;   
}

Boolean _writeSeparateIndex_L3(Byte version, 
				int encodingType, 
				Boolean ns, 
				Boolean byteOrder, 
				int nestDepth, 
				int LCLevel, 
				int rootIndex, 
				//UByte* xmlDoc, 
				int docOffset, 
				int docLen, 
				FastLongBuffer *vtdBuffer, 
				FastLongBuffer *l1Buffer, 
				FastLongBuffer *l2Buffer, 
				FastIntBuffer *l3Buffer, 
				FILE *f){
					int i;
					Byte ba[4];
					Long l;
					Boolean littleEndian = isLittleEndian();
			if (docLen <= 0 
				|| vtdBuffer == NULL
				|| l1Buffer == NULL 
				|| l2Buffer == NULL 
				|| l3Buffer == NULL
				|| f == NULL)
			{
				throwException2(invalid_argument, "writeSeparateIndex's argument invalid");
				return FALSE;	
			}

			if (vtdBuffer->size == 0){
				throwException2(index_write_exception,"vTDBuffer can't be zero in size");
			}
			
			//UPGRADE_TODO: Class 'java.io.DataOutputStream' was converted to 'System.IO.BinaryWriter' which has a different behavior. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1073_javaioDataOutputStream'"
			//System.IO.BinaryWriter dos = new System.IO.BinaryWriter(os);
			// first 4 bytes
			
			ba[0] = (Byte) version; // version # is 2 
			ba[1] = (Byte) encodingType;
            if (littleEndian == FALSE)
                ba[2] = (Byte)(ns ? 0xe0 : 0xa0); // big endien
            else
                ba[2] = (Byte)(ns ? 0xc0 : 0x80);
			ba[3] = (Byte) nestDepth;
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			// second 4 bytes
			ba[0] = 0;
			ba[1] = 4;
			ba[2] = (Byte) ((rootIndex & 0xff00) >> 8);
			ba[3] = (Byte) (rootIndex & 0xff);
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			// 2 reserved 32-bit words set to zero
			ba[1] = ba[2] = ba[3] = 0;
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			// write XML doc in bytes	
			l = docLen;
			if (fwrite((UByte*) (&l), 1 , 8,f)!=8){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}

			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}

            //dos.Write(xmlDoc, docOffset, docLen);
			/*if (fwrite((UByte*)(xmlDoc+docOffset), 1, docLen,f)!=docLen){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}*/
			//dos.Write(xmlDoc, docOffset, docLen);
			// zero padding to make it integer multiple of 64 bits
			/*if ((docLen & 0x07) != 0)
			{
				int t = (((docLen >> 3) + 1) << 3) - docLen;
				for (; t > 0; t--){
					if (fwrite(ba,1,1,f)!=1){
						throwException2(index_write_exception, "fwrite error occurred");
						return FALSE;
					};
				}
			}*/
			// write VTD
            
			//dos.Write((long)vtdBuffer.size());
			l = vtdBuffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < vtdBuffer->size; i++)
			{
				l = longAt(vtdBuffer,i);
				if (fwrite((UByte*) &l, 1 , 8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}
			// write L1 
			//dos.Write((long)l1Buffer.size());
			l = l1Buffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l1Buffer->size; i++)
			{
				l = longAt(l1Buffer,i);
				if (fwrite((UByte*) &l, 1 ,8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}
			// write L2
			l = l2Buffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l2Buffer->size; i++)
			{
				l = longAt(l2Buffer,i);
				if (fwrite((UByte*) &l, 1 , 8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}
			// write L3
			l = l3Buffer->size;
			if (fwrite((UByte*) &l, 1 , 8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l3Buffer->size; i++)
			{
				int s = intAt(l3Buffer,i);
				if (fwrite((UByte*)&s, 1 , 4, f)!=4){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}
			// pad zero if # of l3 entry is odd
			if ((l3Buffer->size & 1) != 0){
				if (fwrite(ba,1,1,f)!=1){
						throwException2(index_write_exception, "fwrite error occurred");
						return FALSE;
					};
			}
			//fclose(f);
			return TRUE;

}

Boolean _readSeparateIndex(FILE *xml, int XMLSize, FILE *f, VTDGen *vg){
	int intLongSwitch;
	int endian;
	Byte ba[4];
	int LCLevel;
	Long l;
	int size;
	UByte *XMLDoc;
	Boolean littleEndian = isLittleEndian();
	if (f == NULL || vg == NULL)
	{
		throwException2(invalid_argument,"Invalid argument(s) for readIndex()");
		return FALSE;
	}
	
	// first byte
	if (fread(ba,1,1,f) != 1){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	if (ba[0]!=2){
		throwException2(index_read_exception,"version # error");
		return FALSE;
	}

	// no check on version number for now
	// second byte
	if (fread(ba,1,1,f) != 1){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}

	vg->encoding = ba[0];
	
	// third byte
	if (fread(ba,1,1,f) != 1){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	if ((ba[0] & 0x80) != 0)
		intLongSwitch = 1;
	//use ints
	else
		intLongSwitch = 0;
	if ((ba[0] & 0x40) != 0)
		vg->ns = TRUE;
	else
		vg->ns = FALSE;
	if ((ba[0] & 0x20) != 0)
		endian = 1;
	else
		endian = 0;

	if ((ba[0] & 0x1f) != 0)
		throwException2(index_read_exception,"Last 5 bits of the third byte should be zero");
	
	// fourth byte
	if (fread(ba,1,1,f) != 1){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	vg->VTDDepth = ba[0];

	// 5th and 6th byte
	if (fread(ba,1,2,f) != 2){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	LCLevel = (((int) ba[0]) << 8) | ba[1];
	if (LCLevel != 4 && LCLevel!=6)
	{
		throwException2(index_read_exception,"LC levels must be at least 3");
		return FALSE;
	}
	if (LCLevel ==4)
		vg->shallowDepth = TRUE;
	else
		vg->shallowDepth = FALSE;
	// 7th and 8th byte
	if (fread(ba,1,2,f) != 2){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	vg->rootIndex = (((int) ba[0]) << 8) | ba[1];

	// skip a long
	if (fread(&l,1,8,f) != 8){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	//Console.WriteLine(" l ==>" + l);
	if (fread(&l,1,8,f) != 8){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	//Console.WriteLine(" l ==>" + l);
	if (fread(&l,1,8,f) != 8){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	//Console.WriteLine(" l ==>" + l);
	// read XML size
	if (littleEndian && (endian == 0)
		|| (littleEndian == FALSE) && (endian == 1))
		size = (int)l;
	else
		size = (int)reverseLong(l);
	
	if (size!= XMLSize){
		throwException2(index_read_exception, "XML size mismatch");
	}

	//Console.WriteLine(" l ==>" + l);
	if (fread(&l,1,8,f) != 8){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	//Console.WriteLine(" l ==>" + l);
	if (fread(&l,1,8,f) != 8){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}

	// read XML bytes the assumption is that there is no offset shift
	// in XML bytes
	XMLDoc = malloc(size);
	if (XMLDoc == NULL){
		throwException2(out_of_mem,"Byte array allocation failed");
		return FALSE;
	}
	if (fread(XMLDoc,1,size,xml)!= size){
		throwException2(index_read_exception,"fread error occurred");
		return FALSE;
	}
	//dis.Read(XMLDoc,0,size);
	/*if ((size & 0x7) != 0)
	{
		int t = (((size >> 3) + 1) << 3) - size;
		while (t > 0)
		{
			if (fread(ba,1,1,xml) != 1){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			t--;
		}
	}*/

	setDoc(vg,XMLDoc,size);

	if ( (littleEndian && (endian == 0))
		|| (littleEndian == FALSE && endian == 1))
	{
		int vtdSize,l1Size,l2Size,l3Size,l4Size, l5Size;
		// read vtd records
		l=0;
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		vtdSize = (int) l;
		while (vtdSize > 0)
		{
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			appendLong(vg->VTDBuffer ,l);
			vtdSize--;
		}
		// read L1 LC records
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		l1Size = (int) l;
		while (l1Size > 0)
		{
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			appendLong(vg->l1Buffer,l);
			l1Size--;
		}
		// read L2 LC records
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		l2Size = (int) l;
		while (l2Size > 0)
		{
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			appendLong(vg->l2Buffer, l);
			l2Size--;
		}
		// read L3 LC records
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		l3Size = (int) l;
		if (vg->shallowDepth){
			if (intLongSwitch == 1)
			{
				//l3 uses ints
				while (l3Size > 0)
				{
					if (fread(&size,1,4,f) != 4){
						throwException2(index_read_exception,"fread error occurred");
						return FALSE;
					}
					appendInt(vg->l3Buffer,size);
					l3Size--;
				}
			}
			else
			{
				while (l3Size > 0)
				{
					if (fread(&l,1,8,f) != 8){
						throwException2(index_read_exception,"fread error occurred");
						return FALSE;
					}
					appendLong((FastLongBuffer *)vg->l3Buffer,(int)(l >> 32));
					l3Size--;
				}
			}
		}else{
			while (l3Size > 0)
			{
				if (fread(&l,1,8,f) != 8){
					throwException2(index_read_exception,"fread error occurred");
					return FALSE;
				}
				appendLong(vg->_l3Buffer,l);
				l3Size--;
			}
			// read L2 LC records
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			l4Size = (int) l;
			while (l2Size > 0)
			{
				if (fread(&l,1,8,f) != 8){
					throwException2(index_read_exception,"fread error occurred");
					return FALSE;
				}
				appendLong(vg->_l4Buffer, l);
				l4Size--;
			}
			// read L3 LC records
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			l5Size = (int) l;
			if (intLongSwitch == 1)
			{
				//l3 uses ints
				while (l5Size > 0)
				{
					if (fread(&size,1,4,f) != 4){
						throwException2(index_read_exception,"fread error occurred");
						return FALSE;
					}
					appendInt(vg->_l5Buffer,size);
					l5Size--;
				}
			}
			else
			{
				while (l5Size > 0)
				{
					if (fread(&l,1,8,f) != 8){
						throwException2(index_read_exception,"fread error occurred");
						return FALSE;
					}
					appendLong((FastLongBuffer *)vg->_l3Buffer,(int)(l >> 32));
					l5Size--;
				}
			}
		}
	}
	else
	{
		// read vtd records
		int vtdSize,l1Size,l2Size,l3Size,l4Size,l5Size;
		// read vtd records
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		vtdSize = (int) reverseLong(l);
		while (vtdSize > 0)
		{
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			appendLong(vg->VTDBuffer,reverseLong(l));
			vtdSize--;
		}
		// read L1 LC records
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		l1Size = (int) reverseLong(l);
		while (l1Size > 0)
		{
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			appendLong(vg->l1Buffer,reverseLong(l));
			l1Size--;
		}
		// read L2 LC records
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		l2Size = (int) reverseLong(l);
		while (l2Size > 0)
		{
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			appendLong(vg->l2Buffer, reverseLong(l));
			l2Size--;
		}
		// read L3 LC records
		if (fread(&l,1,8,f) != 8){
			throwException2(index_read_exception,"fread error occurred");
			return FALSE;
		}
		l3Size = (int) reverseLong(l);
		if (vg->shallowDepth){
			if (intLongSwitch == 1)
			{
				//l3 uses ints
				while (l3Size > 0)
				{
					if (fread(&size,1,4,f) != 4){
						throwException2(index_read_exception,"fread error occurred");
						return FALSE;
					}
					appendInt(vg->l3Buffer,reverseInt(size));
					l3Size--;
				}
			}
			else
			{
				while (l3Size > 0)
				{
					if (fread(&l,1,8,f) != 8){
						throwException2(index_read_exception,"fread error occurred");
						return FALSE;
					}
					appendInt(vg->l3Buffer,reverseInt((int) (l >> 32)));
					l3Size--;
				}
			}
		}else{
			while (l3Size > 0)
			{
				if (fread(&l,1,8,f) != 8){
					throwException2(index_read_exception,"fread error occurred");
					return FALSE;
				}
				appendLong(vg->_l3Buffer,reverseLong(l));
				l3Size--;
			}
			// read L2 LC records
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			l4Size = (int) reverseLong(l);
			while (l4Size > 0)
			{
				if (fread(&l,1,8,f) != 8){
					throwException2(index_read_exception,"fread error occurred");
					return FALSE;
				}
				appendLong(vg->_l4Buffer, reverseLong(l));
				l4Size--;
			}
			// read L3 LC records
			if (fread(&l,1,8,f) != 8){
				throwException2(index_read_exception,"fread error occurred");
				return FALSE;
			}
			l5Size = (int) reverseLong(l);
			if (vg->shallowDepth){
				if (intLongSwitch == 1)
				{
					//l3 uses ints
					while (l5Size > 0)
					{
						if (fread(&size,1,4,f) != 4){
							throwException2(index_read_exception,"fread error occurred");
							return FALSE;
						}
						appendInt(vg->_l5Buffer,reverseInt(size));
						l5Size--;
					}
				}
				else
				{
					while (l5Size > 0)
					{
						if (fread(&l,1,8,f) != 8){
							throwException2(index_read_exception,"fread error occurred");
							return FALSE;
						}
						appendInt(vg->_l5Buffer,reverseInt((int) (l >> 32)));
						l5Size--;
					}
				}
			}
		}
	}
	//fclose(f);
	return TRUE;
	
}


Boolean _writeIndex_L5(Byte version, 
				int encodingType, 
				Boolean ns, 
				Boolean byteOrder, 
				int nestDepth, 
				int LCLevel, 
				int rootIndex, 
				UByte* xmlDoc, 
				int docOffset, 
				int docLen, 
				FastLongBuffer *vtdBuffer, 
				FastLongBuffer *l1Buffer, 
				FastLongBuffer *l2Buffer, 
				FastLongBuffer *l3Buffer, 
				FastLongBuffer *l4Buffer,
				FastIntBuffer *l5Buffer,
				FILE *f){					
					int i;
					Byte ba[4];
					Long l;
					Boolean littleEndian = isLittleEndian();
			if (xmlDoc == NULL 
				|| docLen <= 0 
				|| vtdBuffer == NULL
				|| l1Buffer == NULL 
				|| l2Buffer == NULL 
				|| l3Buffer == NULL
				|| l4Buffer == NULL
				|| l5Buffer == NULL
				|| f == NULL)
			{
				throwException2(invalid_argument, "writeIndex's argument invalid");
				return FALSE;	
			}

			if (vtdBuffer->size == 0){
				throwException2(index_write_exception,"vTDBuffer can't be zero in size");
			}
			
			//UPGRADE_TODO: Class 'java.io.DataOutputStream' was converted to 'System.IO.BinaryWriter' which has a different behavior. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1073_javaioDataOutputStream'"
			//System.IO.BinaryWriter dos = new System.IO.BinaryWriter(os);
			// first 4 bytes
			
			ba[0] = (Byte) version; // version # is 1 
			ba[1] = (Byte) encodingType;
            if (littleEndian == FALSE)
                ba[2] = (Byte)(ns ? 0xe0 : 0xa0); // big endien
            else
                ba[2] = (Byte)(ns ? 0xc0 : 0x80);
			ba[3] = (Byte) nestDepth;
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			// second 4 bytes
			ba[0] = 0;
			ba[1] = 6;
			ba[2] = (Byte) ((rootIndex & 0xff00) >> 8);
			ba[3] = (Byte) (rootIndex & 0xff);
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			// 2 reserved 32-bit words set to zero
			ba[1] = ba[2] = ba[3] = 0;
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			// write XML doc in bytes	
			l = docLen;
			if (fwrite((UByte*) (&l), 1 , 8,f)!=8){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
            //dos.Write(xmlDoc, docOffset, docLen);
			if (fwrite((UByte*)(xmlDoc+docOffset), 1, docLen,f)!=docLen){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			//dos.Write(xmlDoc, docOffset, docLen);
			// zero padding to make it integer multiple of 64 bits
			if ((docLen & 0x07) != 0)
			{
				int t = (((docLen >> 3) + 1) << 3) - docLen;
				for (; t > 0; t--){
					if (fwrite(ba,1,1,f)!=1){
						throwException2(index_write_exception, "fwrite error occurred");
						return FALSE;
					};
				}
			}
			// write VTD
            
			//dos.Write((long)vtdBuffer.size());
			l = vtdBuffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < vtdBuffer->size; i++)
			{
				l = longAt(vtdBuffer,i);
				if (fwrite((UByte*) &l, 1 , 8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}
			// write L1 
			//dos.Write((long)l1Buffer.size());
			l = l1Buffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l1Buffer->size; i++)
			{
				l = longAt(l1Buffer,i);
				if (fwrite((UByte*) &l, 1 ,8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}
			// write L2
			l = l2Buffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l2Buffer->size; i++)
			{
				l = longAt(l2Buffer,i);
				if (fwrite((UByte*) &l, 1 , 8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}

			// write L3
			l = l3Buffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l3Buffer->size; i++)
			{
				l = longAt(l3Buffer,i);
				if (fwrite((UByte*) &l, 1 , 8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}

			// write L4
			l = l4Buffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l4Buffer->size; i++)
			{
				l = longAt(l4Buffer,i);
				if (fwrite((UByte*) &l, 1 , 8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}

			// write L5
			l = l5Buffer->size;
			if (fwrite((UByte*) &l, 1 , 8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l5Buffer->size; i++)
			{
				int s = intAt(l5Buffer,i);
				if (fwrite((UByte*)&s, 1 , 4, f)!=4){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}
			// pad zero if # of l5 entry is odd
			if ((l5Buffer->size & 1) != 0){
				if (fwrite(ba,1,1,f)!=1){
						throwException2(index_write_exception, "fwrite error occurred");
						return FALSE;
					};
			}
			//fclose(f);
			return TRUE;
}

Boolean _writeSeparateIndex_L5(Byte version, 
				int encodingType, 
				Boolean ns, 
				Boolean byteOrder, 
				int nestDepth, 
				int LCLevel, 
				int rootIndex, 
				//UByte* xmlDoc, 
				int docOffset, 
				int docLen, 
				FastLongBuffer *vtdBuffer, 
				FastLongBuffer *l1Buffer, 
				FastLongBuffer *l2Buffer, 
				FastLongBuffer *l3Buffer, 
				FastLongBuffer *l4Buffer,
				FastIntBuffer *l5Buffer,
				FILE *f){
										int i;
					Byte ba[4];
					Long l;
					Boolean littleEndian = isLittleEndian();
			if (docLen <= 0 
				|| vtdBuffer == NULL
				|| l1Buffer == NULL 
				|| l2Buffer == NULL 
				|| l3Buffer == NULL
				|| l4Buffer == NULL
				|| l5Buffer == NULL
				|| f == NULL)
			{
				throwException2(invalid_argument, "writeSeparateIndex's argument invalid");
				return FALSE;	
			}

			if (vtdBuffer->size == 0){
				throwException2(index_write_exception,"vTDBuffer can't be zero in size");
			}
			
			//UPGRADE_TODO: Class 'java.io.DataOutputStream' was converted to 'System.IO.BinaryWriter' which has a different behavior. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1073_javaioDataOutputStream'"
			//System.IO.BinaryWriter dos = new System.IO.BinaryWriter(os);
			// first 4 bytes
			
			ba[0] = (Byte) version; // version # is 2 
			ba[1] = (Byte) encodingType;
            if (littleEndian == FALSE)
                ba[2] = (Byte)(ns ? 0xe0 : 0xa0); // big endien
            else
                ba[2] = (Byte)(ns ? 0xc0 : 0x80);
			ba[3] = (Byte) nestDepth;
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			// second 4 bytes
			ba[0] = 0;
			ba[1] = 6;
			ba[2] = (Byte) ((rootIndex & 0xff00) >> 8);
			ba[3] = (Byte) (rootIndex & 0xff);
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			// 2 reserved 32-bit words set to zero
			ba[1] = ba[2] = ba[3] = 0;
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			// write XML doc in bytes	
			l = docLen;
			if (fwrite((UByte*) (&l), 1 , 8,f)!=8){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}

			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}
			if (fwrite(ba,1,4,f)!=4){
				throwException2(index_write_exception,"fwrite error occurred");
				return FALSE;
			}

            //dos.Write(xmlDoc, docOffset, docLen);
			/*if (fwrite((UByte*)(xmlDoc+docOffset), 1, docLen,f)!=docLen){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}*/
			//dos.Write(xmlDoc, docOffset, docLen);
			// zero padding to make it integer multiple of 64 bits
			/*if ((docLen & 0x07) != 0)
			{
				int t = (((docLen >> 3) + 1) << 3) - docLen;
				for (; t > 0; t--){
					if (fwrite(ba,1,1,f)!=1){
						throwException2(index_write_exception, "fwrite error occurred");
						return FALSE;
					};
				}
			}*/
			// write VTD
            
			//dos.Write((long)vtdBuffer.size());
			l = vtdBuffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < vtdBuffer->size; i++)
			{
				l = longAt(vtdBuffer,i);
				if (fwrite((UByte*) &l, 1 , 8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}
			// write L1 
			//dos.Write((long)l1Buffer.size());
			l = l1Buffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l1Buffer->size; i++)
			{
				l = longAt(l1Buffer,i);
				if (fwrite((UByte*) &l, 1 ,8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}
			// write L2
			l = l2Buffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l2Buffer->size; i++)
			{
				l = longAt(l2Buffer,i);
				if (fwrite((UByte*) &l, 1 , 8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}

			// write L3
			l = l3Buffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l3Buffer->size; i++)
			{
				l = longAt(l3Buffer,i);
				if (fwrite((UByte*) &l, 1 , 8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}

			// write L4
			l = l4Buffer->size;
			if (fwrite((UByte*) &l, 1 ,8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l4Buffer->size; i++)
			{
				l = longAt(l4Buffer,i);
				if (fwrite((UByte*) &l, 1 , 8,f)!=8){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}


			// write L5
			l = l5Buffer->size;
			if (fwrite((UByte*) &l, 1 , 8,f)!=8){
				throwException2(index_write_exception, "fwrite error occurred");
				return FALSE;
			}
			for (i = 0; i < l5Buffer->size; i++)
			{
				int s = intAt(l5Buffer,i);
				if (fwrite((UByte*)&s, 1 , 4, f)!=4){
					throwException2(index_write_exception, "fwrite error occurred");
					return FALSE;
				}
			}
			// pad zero if # of l5 entry is odd
			if ((l5Buffer->size & 1) != 0){
				if (fwrite(ba,1,1,f)!=1){
						throwException2(index_write_exception, "fwrite error occurred");
						return FALSE;
					};
			}
			//fclose(f);
			return TRUE;
}
