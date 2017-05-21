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

#include "decoder.h"

void iso_8859_2_chars_init(){
	int i;
	if (iso_8859_2_chars_ready)
		return;
	else{
		for ( i = 0; i < 128; i++)
		{
			iso_8859_2_chars[i] = (char) i;
		}
		for ( i = 128; i < 256; i++)
		{
			iso_8859_2_chars[i] = (char) (0xfffd);
		}
		iso_8859_2_chars[0xA0] = (char) (0x00A0); /*	NO-BREAK SPACE */
		iso_8859_2_chars[0xA1] = (char) (0x0104); /*	LATIN CAPITAL LETTER A WITH OGONEK*/
		iso_8859_2_chars[0xA2] = (char) (0x02D8); /*	BREVE */
		iso_8859_2_chars[0xA3] = (char) (0x0141); /*	LATIN CAPITAL LETTER L WITH STROKE*/
		iso_8859_2_chars[0xA4] = (char) (0x00A4); /*	CURRENCY SIGN*/
		iso_8859_2_chars[0xA5] = (char) (0x013D); /*	LATIN CAPITAL LETTER L WITH CARON*/
		iso_8859_2_chars[0xA6] = (char) (0x015A); /*	LATIN CAPITAL LETTER S WITH ACUTE*/
		iso_8859_2_chars[0xA7] = (char) (0x00A7); /*	SECTION SIGN*/
		iso_8859_2_chars[0xA8] = (char) (0x00A8); /*	DIAERESIS*/
		iso_8859_2_chars[0xA9] = (char) (0x0160); /*	LATIN CAPITAL LETTER S WITH CARON*/
		iso_8859_2_chars[0xAA] = (char) (0x015E); /*	LATIN CAPITAL LETTER S WITH CEDILLA*/
		iso_8859_2_chars[0xAB] = (char) (0x0164); /*	LATIN CAPITAL LETTER T WITH CARON*/
		iso_8859_2_chars[0xAC] = (char) (0x0179); /*	LATIN CAPITAL LETTER Z WITH ACUTE*/
		iso_8859_2_chars[0xAD] = (char) (0x00AD); /*	SOFT HYPHEN*/
		iso_8859_2_chars[0xAE] = (char) (0x017D); /*	LATIN CAPITAL LETTER Z WITH CARON*/
		iso_8859_2_chars[0xAF] = (char) (0x017B); /*	LATIN CAPITAL LETTER Z WITH DOT ABOVE*/
		iso_8859_2_chars[0xB0] = (char) (0x00B0); /*	DEGREE SIGN*/
		iso_8859_2_chars[0xB1] = (char) (0x0105); /*	LATIN SMALL LETTER A WITH OGONEK*/
		iso_8859_2_chars[0xB2] = (char) (0x02DB); /*	OGONEK*/
		iso_8859_2_chars[0xB3] = (char) (0x0142); /*	LATIN SMALL LETTER L WITH STROKE*/
		iso_8859_2_chars[0xB4] = (char) (0x00B4); /*	ACUTE ACCENT*/
		iso_8859_2_chars[0xB5] = (char) (0x013E); /*	LATIN SMALL LETTER L WITH CARON*/
		iso_8859_2_chars[0xB6] = (char) (0x015B); /*	LATIN SMALL LETTER S WITH ACUTE*/
		iso_8859_2_chars[0xB7] = (char) (0x02C7); /*	CARON*/
		iso_8859_2_chars[0xB8] = (char) (0x00B8); /*	CEDILLA*/
		iso_8859_2_chars[0xB9] = (char) (0x0161); /*	LATIN SMALL LETTER S WITH CARON*/
		iso_8859_2_chars[0xBA] = (char) (0x015F); /*	LATIN SMALL LETTER S WITH CEDILLA*/
		iso_8859_2_chars[0xBB] = (char) (0x0165); /*	LATIN SMALL LETTER T WITH CARON*/
		iso_8859_2_chars[0xBC] = (char) (0x017A); /*	LATIN SMALL LETTER Z WITH ACUTE*/
		iso_8859_2_chars[0xBD] = (char) (0x02DD); /*	DOUBLE ACUTE ACCENT*/
		iso_8859_2_chars[0xBE] = (char) (0x017E); /*	LATIN SMALL LETTER Z WITH CARON*/
		iso_8859_2_chars[0xBF] = (char) (0x017C); /*	LATIN SMALL LETTER Z WITH DOT ABOVE*/
		iso_8859_2_chars[0xC0] = (char) (0x0154); /*	LATIN CAPITAL LETTER R WITH ACUTE*/
		iso_8859_2_chars[0xC1] = (char) (0x00C1); /*	LATIN CAPITAL LETTER A WITH ACUTE*/
		iso_8859_2_chars[0xC2] = (char) (0x00C2); /*	LATIN CAPITAL LETTER A WITH CIRCUMFLEX*/
		iso_8859_2_chars[0xC3] = (char) (0x0102); /*	LATIN CAPITAL LETTER A WITH BREVE*/
		iso_8859_2_chars[0xC4] = (char) (0x00C4); /*	LATIN CAPITAL LETTER A WITH DIAERESIS*/
		iso_8859_2_chars[0xC5] = (char) (0x0139); /*	LATIN CAPITAL LETTER L WITH ACUTE*/
		iso_8859_2_chars[0xC6] = (char) (0x0106); /*	LATIN CAPITAL LETTER C WITH ACUTE*/
		iso_8859_2_chars[0xC7] = (char) (0x00C7); /*	LATIN CAPITAL LETTER C WITH CEDILLA*/
		iso_8859_2_chars[0xC8] = (char) (0x010C); /*	LATIN CAPITAL LETTER C WITH CARON*/
		iso_8859_2_chars[0xC9] = (char) (0x00C9); /*	LATIN CAPITAL LETTER E WITH ACUTE*/
		iso_8859_2_chars[0xCA] = (char) (0x0118); /*LATIN CAPITAL LETTER E WITH OGONEK*/
		iso_8859_2_chars[0xCB] = (char) (0x00CB); /*	LATIN CAPITAL LETTER E WITH DIAERESIS*/
		iso_8859_2_chars[0xCC] = (char) (0x011A); /*	LATIN CAPITAL LETTER E WITH CARON*/
		iso_8859_2_chars[0xCD] = (char) (0x00CD); /*	LATIN CAPITAL LETTER I WITH ACUTE*/
		iso_8859_2_chars[0xCE] = (char) (0x00CE); /*	LATIN CAPITAL LETTER I WITH CIRCUMFLEX*/
		iso_8859_2_chars[0xCF] = (char) (0x010E); /*	LATIN CAPITAL LETTER D WITH CARON*/
		iso_8859_2_chars[0xD0] = (char) (0x0110); /*	LATIN CAPITAL LETTER D WITH STROKE*/
		iso_8859_2_chars[0xD1] = (char) (0x0143); /*	LATIN CAPITAL LETTER N WITH ACUTE*/
		iso_8859_2_chars[0xD2] = (char) (0x0147); /*	LATIN CAPITAL LETTER N WITH CARON*/
		iso_8859_2_chars[0xD3] = (char) (0x00D3); /*	LATIN CAPITAL LETTER O WITH ACUTE*/
		iso_8859_2_chars[0xD4] = (char) (0x00D4); /*	LATIN CAPITAL LETTER O WITH CIRCUMFLEX*/
		iso_8859_2_chars[0xD5] = (char) (0x0150); /*	LATIN CAPITAL LETTER O WITH DOUBLE ACUTE*/
		iso_8859_2_chars[0xD6] = (char) (0x00D6); /*	LATIN CAPITAL LETTER O WITH DIAERESIS*/
		iso_8859_2_chars[0xD7] = (char) (0x00D7); /*	MULTIPLICATION SIGN*/
		iso_8859_2_chars[0xD8] = (char) (0x0158); /*	LATIN CAPITAL LETTER R WITH CARON*/
		iso_8859_2_chars[0xD9] = (char) (0x016E); /*	LATIN CAPITAL LETTER U WITH RING ABOVE*/
		iso_8859_2_chars[0xDA] = (char) (0x00DA); /*	LATIN CAPITAL LETTER U WITH ACUTE*/
		iso_8859_2_chars[0xDB] = (char) (0x0170); /*	LATIN CAPITAL LETTER U WITH DOUBLE ACUTE*/
		iso_8859_2_chars[0xDC] = (char) (0x00DC); /*	LATIN CAPITAL LETTER U WITH DIAERESIS*/
		iso_8859_2_chars[0xDD] = (char) (0x00DD); /*	LATIN CAPITAL LETTER Y WITH ACUTE*/
		iso_8859_2_chars[0xDE] = (char) (0x0162); /*	LATIN CAPITAL LETTER T WITH CEDILLA*/
		iso_8859_2_chars[0xDF] = (char) (0x00DF); /*	LATIN SMALL LETTER SHARP S*/
		iso_8859_2_chars[0xE0] = (char) (0x0155); /*	LATIN SMALL LETTER R WITH ACUTE*/
		iso_8859_2_chars[0xE1] = (char) (0x00E1); /*	LATIN SMALL LETTER A WITH ACUTE*/
		iso_8859_2_chars[0xE2] = (char) (0x00E2); /*	LATIN SMALL LETTER A WITH CIRCUMFLEX*/
		iso_8859_2_chars[0xE3] = (char) (0x0103); /*	LATIN SMALL LETTER A WITH BREVE*/
		iso_8859_2_chars[0xE4] = (char) (0x00E4); /*	LATIN SMALL LETTER A WITH DIAERESIS*/
		iso_8859_2_chars[0xE5] = (char) (0x013A); /*	LATIN SMALL LETTER L WITH ACUTE*/
		iso_8859_2_chars[0xE6] = (char) (0x0107); /*	LATIN SMALL LETTER C WITH ACUTE*/
		iso_8859_2_chars[0xE7] = (char) (0x00E7); /*	LATIN SMALL LETTER C WITH CEDILLA*/
		iso_8859_2_chars[0xE8] = (char) (0x010D); /*	LATIN SMALL LETTER C WITH CARON*/
		iso_8859_2_chars[0xE9] = (char) (0x00E9); /*	LATIN SMALL LETTER E WITH ACUTE*/
		iso_8859_2_chars[0xEA] = (char) (0x0119); /*	LATIN SMALL LETTER E WITH OGONEK*/
		iso_8859_2_chars[0xEB] = (char) (0x00EB); /*	LATIN SMALL LETTER E WITH DIAERESIS*/
		iso_8859_2_chars[0xEC] = (char) (0x011B); /*	LATIN SMALL LETTER E WITH CARON*/
		iso_8859_2_chars[0xED] = (char) (0x00ED); /*	LATIN SMALL LETTER I WITH ACUTE*/
		iso_8859_2_chars[0xEE] = (char) (0x00EE); /*	LATIN SMALL LETTER I WITH CIRCUMFLEX*/
		iso_8859_2_chars[0xEF] = (char) (0x010F); /*	LATIN SMALL LETTER D WITH CARON*/
		iso_8859_2_chars[0xF0] = (char) (0x0111); /*	LATIN SMALL LETTER D WITH STROKE*/
		iso_8859_2_chars[0xF1] = (char) (0x0144); /*	LATIN SMALL LETTER N WITH ACUTE*/
		iso_8859_2_chars[0xF2] = (char) (0x0148); /*	LATIN SMALL LETTER N WITH CARON*/
		iso_8859_2_chars[0xF3] = (char) (0x00F3); /*	LATIN SMALL LETTER O WITH ACUTE*/
		iso_8859_2_chars[0xF4] = (char) (0x00F4); /*	LATIN SMALL LETTER O WITH CIRCUMFLEX*/
		iso_8859_2_chars[0xF5] = (char) (0x0151); /*	LATIN SMALL LETTER O WITH DOUBLE ACUTE*/
		iso_8859_2_chars[0xF6] = (char) (0x00F6); /*	LATIN SMALL LETTER O WITH DIAERESIS*/
		iso_8859_2_chars[0xF7] = (char) (0x00F7); /*	DIVISION SIGN*/
		iso_8859_2_chars[0xF8] = (char) (0x0159); /*	LATIN SMALL LETTER R WITH CARON*/
		iso_8859_2_chars[0xF9] = (char) (0x016F); /*LATIN SMALL LETTER U WITH RING ABOVE*/
		iso_8859_2_chars[0xFA] = (char) (0x00FA); /*	LATIN SMALL LETTER U WITH ACUTE*/
		iso_8859_2_chars[0xFB] = (char) (0x0171); /*	LATIN SMALL LETTER U WITH DOUBLE ACUTE*/
		iso_8859_2_chars[0xFC] = (char) (0x00FC); /*LATIN SMALL LETTER U WITH DIAERESIS*/
		iso_8859_2_chars[0xFD] = (char) (0x00FD); /*	LATIN SMALL LETTER Y WITH ACUTE*/
		iso_8859_2_chars[0xFE] = (char) (0x0163); /*LATIN SMALL LETTER T WITH CEDILLA*/
		iso_8859_2_chars[0xFF] = (char) (0x02D9); /*	DOT ABOVE*/

		iso_8859_2_chars_ready = TRUE;
	}
}
void iso_8859_3_chars_init(){
	int i;
	if (iso_8859_3_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			iso_8859_3_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			iso_8859_3_chars[i] = (char) (0xfffd);
		}
		iso_8859_3_chars[0xA0] = (char) (0x00A0); /*	NO-BREAK SPACE*/
		iso_8859_3_chars[0xA1] = (char) (0x0126); /*	LATIN CAPITAL LETTER H WITH STROKE*/
		iso_8859_3_chars[0xA2] = (char) (0x02D8); /*	BREVE*/
		iso_8859_3_chars[0xA3] = (char) (0x00A3); /*	POUND SIGN*/
		iso_8859_3_chars[0xA4] = (char) (0x00A4); /*	CURRENCY SIGN*/
		iso_8859_3_chars[0xA6] = (char) (0x0124); /*	LATIN CAPITAL LETTER H WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xA7] = (char) (0x00A7); /*	SECTION SIGN*/
		iso_8859_3_chars[0xA8] = (char) (0x00A8); /*	DIAERESIS*/
		iso_8859_3_chars[0xA9] = (char) (0x0130); /*	LATIN CAPITAL LETTER I WITH DOT ABOVE*/
		iso_8859_3_chars[0xAA] = (char) (0x015E); /*	LATIN CAPITAL LETTER S WITH CEDILLA*/
		iso_8859_3_chars[0xAB] = (char) (0x011E); /*	LATIN CAPITAL LETTER G WITH BREVE*/
		iso_8859_3_chars[0xAC] = (char) (0x0134); /*	LATIN CAPITAL LETTER J WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xAD] = (char) (0x00AD); /*	SOFT HYPHEN*/
		iso_8859_3_chars[0xAF] = (char) (0x017B); /*	LATIN CAPITAL LETTER Z WITH DOT ABOVE*/
		iso_8859_3_chars[0xB0] = (char) (0x00B0); /*	DEGREE SIGN*/
		iso_8859_3_chars[0xB1] = (char) (0x0127); /*	LATIN SMALL LETTER H WITH STROKE*/
		iso_8859_3_chars[0xB2] = (char) (0x00B2); /*	SUPERSCRIPT TWO*/
		iso_8859_3_chars[0xB3] = (char) (0x00B3); /*	SUPERSCRIPT THREE*/
		iso_8859_3_chars[0xB4] = (char) (0x00B4); /*	ACUTE ACCENT*/
		iso_8859_3_chars[0xB5] = (char) (0x00B5); /*	MICRO SIGN*/
		iso_8859_3_chars[0xB6] = (char) (0x0125); /*	LATIN SMALL LETTER H WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xB7] = (char) (0x00B7); /*	MIDDLE DOT*/
		iso_8859_3_chars[0xB8] = (char) (0x00B8); /*	CEDILLA*/
		iso_8859_3_chars[0xB9] = (char) (0x0131); /*	LATIN SMALL LETTER DOTLESS I*/
		iso_8859_3_chars[0xBA] = (char) (0x015F); /*	LATIN SMALL LETTER S WITH CEDILLA*/
		iso_8859_3_chars[0xBB] = (char) (0x011F); /*	LATIN SMALL LETTER G WITH BREVE*/
		iso_8859_3_chars[0xBC] = (char) (0x0135); /*	LATIN SMALL LETTER J WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xBD] = (char) (0x00BD); /*	VULGAR FRACTION ONE HALF*/
		iso_8859_3_chars[0xBF] = (char) (0x017C); /*	LATIN SMALL LETTER Z WITH DOT ABOVE*/
		iso_8859_3_chars[0xC0] = (char) (0x00C0); /*	LATIN CAPITAL LETTER A WITH GRAVE*/
		iso_8859_3_chars[0xC1] = (char) (0x00C1); /*	LATIN CAPITAL LETTER A WITH ACUTE*/
		iso_8859_3_chars[0xC2] = (char) (0x00C2); /*	LATIN CAPITAL LETTER A WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xC4] = (char) (0x00C4); /*	LATIN CAPITAL LETTER A WITH DIAERESIS*/
		iso_8859_3_chars[0xC5] = (char) (0x010A); /*	LATIN CAPITAL LETTER C WITH DOT ABOVE*/
		iso_8859_3_chars[0xC6] = (char) (0x0108); /*	LATIN CAPITAL LETTER C WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xC7] = (char) (0x00C7); /*	LATIN CAPITAL LETTER C WITH CEDILLA*/
		iso_8859_3_chars[0xC8] = (char) (0x00C8); /*	LATIN CAPITAL LETTER E WITH GRAVE*/
		iso_8859_3_chars[0xC9] = (char) (0x00C9); /*	LATIN CAPITAL LETTER E WITH ACUTE*/
		iso_8859_3_chars[0xCA] = (char) (0x00CA); /*	LATIN CAPITAL LETTER E WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xCB] = (char) (0x00CB); /*	LATIN CAPITAL LETTER E WITH DIAERESIS*/
		iso_8859_3_chars[0xCC] = (char) (0x00CC); /*	LATIN CAPITAL LETTER I WITH GRAVE*/
		iso_8859_3_chars[0xCD] = (char) (0x00CD); /*	LATIN CAPITAL LETTER I WITH ACUTE*/
		iso_8859_3_chars[0xCE] = (char) (0x00CE); /*	LATIN CAPITAL LETTER I WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xCF] = (char) (0x00CF); /*	LATIN CAPITAL LETTER I WITH DIAERESIS*/
		iso_8859_3_chars[0xD1] = (char) (0x00D1); /*	LATIN CAPITAL LETTER N WITH TILDE*/
		iso_8859_3_chars[0xD2] = (char) (0x00D2); /*	LATIN CAPITAL LETTER O WITH GRAVE*/
		iso_8859_3_chars[0xD3] = (char) (0x00D3); /*	LATIN CAPITAL LETTER O WITH ACUTE*/
		iso_8859_3_chars[0xD4] = (char) (0x00D4); /*	LATIN CAPITAL LETTER O WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xD5] = (char) (0x0120); /*	LATIN CAPITAL LETTER G WITH DOT ABOVE*/
		iso_8859_3_chars[0xD6] = (char) (0x00D6); /*	LATIN CAPITAL LETTER O WITH DIAERESIS*/
		iso_8859_3_chars[0xD7] = (char) (0x00D7); /*	MULTIPLICATION SIGN*/
		iso_8859_3_chars[0xD8] = (char) (0x011C); /*	LATIN CAPITAL LETTER G WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xD9] = (char) (0x00D9); /*	LATIN CAPITAL LETTER U WITH GRAVE*/
		iso_8859_3_chars[0xDA] = (char) (0x00DA); /*	LATIN CAPITAL LETTER U WITH ACUTE*/
		iso_8859_3_chars[0xDB] = (char) (0x00DB); /*	LATIN CAPITAL LETTER U WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xDC] = (char) (0x00DC); /*	LATIN CAPITAL LETTER U WITH DIAERESIS*/
		iso_8859_3_chars[0xDD] = (char) (0x016C); /*	LATIN CAPITAL LETTER U WITH BREVE*/
		iso_8859_3_chars[0xDE] = (char) (0x015C); /*	LATIN CAPITAL LETTER S WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xDF] = (char) (0x00DF); /*	LATIN SMALL LETTER SHARP S*/
		iso_8859_3_chars[0xE0] = (char) (0x00E0); /*	LATIN SMALL LETTER A WITH GRAVE*/
		iso_8859_3_chars[0xE1] = (char) (0x00E1); /*	LATIN SMALL LETTER A WITH ACUTE*/
		iso_8859_3_chars[0xE2] = (char) (0x00E2); /*	LATIN SMALL LETTER A WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xE4] = (char) (0x00E4); /*	LATIN SMALL LETTER A WITH DIAERESIS*/
		iso_8859_3_chars[0xE5] = (char) (0x010B); /*	LATIN SMALL LETTER C WITH DOT ABOVE*/
		iso_8859_3_chars[0xE6] = (char) (0x0109); /*	LATIN SMALL LETTER C WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xE7] = (char) (0x00E7); /*	LATIN SMALL LETTER C WITH CEDILLA*/
		iso_8859_3_chars[0xE8] = (char) (0x00E8); /*	LATIN SMALL LETTER E WITH GRAVE*/
		iso_8859_3_chars[0xE9] = (char) (0x00E9); /*	LATIN SMALL LETTER E WITH ACUTE*/
		iso_8859_3_chars[0xEA] = (char) (0x00EA); /*	LATIN SMALL LETTER E WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xEB] = (char) (0x00EB); /*	LATIN SMALL LETTER E WITH DIAERESIS*/
		iso_8859_3_chars[0xEC] = (char) (0x00EC); /*	LATIN SMALL LETTER I WITH GRAVE*/
		iso_8859_3_chars[0xED] = (char) (0x00ED); /*	LATIN SMALL LETTER I WITH ACUTE*/
		iso_8859_3_chars[0xEE] = (char) (0x00EE); /*	LATIN SMALL LETTER I WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xEF] = (char) (0x00EF); /*	LATIN SMALL LETTER I WITH DIAERESIS*/
		iso_8859_3_chars[0xF1] = (char) (0x00F1); /*	LATIN SMALL LETTER N WITH TILDE*/
		iso_8859_3_chars[0xF2] = (char) (0x00F2); /*	LATIN SMALL LETTER O WITH GRAVE*/
		iso_8859_3_chars[0xF3] = (char) (0x00F3); /*	LATIN SMALL LETTER O WITH ACUTE*/
		iso_8859_3_chars[0xF4] = (char) (0x00F4); /*	LATIN SMALL LETTER O WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xF5] = (char) (0x0121); /*	LATIN SMALL LETTER G WITH DOT ABOVE*/
		iso_8859_3_chars[0xF6] = (char) (0x00F6); /*	LATIN SMALL LETTER O WITH DIAERESIS*/
		iso_8859_3_chars[0xF7] = (char) (0x00F7); /*	DIVISION SIGN*/
		iso_8859_3_chars[0xF8] = (char) (0x011D); /*	LATIN SMALL LETTER G WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xF9] = (char) (0x00F9); /*	LATIN SMALL LETTER U WITH GRAVE*/
		iso_8859_3_chars[0xFA] = (char) (0x00FA); /*	LATIN SMALL LETTER U WITH ACUTE*/
		iso_8859_3_chars[0xFB] = (char) (0x00FB); /*	LATIN SMALL LETTER U WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xFC] = (char) (0x00FC); /*	LATIN SMALL LETTER U WITH DIAERESIS*/
		iso_8859_3_chars[0xFD] = (char) (0x016D); /*	LATIN SMALL LETTER U WITH BREVE*/
		iso_8859_3_chars[0xFE] = (char) (0x015D); /*	LATIN SMALL LETTER S WITH CIRCUMFLEX*/
		iso_8859_3_chars[0xFF] = (char) (0x02D9); /**/
		iso_8859_3_chars_ready = TRUE;
	}
}
void iso_8859_4_chars_init(){
	int i;
	if (iso_8859_4_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			iso_8859_4_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			iso_8859_4_chars[i] = (char) (0xfffd);
		}

		iso_8859_4_chars[0xA0] = (char) (0x00A0);
		iso_8859_4_chars[0xA1] = (char) (0x0104);
		iso_8859_4_chars[0xA2] = (char) (0x0138);
		iso_8859_4_chars[0xA3] = (char) (0x0156);
		iso_8859_4_chars[0xA4] = (char) (0x00A4);
		iso_8859_4_chars[0xA5] = (char) (0x0128);
		iso_8859_4_chars[0xA6] = (char) (0x013B);
		iso_8859_4_chars[0xA7] = (char) (0x00A7);
		iso_8859_4_chars[0xA8] = (char) (0x00A8);
		iso_8859_4_chars[0xA9] = (char) (0x0160);
		iso_8859_4_chars[0xAA] = (char) (0x0112);
		iso_8859_4_chars[0xAB] = (char) (0x0122);
		iso_8859_4_chars[0xAC] = (char) (0x0166);
		iso_8859_4_chars[0xAD] = (char) (0x00AD);
		iso_8859_4_chars[0xAE] = (char) (0x017D);
		iso_8859_4_chars[0xAF] = (char) (0x00AF);
		iso_8859_4_chars[0xB0] = (char) (0x00B0);
		iso_8859_4_chars[0xB1] = (char) (0x0105);
		iso_8859_4_chars[0xB2] = (char) (0x02DB);
		iso_8859_4_chars[0xB3] = (char) (0x0157);
		iso_8859_4_chars[0xB4] = (char) (0x00B4);
		iso_8859_4_chars[0xB5] = (char) (0x0129);
		iso_8859_4_chars[0xB6] = (char) (0x013C);
		iso_8859_4_chars[0xB7] = (char) (0x02C7);
		iso_8859_4_chars[0xB8] = (char) (0x00B8);
		iso_8859_4_chars[0xB9] = (char) (0x0161);
		iso_8859_4_chars[0xBA] = (char) (0x0113);
		iso_8859_4_chars[0xBB] = (char) (0x0123);
		iso_8859_4_chars[0xBC] = (char) (0x0167);
		iso_8859_4_chars[0xBD] = (char) (0x014A);
		iso_8859_4_chars[0xBE] = (char) (0x017E);
		iso_8859_4_chars[0xBF] = (char) (0x014B);
		iso_8859_4_chars[0xC0] = (char) (0x0100);
		iso_8859_4_chars[0xC1] = (char) (0x00C1);
		iso_8859_4_chars[0xC2] = (char) (0x00C2);
		iso_8859_4_chars[0xC3] = (char) (0x00C3);
		iso_8859_4_chars[0xC4] = (char) (0x00C4);
		iso_8859_4_chars[0xC5] = (char) (0x00C5);
		iso_8859_4_chars[0xC6] = (char) (0x00C6);
		iso_8859_4_chars[0xC7] = (char) (0x012E);
		iso_8859_4_chars[0xC8] = (char) (0x010C);
		iso_8859_4_chars[0xC9] = (char) (0x00C9);
		iso_8859_4_chars[0xCA] = (char) (0x0118);
		iso_8859_4_chars[0xCB] = (char) (0x00CB);
		iso_8859_4_chars[0xCC] = (char) (0x0116);
		iso_8859_4_chars[0xCD] = (char) (0x00CD);
		iso_8859_4_chars[0xCE] = (char) (0x00CE);
		iso_8859_4_chars[0xCF] = (char) (0x012A);
		iso_8859_4_chars[0xD0] = (char) (0x0110);
		iso_8859_4_chars[0xD1] = (char) (0x0145);
		iso_8859_4_chars[0xD2] = (char) (0x014C);
		iso_8859_4_chars[0xD3] = (char) (0x0136);
		iso_8859_4_chars[0xD4] = (char) (0x00D4);
		iso_8859_4_chars[0xD5] = (char) (0x00D5);
		iso_8859_4_chars[0xD6] = (char) (0x00D6);
		iso_8859_4_chars[0xD7] = (char) (0x00D7);
		iso_8859_4_chars[0xD8] = (char) (0x00D8);
		iso_8859_4_chars[0xD9] = (char) (0x0172);
		iso_8859_4_chars[0xDA] = (char) (0x00DA);
		iso_8859_4_chars[0xDB] = (char) (0x00DB);
		iso_8859_4_chars[0xDC] = (char) (0x00DC);
		iso_8859_4_chars[0xDD] = (char) (0x0168);
		iso_8859_4_chars[0xDE] = (char) (0x016A);
		iso_8859_4_chars[0xDF] = (char) (0x00DF);
		iso_8859_4_chars[0xE0] = (char) (0x0101);
		iso_8859_4_chars[0xE1] = (char) (0x00E1);
		iso_8859_4_chars[0xE2] = (char) (0x00E2);
		iso_8859_4_chars[0xE3] = (char) (0x00E3);
		iso_8859_4_chars[0xE4] = (char) (0x00E4);
		iso_8859_4_chars[0xE5] = (char) (0x00E5);
		iso_8859_4_chars[0xE6] = (char) (0x00E6);
		iso_8859_4_chars[0xE7] = (char) (0x012F);
		iso_8859_4_chars[0xE8] = (char) (0x010D);
		iso_8859_4_chars[0xE9] = (char) (0x00E9);
		iso_8859_4_chars[0xEA] = (char) (0x0119);
		iso_8859_4_chars[0xEB] = (char) (0x00EB);
		iso_8859_4_chars[0xEC] = (char) (0x0117);
		iso_8859_4_chars[0xED] = (char) (0x00ED);
		iso_8859_4_chars[0xEE] = (char) (0x00EE);
		iso_8859_4_chars[0xEF] = (char) (0x012B);
		iso_8859_4_chars[0xF0] = (char) (0x0111);
		iso_8859_4_chars[0xF1] = (char) (0x0146);
		iso_8859_4_chars[0xF2] = (char) (0x014D);
		iso_8859_4_chars[0xF3] = (char) (0x0137);
		iso_8859_4_chars[0xF4] = (char) (0x00F4);
		iso_8859_4_chars[0xF5] = (char) (0x00F5);
		iso_8859_4_chars[0xF6] = (char) (0x00F6);
		iso_8859_4_chars[0xF7] = (char) (0x00F7);
		iso_8859_4_chars[0xF8] = (char) (0x00F8);
		iso_8859_4_chars[0xF9] = (char) (0x0173);
		iso_8859_4_chars[0xFA] = (char) (0x00FA);
		iso_8859_4_chars[0xFB] = (char) (0x00FB);
		iso_8859_4_chars[0xFC] = (char) (0x00FC);
		iso_8859_4_chars[0xFD] = (char) (0x0169);
		iso_8859_4_chars[0xFE] = (char) (0x016B);
		iso_8859_4_chars[0xFF] = (char) (0x02D9);
		iso_8859_4_chars_ready = TRUE;
	}
}
void iso_8859_5_chars_init(){
	int i;
	if (iso_8859_5_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			iso_8859_5_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			iso_8859_5_chars[i] = (char) (0xfffd);
		}
		iso_8859_5_chars[0xA0] = (char) (0x00A0);
		iso_8859_5_chars[0xA1] = (char) (0x0401);
		iso_8859_5_chars[0xA2] = (char) (0x0402);
		iso_8859_5_chars[0xA3] = (char) (0x0403);
		iso_8859_5_chars[0xA4] = (char) (0x0404);
		iso_8859_5_chars[0xA5] = (char) (0x0405);
		iso_8859_5_chars[0xA6] = (char) (0x0406);
		iso_8859_5_chars[0xA7] = (char) (0x0407);
		iso_8859_5_chars[0xA8] = (char) (0x0408);
		iso_8859_5_chars[0xA9] = (char) (0x0409);
		iso_8859_5_chars[0xAA] = (char) (0x040A);
		iso_8859_5_chars[0xAB] = (char) (0x040B);
		iso_8859_5_chars[0xAC] = (char) (0x040C);
		iso_8859_5_chars[0xAD] = (char) (0x00AD);
		iso_8859_5_chars[0xAE] = (char) (0x040E);
		iso_8859_5_chars[0xAF] = (char) (0x040F);
		iso_8859_5_chars[0xB0] = (char) (0x0410);
		iso_8859_5_chars[0xB1] = (char) (0x0411);
		iso_8859_5_chars[0xB2] = (char) (0x0412);
		iso_8859_5_chars[0xB3] = (char) (0x0413);
		iso_8859_5_chars[0xB4] = (char) (0x0414);
		iso_8859_5_chars[0xB5] = (char) (0x0415);
		iso_8859_5_chars[0xB6] = (char) (0x0416);
		iso_8859_5_chars[0xB7] = (char) (0x0417);
		iso_8859_5_chars[0xB8] = (char) (0x0418);
		iso_8859_5_chars[0xB9] = (char) (0x0419);
		iso_8859_5_chars[0xBA] = (char) (0x041A);
		iso_8859_5_chars[0xBB] = (char) (0x041B);
		iso_8859_5_chars[0xBC] = (char) (0x041C);
		iso_8859_5_chars[0xBD] = (char) (0x041D);
		iso_8859_5_chars[0xBE] = (char) (0x041E);
		iso_8859_5_chars[0xBF] = (char) (0x041F);
		iso_8859_5_chars[0xC0] = (char) (0x0420);
		iso_8859_5_chars[0xC1] = (char) (0x0421);
		iso_8859_5_chars[0xC2] = (char) (0x0422);
		iso_8859_5_chars[0xC3] = (char) (0x0423);
		iso_8859_5_chars[0xC4] = (char) (0x0424);
		iso_8859_5_chars[0xC5] = (char) (0x0425);
		iso_8859_5_chars[0xC6] = (char) (0x0426);
		iso_8859_5_chars[0xC7] = (char) (0x0427);
		iso_8859_5_chars[0xC8] = (char) (0x0428);
		iso_8859_5_chars[0xC9] = (char) (0x0429);
		iso_8859_5_chars[0xCA] = (char) (0x042A);
		iso_8859_5_chars[0xCB] = (char) (0x042B);
		iso_8859_5_chars[0xCC] = (char) (0x042C);
		iso_8859_5_chars[0xCD] = (char) (0x042D);
		iso_8859_5_chars[0xCE] = (char) (0x042E);
		iso_8859_5_chars[0xCF] = (char) (0x042F);
		iso_8859_5_chars[0xD0] = (char) (0x0430);
		iso_8859_5_chars[0xD1] = (char) (0x0431);
		iso_8859_5_chars[0xD2] = (char) (0x0432);
		iso_8859_5_chars[0xD3] = (char) (0x0433);
		iso_8859_5_chars[0xD4] = (char) (0x0434);
		iso_8859_5_chars[0xD5] = (char) (0x0435);
		iso_8859_5_chars[0xD6] = (char) (0x0436);
		iso_8859_5_chars[0xD7] = (char) (0x0437);
		iso_8859_5_chars[0xD8] = (char) (0x0438);
		iso_8859_5_chars[0xD9] = (char) (0x0439);
		iso_8859_5_chars[0xDA] = (char) (0x043A);
		iso_8859_5_chars[0xDB] = (char) (0x043B);
		iso_8859_5_chars[0xDC] = (char) (0x043C);
		iso_8859_5_chars[0xDD] = (char) (0x043D);
		iso_8859_5_chars[0xDE] = (char) (0x043E);
		iso_8859_5_chars[0xDF] = (char) (0x043F);
		iso_8859_5_chars[0xE0] = (char) (0x0440);
		iso_8859_5_chars[0xE1] = (char) (0x0441);
		iso_8859_5_chars[0xE2] = (char) (0x0442);
		iso_8859_5_chars[0xE3] = (char) (0x0443);
		iso_8859_5_chars[0xE4] = (char) (0x0444);
		iso_8859_5_chars[0xE5] = (char) (0x0445);
		iso_8859_5_chars[0xE6] = (char) (0x0446);
		iso_8859_5_chars[0xE7] = (char) (0x0447);
		iso_8859_5_chars[0xE8] = (char) (0x0448);
		iso_8859_5_chars[0xE9] = (char) (0x0449);
		iso_8859_5_chars[0xEA] = (char) (0x044A);
		iso_8859_5_chars[0xEB] = (char) (0x044B);
		iso_8859_5_chars[0xEC] = (char) (0x044C);
		iso_8859_5_chars[0xED] = (char) (0x044D);
		iso_8859_5_chars[0xEE] = (char) (0x044E);
		iso_8859_5_chars[0xEF] = (char) (0x044F);
		iso_8859_5_chars[0xF0] = (char) (0x2116);
		iso_8859_5_chars[0xF1] = (char) (0x0451);
		iso_8859_5_chars[0xF2] = (char) (0x0452);
		iso_8859_5_chars[0xF3] = (char) (0x0453);
		iso_8859_5_chars[0xF4] = (char) (0x0454);
		iso_8859_5_chars[0xF5] = (char) (0x0455);
		iso_8859_5_chars[0xF6] = (char) (0x0456);
		iso_8859_5_chars[0xF7] = (char) (0x0457);
		iso_8859_5_chars[0xF8] = (char) (0x0458);
		iso_8859_5_chars[0xF9] = (char) (0x0459);
		iso_8859_5_chars[0xFA] = (char) (0x045A);
		iso_8859_5_chars[0xFB] = (char) (0x045B);
		iso_8859_5_chars[0xFC] = (char) (0x045C);
		iso_8859_5_chars[0xFD] = (char) (0x00A7);
		iso_8859_5_chars[0xFE] = (char) (0x045E);
		iso_8859_5_chars[0xFF] = (char) (0x045F);
		iso_8859_5_chars_ready = TRUE;
	}
}
void iso_8859_6_chars_init(){
	int i;
	if (iso_8859_6_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			iso_8859_6_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			iso_8859_6_chars[i] = (char) (0xfffd);
		}
		iso_8859_6_chars[0xA0] = (char) (0x00A0);
		iso_8859_6_chars[0xA4] = (char) (0x00A4);
		iso_8859_6_chars[0xAC] = (char) (0x060C);
		iso_8859_6_chars[0xAD] = (char) (0x00AD);
		iso_8859_6_chars[0xBB] = (char) (0x061B);
		iso_8859_6_chars[0xBF] = (char) (0x061F);
		iso_8859_6_chars[0xC1] = (char) (0x0621);
		iso_8859_6_chars[0xC2] = (char) (0x0622);
		iso_8859_6_chars[0xC3] = (char) (0x0623);
		iso_8859_6_chars[0xC4] = (char) (0x0624);
		iso_8859_6_chars[0xC5] = (char) (0x0625);
		iso_8859_6_chars[0xC6] = (char) (0x0626);
		iso_8859_6_chars[0xC7] = (char) (0x0627);
		iso_8859_6_chars[0xC8] = (char) (0x0628);
		iso_8859_6_chars[0xC9] = (char) (0x0629);
		iso_8859_6_chars[0xCA] = (char) (0x062A);
		iso_8859_6_chars[0xCB] = (char) (0x062B);
		iso_8859_6_chars[0xCC] = (char) (0x062C);
		iso_8859_6_chars[0xCD] = (char) (0x062D);
		iso_8859_6_chars[0xCE] = (char) (0x062E);
		iso_8859_6_chars[0xCF] = (char) (0x062F);
		iso_8859_6_chars[0xD0] = (char) (0x0630);
		iso_8859_6_chars[0xD1] = (char) (0x0631);
		iso_8859_6_chars[0xD2] = (char) (0x0632);
		iso_8859_6_chars[0xD3] = (char) (0x0633);
		iso_8859_6_chars[0xD4] = (char) (0x0634);
		iso_8859_6_chars[0xD5] = (char) (0x0635);
		iso_8859_6_chars[0xD6] = (char) (0x0636);
		iso_8859_6_chars[0xD7] = (char) (0x0637);
		iso_8859_6_chars[0xD8] = (char) (0x0638);
		iso_8859_6_chars[0xD9] = (char) (0x0639);
		iso_8859_6_chars[0xDA] = (char) (0x063A);
		iso_8859_6_chars[0xE0] = (char) (0x0640);
		iso_8859_6_chars[0xE1] = (char) (0x0641);
		iso_8859_6_chars[0xE2] = (char) (0x0642);
		iso_8859_6_chars[0xE3] = (char) (0x0643);
		iso_8859_6_chars[0xE4] = (char) (0x0644);
		iso_8859_6_chars[0xE5] = (char) (0x0645);
		iso_8859_6_chars[0xE6] = (char) (0x0646);
		iso_8859_6_chars[0xE7] = (char) (0x0647);
		iso_8859_6_chars[0xE8] = (char) (0x0648);
		iso_8859_6_chars[0xE9] = (char) (0x0649);
		iso_8859_6_chars[0xEA] = (char) (0x064A);
		iso_8859_6_chars[0xEB] = (char) (0x064B);
		iso_8859_6_chars[0xEC] = (char) (0x064C);
		iso_8859_6_chars[0xED] = (char) (0x064D);
		iso_8859_6_chars[0xEE] = (char) (0x064E);
		iso_8859_6_chars[0xEF] = (char) (0x064F);
		iso_8859_6_chars[0xF0] = (char) (0x0650);
		iso_8859_6_chars[0xF1] = (char) (0x0651);
		iso_8859_6_chars[0xF2] = (char) (0x0652);


		iso_8859_6_chars_ready = TRUE;
	}
}
void iso_8859_7_chars_init(){
	int i;
	if (iso_8859_7_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			iso_8859_7_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			iso_8859_7_chars[i] = (char) (0xfffd);
		}
		iso_8859_7_chars[0xA0] = (char) (0x00A0);
		iso_8859_7_chars[0xA1] = (char) (0x02BD);
		iso_8859_7_chars[0xA2] = (char) (0x02BC);
		iso_8859_7_chars[0xA3] = (char) (0x00A3);
		iso_8859_7_chars[0xA6] = (char) (0x00A6);
		iso_8859_7_chars[0xA7] = (char) (0x00A7);
		iso_8859_7_chars[0xA8] = (char) (0x00A8);
		iso_8859_7_chars[0xA9] = (char) (0x00A9);
		iso_8859_7_chars[0xAB] = (char) (0x00AB);
		iso_8859_7_chars[0xAC] = (char) (0x00AC);
		iso_8859_7_chars[0xAD] = (char) (0x00AD);
		iso_8859_7_chars[0xAF] = (char) (0x2015);
		iso_8859_7_chars[0xB0] = (char) (0x00B0);
		iso_8859_7_chars[0xB1] = (char) (0x00B1);
		iso_8859_7_chars[0xB2] = (char) (0x00B2);
		iso_8859_7_chars[0xB3] = (char) (0x00B3);
		iso_8859_7_chars[0xB4] = (char) (0x0384);
		iso_8859_7_chars[0xB5] = (char) (0x0385);
		iso_8859_7_chars[0xB6] = (char) (0x0386);
		iso_8859_7_chars[0xB7] = (char) (0x00B7);
		iso_8859_7_chars[0xB8] = (char) (0x0388);
		iso_8859_7_chars[0xB9] = (char) (0x0389);
		iso_8859_7_chars[0xBA] = (char) (0x038A);
		iso_8859_7_chars[0xBB] = (char) (0x00BB);
		iso_8859_7_chars[0xBC] = (char) (0x038C);
		iso_8859_7_chars[0xBD] = (char) (0x00BD);
		iso_8859_7_chars[0xBE] = (char) (0x038E);
		iso_8859_7_chars[0xBF] = (char) (0x038F);
		iso_8859_7_chars[0xC0] = (char) (0x0390);
		iso_8859_7_chars[0xC1] = (char) (0x0391);
		iso_8859_7_chars[0xC2] = (char) (0x0392);
		iso_8859_7_chars[0xC3] = (char) (0x0393);
		iso_8859_7_chars[0xC4] = (char) (0x0394);
		iso_8859_7_chars[0xC5] = (char) (0x0395);
		iso_8859_7_chars[0xC6] = (char) (0x0396);
		iso_8859_7_chars[0xC7] = (char) (0x0397);
		iso_8859_7_chars[0xC8] = (char) (0x0398);
		iso_8859_7_chars[0xC9] = (char) (0x0399);
		iso_8859_7_chars[0xCA] = (char) (0x039A);
		iso_8859_7_chars[0xCB] = (char) (0x039B);
		iso_8859_7_chars[0xCC] = (char) (0x039C);
		iso_8859_7_chars[0xCD] = (char) (0x039D);
		iso_8859_7_chars[0xCE] = (char) (0x039E);
		iso_8859_7_chars[0xCF] = (char) (0x039F);
		iso_8859_7_chars[0xD0] = (char) (0x03A0);
		iso_8859_7_chars[0xD1] = (char) (0x03A1);
		iso_8859_7_chars[0xD3] = (char) (0x03A3);
		iso_8859_7_chars[0xD4] = (char) (0x03A4);
		iso_8859_7_chars[0xD5] = (char) (0x03A5);
		iso_8859_7_chars[0xD6] = (char) (0x03A6);
		iso_8859_7_chars[0xD7] = (char) (0x03A7);
		iso_8859_7_chars[0xD8] = (char) (0x03A8);
		iso_8859_7_chars[0xD9] = (char) (0x03A9);
		iso_8859_7_chars[0xDA] = (char) (0x03AA);
		iso_8859_7_chars[0xDB] = (char) (0x03AB);
		iso_8859_7_chars[0xDC] = (char) (0x03AC);
		iso_8859_7_chars[0xDD] = (char) (0x03AD);
		iso_8859_7_chars[0xDE] = (char) (0x03AE);
		iso_8859_7_chars[0xDF] = (char) (0x03AF);
		iso_8859_7_chars[0xE0] = (char) (0x03B0);
		iso_8859_7_chars[0xE1] = (char) (0x03B1);
		iso_8859_7_chars[0xE2] = (char) (0x03B2);
		iso_8859_7_chars[0xE3] = (char) (0x03B3);
		iso_8859_7_chars[0xE4] = (char) (0x03B4);
		iso_8859_7_chars[0xE5] = (char) (0x03B5);
		iso_8859_7_chars[0xE6] = (char) (0x03B6);
		iso_8859_7_chars[0xE7] = (char) (0x03B7);
		iso_8859_7_chars[0xE8] = (char) (0x03B8);
		iso_8859_7_chars[0xE9] = (char) (0x03B9);
		iso_8859_7_chars[0xEA] = (char) (0x03BA);
		iso_8859_7_chars[0xEB] = (char) (0x03BB);
		iso_8859_7_chars[0xEC] = (char) (0x03BC);
		iso_8859_7_chars[0xED] = (char) (0x03BD);
		iso_8859_7_chars[0xEE] = (char) (0x03BE);
		iso_8859_7_chars[0xEF] = (char) (0x03BF);
		iso_8859_7_chars[0xF0] = (char) (0x03C0);
		iso_8859_7_chars[0xF1] = (char) (0x03C1);
		iso_8859_7_chars[0xF2] = (char) (0x03C2);
		iso_8859_7_chars[0xF3] = (char) (0x03C3);
		iso_8859_7_chars[0xF4] = (char) (0x03C4);
		iso_8859_7_chars[0xF5] = (char) (0x03C5);
		iso_8859_7_chars[0xF6] = (char) (0x03C6);
		iso_8859_7_chars[0xF7] = (char) (0x03C7);
		iso_8859_7_chars[0xF8] = (char) (0x03C8);
		iso_8859_7_chars[0xF9] = (char) (0x03C9);
		iso_8859_7_chars[0xFA] = (char) (0x03CA);
		iso_8859_7_chars[0xFB] = (char) (0x03CB);
		iso_8859_7_chars[0xFC] = (char) (0x03CC);
		iso_8859_7_chars[0xFD] = (char) (0x03CD);
		iso_8859_7_chars[0xFE] = (char) (0x03CE);


		iso_8859_7_chars_ready = TRUE;
	}
}
void iso_8859_8_chars_init(){
	int i;
	if (iso_8859_8_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			iso_8859_8_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			iso_8859_8_chars[i] = (char) (0xfffd);
		}
		iso_8859_8_chars[0xAA] = (char) (0x00D7);
		iso_8859_8_chars[0xAB] = (char) (0x00AB);
		iso_8859_8_chars[0xAC] = (char) (0x00AC);
		iso_8859_8_chars[0xAD] = (char) (0x00AD);
		iso_8859_8_chars[0xAE] = (char) (0x00AE);
		iso_8859_8_chars[0xAF] = (char) (0x203E);
		iso_8859_8_chars[0xB0] = (char) (0x00B0);
		iso_8859_8_chars[0xB1] = (char) (0x00B1);
		iso_8859_8_chars[0xB2] = (char) (0x00B2);
		iso_8859_8_chars[0xB3] = (char) (0x00B3);
		iso_8859_8_chars[0xB4] = (char) (0x00B4);
		iso_8859_8_chars[0xB5] = (char) (0x00B5);
		iso_8859_8_chars[0xB6] = (char) (0x00B6);
		iso_8859_8_chars[0xB7] = (char) (0x00B7);
		iso_8859_8_chars[0xB8] = (char) (0x00B8);
		iso_8859_8_chars[0xB9] = (char) (0x00B9);
		iso_8859_8_chars[0xBA] = (char) (0x00F7);
		iso_8859_8_chars[0xBB] = (char) (0x00BB);
		iso_8859_8_chars[0xBC] = (char) (0x00BC);
		iso_8859_8_chars[0xBD] = (char) (0x00BD);
		iso_8859_8_chars[0xBE] = (char) (0x00BE);
		iso_8859_8_chars[0xDF] = (char) (0x2017);
		iso_8859_8_chars[0xE0] = (char) (0x05D0);
		iso_8859_8_chars[0xE1] = (char) (0x05D1);
		iso_8859_8_chars[0xE2] = (char) (0x05D2);
		iso_8859_8_chars[0xE3] = (char) (0x05D3);
		iso_8859_8_chars[0xE4] = (char) (0x05D4);
		iso_8859_8_chars[0xE5] = (char) (0x05D5);
		iso_8859_8_chars[0xE6] = (char) (0x05D6);
		iso_8859_8_chars[0xE7] = (char) (0x05D7);
		iso_8859_8_chars[0xE8] = (char) (0x05D8);
		iso_8859_8_chars[0xE9] = (char) (0x05D9);
		iso_8859_8_chars[0xEA] = (char) (0x05DA);
		iso_8859_8_chars[0xEB] = (char) (0x05DB);
		iso_8859_8_chars[0xEC] = (char) (0x05DC);
		iso_8859_8_chars[0xED] = (char) (0x05DD);
		iso_8859_8_chars[0xEE] = (char) (0x05DE);
		iso_8859_8_chars[0xEF] = (char) (0x05DF);
		iso_8859_8_chars[0xF0] = (char) (0x05E0);
		iso_8859_8_chars[0xF1] = (char) (0x05E1);
		iso_8859_8_chars[0xF2] = (char) (0x05E2);
		iso_8859_8_chars[0xF3] = (char) (0x05E3);
		iso_8859_8_chars[0xF4] = (char) (0x05E4);
		iso_8859_8_chars[0xF5] = (char) (0x05E5);
		iso_8859_8_chars[0xF6] = (char) (0x05E6);
		iso_8859_8_chars[0xF7] = (char) (0x05E7);
		iso_8859_8_chars[0xF8] = (char) (0x05E8);
		iso_8859_8_chars[0xF9] = (char) (0x05E9);
		iso_8859_8_chars[0xFA] = (char) (0x05EA);


		iso_8859_8_chars_ready = TRUE;
	}
}
void iso_8859_9_chars_init(){
	int i;
	if (iso_8859_9_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			iso_8859_9_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			iso_8859_9_chars[i] = (char) (0xfffd);
		}

		iso_8859_9_chars[0xA0] = (char) (0x00A0);
		iso_8859_9_chars[0xA1] = (char) (0x00A1);
		iso_8859_9_chars[0xA2] = (char) (0x00A2);
		iso_8859_9_chars[0xA3] = (char) (0x00A3);
		iso_8859_9_chars[0xA4] = (char) (0x00A4);
		iso_8859_9_chars[0xA5] = (char) (0x00A5);
		iso_8859_9_chars[0xA6] = (char) (0x00A6);
		iso_8859_9_chars[0xA7] = (char) (0x00A7);
		iso_8859_9_chars[0xA8] = (char) (0x00A8);
		iso_8859_9_chars[0xA9] = (char) (0x00A9);
		iso_8859_9_chars[0xAA] = (char) (0x00AA);
		iso_8859_9_chars[0xAB] = (char) (0x00AB);
		iso_8859_9_chars[0xAC] = (char) (0x00AC);
		iso_8859_9_chars[0xAD] = (char) (0x00AD);
		iso_8859_9_chars[0xAE] = (char) (0x00AE);
		iso_8859_9_chars[0xAF] = (char) (0x00AF);
		iso_8859_9_chars[0xB0] = (char) (0x00B0);
		iso_8859_9_chars[0xB1] = (char) (0x00B1);
		iso_8859_9_chars[0xB2] = (char) (0x00B2);
		iso_8859_9_chars[0xB3] = (char) (0x00B3);
		iso_8859_9_chars[0xB4] = (char) (0x00B4);
		iso_8859_9_chars[0xB5] = (char) (0x00B5);
		iso_8859_9_chars[0xB6] = (char) (0x00B6);
		iso_8859_9_chars[0xB7] = (char) (0x00B7);
		iso_8859_9_chars[0xB8] = (char) (0x00B8);
		iso_8859_9_chars[0xB9] = (char) (0x00B9);
		iso_8859_9_chars[0xBA] = (char) (0x00BA);
		iso_8859_9_chars[0xBB] = (char) (0x00BB);
		iso_8859_9_chars[0xBC] = (char) (0x00BC);
		iso_8859_9_chars[0xBD] = (char) (0x00BD);
		iso_8859_9_chars[0xBE] = (char) (0x00BE);
		iso_8859_9_chars[0xBF] = (char) (0x00BF);
		iso_8859_9_chars[0xC0] = (char) (0x00C0);
		iso_8859_9_chars[0xC1] = (char) (0x00C1);
		iso_8859_9_chars[0xC2] = (char) (0x00C2);
		iso_8859_9_chars[0xC3] = (char) (0x00C3);
		iso_8859_9_chars[0xC4] = (char) (0x00C4);
		iso_8859_9_chars[0xC5] = (char) (0x00C5);
		iso_8859_9_chars[0xC6] = (char) (0x00C6);
		iso_8859_9_chars[0xC7] = (char) (0x00C7);
		iso_8859_9_chars[0xC8] = (char) (0x00C8);
		iso_8859_9_chars[0xC9] = (char) (0x00C9);
		iso_8859_9_chars[0xCA] = (char) (0x00CA);
		iso_8859_9_chars[0xCB] = (char) (0x00CB);
		iso_8859_9_chars[0xCC] = (char) (0x00CC);
		iso_8859_9_chars[0xCD] = (char) (0x00CD);
		iso_8859_9_chars[0xCE] = (char) (0x00CE);
		iso_8859_9_chars[0xCF] = (char) (0x00CF);
		iso_8859_9_chars[0xD0] = (char) (0x011E);
		iso_8859_9_chars[0xD1] = (char) (0x00D1);
		iso_8859_9_chars[0xD2] = (char) (0x00D2);
		iso_8859_9_chars[0xD3] = (char) (0x00D3);
		iso_8859_9_chars[0xD4] = (char) (0x00D4);
		iso_8859_9_chars[0xD5] = (char) (0x00D5);
		iso_8859_9_chars[0xD6] = (char) (0x00D6);
		iso_8859_9_chars[0xD7] = (char) (0x00D7);
		iso_8859_9_chars[0xD8] = (char) (0x00D8);
		iso_8859_9_chars[0xD9] = (char) (0x00D9);
		iso_8859_9_chars[0xDA] = (char) (0x00DA);
		iso_8859_9_chars[0xDB] = (char) (0x00DB);
		iso_8859_9_chars[0xDC] = (char) (0x00DC);
		iso_8859_9_chars[0xDD] = (char) (0x0130);
		iso_8859_9_chars[0xDE] = (char) (0x015E);
		iso_8859_9_chars[0xDF] = (char) (0x00DF);
		iso_8859_9_chars[0xE0] = (char) (0x00E0);
		iso_8859_9_chars[0xE1] = (char) (0x00E1);
		iso_8859_9_chars[0xE2] = (char) (0x00E2);
		iso_8859_9_chars[0xE3] = (char) (0x00E3);
		iso_8859_9_chars[0xE4] = (char) (0x00E4);
		iso_8859_9_chars[0xE5] = (char) (0x00E5);
		iso_8859_9_chars[0xE6] = (char) (0x00E6);
		iso_8859_9_chars[0xE7] = (char) (0x00E7);
		iso_8859_9_chars[0xE8] = (char) (0x00E8);
		iso_8859_9_chars[0xE9] = (char) (0x00E9);
		iso_8859_9_chars[0xEA] = (char) (0x00EA);
		iso_8859_9_chars[0xEB] = (char) (0x00EB);
		iso_8859_9_chars[0xEC] = (char) (0x00EC);
		iso_8859_9_chars[0xED] = (char) (0x00ED);
		iso_8859_9_chars[0xEE] = (char) (0x00EE);
		iso_8859_9_chars[0xEF] = (char) (0x00EF);
		iso_8859_9_chars[0xF0] = (char) (0x011F);
		iso_8859_9_chars[0xF1] = (char) (0x00F1);
		iso_8859_9_chars[0xF2] = (char) (0x00F2);
		iso_8859_9_chars[0xF3] = (char) (0x00F3);
		iso_8859_9_chars[0xF4] = (char) (0x00F4);
		iso_8859_9_chars[0xF5] = (char) (0x00F5);
		iso_8859_9_chars[0xF6] = (char) (0x00F6);
		iso_8859_9_chars[0xF7] = (char) (0x00F7);
		iso_8859_9_chars[0xF8] = (char) (0x00F8);
		iso_8859_9_chars[0xF9] = (char) (0x00F9);
		iso_8859_9_chars[0xFA] = (char) (0x00FA);
		iso_8859_9_chars[0xFB] = (char) (0x00FB);
		iso_8859_9_chars[0xFC] = (char) (0x00FC);
		iso_8859_9_chars[0xFD] = (char) (0x0131);
		iso_8859_9_chars[0xFE] = (char) (0x015F);
		iso_8859_9_chars[0xFF] = (char) (0x00FF);


		iso_8859_9_chars_ready = TRUE;
	}
}
void iso_8859_10_chars_init(){
	int i;
	if (iso_8859_10_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			iso_8859_10_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			iso_8859_10_chars[i] = (char) (0xfffd);
		}
		iso_8859_10_chars[0xA0] = (char) (0x00A0);
		iso_8859_10_chars[0xA1] = (char) (0x0104);
		iso_8859_10_chars[0xA2] = (char) (0x0112);
		iso_8859_10_chars[0xA3] = (char) (0x0122);
		iso_8859_10_chars[0xA4] = (char) (0x012A);
		iso_8859_10_chars[0xA5] = (char) (0x0128);
		iso_8859_10_chars[0xA6] = (char) (0x0136);
		iso_8859_10_chars[0xA7] = (char) (0x00A7);
		iso_8859_10_chars[0xA8] = (char) (0x013B);
		iso_8859_10_chars[0xA9] = (char) (0x0110);
		iso_8859_10_chars[0xAA] = (char) (0x0160);
		iso_8859_10_chars[0xAB] = (char) (0x0166);
		iso_8859_10_chars[0xAC] = (char) (0x017D);
		iso_8859_10_chars[0xAD] = (char) (0x00AD);
		iso_8859_10_chars[0xAE] = (char) (0x016A);
		iso_8859_10_chars[0xAF] = (char) (0x014A);
		iso_8859_10_chars[0xB0] = (char) (0x00B0);
		iso_8859_10_chars[0xB1] = (char) (0x0105);
		iso_8859_10_chars[0xB2] = (char) (0x0113);
		iso_8859_10_chars[0xB3] = (char) (0x0123);
		iso_8859_10_chars[0xB4] = (char) (0x012B);
		iso_8859_10_chars[0xB5] = (char) (0x0129);
		iso_8859_10_chars[0xB6] = (char) (0x0137);
		iso_8859_10_chars[0xB7] = (char) (0x00B7);
		iso_8859_10_chars[0xB8] = (char) (0x013CA);
		iso_8859_10_chars[0xB9] = (char) (0x0111);
		iso_8859_10_chars[0xBA] = (char) (0x0161);
		iso_8859_10_chars[0xBB] = (char) (0x0167);
		iso_8859_10_chars[0xBC] = (char) (0x017E);
		iso_8859_10_chars[0xBD] = (char) (0x2015);
		iso_8859_10_chars[0xBE] = (char) (0x016B);
		iso_8859_10_chars[0xBF] = (char) (0x014B);
		iso_8859_10_chars[0xC0] = (char) (0x0100);
		iso_8859_10_chars[0xC1] = (char) (0x00C1);
		iso_8859_10_chars[0xC2] = (char) (0x00C2);
		iso_8859_10_chars[0xC3] = (char) (0x00C3);
		iso_8859_10_chars[0xC4] = (char) (0x00C4);
		iso_8859_10_chars[0xC5] = (char) (0x00C5);
		iso_8859_10_chars[0xC6] = (char) (0x00C6);
		iso_8859_10_chars[0xC7] = (char) (0x012E);
		iso_8859_10_chars[0xC8] = (char) (0x010C);
		iso_8859_10_chars[0xC9] = (char) (0x00C9);
		iso_8859_10_chars[0xCA] = (char) (0x0118);
		iso_8859_10_chars[0xCB] = (char) (0x00CB);
		iso_8859_10_chars[0xCC] = (char) (0x0116);
		iso_8859_10_chars[0xCD] = (char) (0x00CD);
		iso_8859_10_chars[0xCE] = (char) (0x00CE);
		iso_8859_10_chars[0xCF] = (char) (0x00CF);
		iso_8859_10_chars[0xD0] = (char) (0x00D0);
		iso_8859_10_chars[0xD1] = (char) (0x0145);
		iso_8859_10_chars[0xD2] = (char) (0x014C);
		iso_8859_10_chars[0xD3] = (char) (0x00D3);
		iso_8859_10_chars[0xD4] = (char) (0x00D4);
		iso_8859_10_chars[0xD5] = (char) (0x00D5);
		iso_8859_10_chars[0xD6] = (char) (0x00D6);
		iso_8859_10_chars[0xD7] = (char) (0x0168);
		iso_8859_10_chars[0xD8] = (char) (0x00D8);
		iso_8859_10_chars[0xD9] = (char) (0x0172);
		iso_8859_10_chars[0xDA] = (char) (0x00DA);
		iso_8859_10_chars[0xDB] = (char) (0x00DB);
		iso_8859_10_chars[0xDC] = (char) (0x00DC);
		iso_8859_10_chars[0xDD] = (char) (0x00DD);
		iso_8859_10_chars[0xDE] = (char) (0x00DE);
		iso_8859_10_chars[0xDF] = (char) (0x00DF);
		iso_8859_10_chars[0xE0] = (char) (0x0101);
		iso_8859_10_chars[0xE1] = (char) (0x00E1);
		iso_8859_10_chars[0xE2] = (char) (0x00E2);
		iso_8859_10_chars[0xE3] = (char) (0x00E3);
		iso_8859_10_chars[0xE4] = (char) (0x00E4);
		iso_8859_10_chars[0xE5] = (char) (0x00E5);
		iso_8859_10_chars[0xE6] = (char) (0x00E6);
		iso_8859_10_chars[0xE7] = (char) (0x012F);
		iso_8859_10_chars[0xE8] = (char) (0x010D);
		iso_8859_10_chars[0xE9] = (char) (0x00E9);
		iso_8859_10_chars[0xEA] = (char) (0x0119);
		iso_8859_10_chars[0xEB] = (char) (0x00EB);
		iso_8859_10_chars[0xEC] = (char) (0x0117);
		iso_8859_10_chars[0xED] = (char) (0x00ED);
		iso_8859_10_chars[0xEE] = (char) (0x00EE);
		iso_8859_10_chars[0xEF] = (char) (0x00EF);
		iso_8859_10_chars[0xF0] = (char) (0x00F0);
		iso_8859_10_chars[0xF1] = (char) (0x0146);
		iso_8859_10_chars[0xF2] = (char) (0x014D);
		iso_8859_10_chars[0xF3] = (char) (0x00F3);
		iso_8859_10_chars[0xF4] = (char) (0x00F4);
		iso_8859_10_chars[0xF5] = (char) (0x00F5);
		iso_8859_10_chars[0xF6] = (char) (0x00F6);
		iso_8859_10_chars[0xF7] = (char) (0x0169);
		iso_8859_10_chars[0xF8] = (char) (0x00F8);
		iso_8859_10_chars[0xF9] = (char) (0x0173);
		iso_8859_10_chars[0xFA] = (char) (0x00FA);
		iso_8859_10_chars[0xFB] = (char) (0x00FB);
		iso_8859_10_chars[0xFC] = (char) (0x00FC);
		iso_8859_10_chars[0xFD] = (char) (0x00FD);
		iso_8859_10_chars[0xFE] = (char) (0x00FE);
		iso_8859_10_chars[0xFF] = (char) (0x0138);


		iso_8859_10_chars_ready = TRUE;
	}
}


void iso_8859_11_chars_init(){
	int i;
	if (iso_8859_11_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			iso_8859_11_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			iso_8859_11_chars[i] = (char) (0xfffd);
		}
		iso_8859_11_chars[0xA1] = (char) (0x0E01); //THAI CHARACTER KO KAI
		iso_8859_11_chars[0xA2] = (char) (0x0E02); //THAI CHARACTER KHO KHAI
		iso_8859_11_chars[0xA3] = (char) (0x0E03); //THAI CHARACTER KHO KHUAT
		iso_8859_11_chars[0xA4] = (char) (0x0E04); //THAI CHARACTER KHO KHWAI
		iso_8859_11_chars[0xA5] = (char) (0x0E05); //THAI CHARACTER KHO KHON
		iso_8859_11_chars[0xA6] = (char) (0x0E06); //THAI CHARACTER KHO RAKHANG
		iso_8859_11_chars[0xA7] = (char) (0x0E07); //THAI CHARACTER NGO NGU
		iso_8859_11_chars[0xA8] = (char) (0x0E08); //THAI CHARACTER CHO CHAN
		iso_8859_11_chars[0xA9] = (char) (0x0E09); //THAI CHARACTER CHO CHING
		iso_8859_11_chars[0xAA] = (char) (0x0E0A); //THAI CHARACTER CHO CHANG
		iso_8859_11_chars[0xAB] = (char) (0x0E0B); //THAI CHARACTER SO SO
		iso_8859_11_chars[0xAC] = (char) (0x0E0C); //THAI CHARACTER CHO CHOE
		iso_8859_11_chars[0xAD] = (char) (0x0E0D); //THAI CHARACTER YO YING
		iso_8859_11_chars[0xAE] = (char) (0x0E0E); //THAI CHARACTER DO CHADA
		iso_8859_11_chars[0xAF] = (char) (0x0E0F); //THAI CHARACTER TO PATAK
		iso_8859_11_chars[0xB0] = (char) (0x0E10); //THAI CHARACTER THO THAN
		iso_8859_11_chars[0xB1] = (char) (0x0E11); //THAI CHARACTER THO NANGMONTHO
		iso_8859_11_chars[0xB2] = (char) (0x0E12); //THAI CHARACTER THO PHUTHAO
		iso_8859_11_chars[0xB3] = (char) (0x0E13); //THAI CHARACTER NO NEN
		iso_8859_11_chars[0xB4] = (char) (0x0E14); //THAI CHARACTER DO DEK
		iso_8859_11_chars[0xB5] = (char) (0x0E15); //THAI CHARACTER TO TAO
		iso_8859_11_chars[0xB6] = (char) (0x0E16); //THAI CHARACTER THO THUNG
		iso_8859_11_chars[0xB7] = (char) (0x0E17); //THAI CHARACTER THO THAHAN
		iso_8859_11_chars[0xB8] = (char) (0x0E18); //THAI CHARACTER THO THONG
		iso_8859_11_chars[0xB9] = (char) (0x0E19); //THAI CHARACTER NO NU
		iso_8859_11_chars[0xBA] = (char) (0x0E1A); //THAI CHARACTER BO BAIMAI
		iso_8859_11_chars[0xBB] = (char) (0x0E1B); //THAI CHARACTER PO PLA
		iso_8859_11_chars[0xBC] = (char) (0x0E1C); //THAI CHARACTER PHO PHUNG
		iso_8859_11_chars[0xBD] = (char) (0x0E1D); //THAI CHARACTER FO FA
		iso_8859_11_chars[0xBE] = (char) (0x0E1E); //THAI CHARACTER PHO PHAN
		iso_8859_11_chars[0xBF] = (char) (0x0E1F); //THAI CHARACTER FO FAN
		iso_8859_11_chars[0xC0] = (char) (0x0E20); //THAI CHARACTER PHO SAMPHAO
		iso_8859_11_chars[0xC1] = (char) (0x0E21); //THAI CHARACTER MO MA
		iso_8859_11_chars[0xC2] = (char) (0x0E22); //THAI CHARACTER YO YAK
		iso_8859_11_chars[0xC3] = (char) (0x0E23); //THAI CHARACTER RO RUA
		iso_8859_11_chars[0xC4] = (char) (0x0E24); //THAI CHARACTER RU
		iso_8859_11_chars[0xC5] = (char) (0x0E25); //THAI CHARACTER LO LING
		iso_8859_11_chars[0xC6] = (char) (0x0E26); //THAI CHARACTER LU
		iso_8859_11_chars[0xC7] = (char) (0x0E27); //THAI CHARACTER WO WAEN
		iso_8859_11_chars[0xC8] = (char) (0x0E28); //THAI CHARACTER SO SALA
		iso_8859_11_chars[0xC9] = (char) (0x0E29); //THAI CHARACTER SO RUSI
		iso_8859_11_chars[0xCA] = (char) (0x0E2A); //THAI CHARACTER SO SUA
		iso_8859_11_chars[0xCB] = (char) (0x0E2B); //THAI CHARACTER HO HIP
		iso_8859_11_chars[0xCC] = (char) (0x0E2C); //THAI CHARACTER LO CHULA
		iso_8859_11_chars[0xCD] = (char) (0x0E2D); //THAI CHARACTER O ANG
		iso_8859_11_chars[0xCE] = (char) (0x0E2E); //THAI CHARACTER HO NOKHUK
		iso_8859_11_chars[0xCF] = (char) (0x0E2F); //THAI CHARACTER PAIYANNOI
		iso_8859_11_chars[0xD0] = (char) (0x0E30); //THAI CHARACTER SARA A
		iso_8859_11_chars[0xD1] = (char) (0x0E31); //THAI CHARACTER MAI HAN-AKAT
		iso_8859_11_chars[0xD2] = (char) (0x0E32); //THAI CHARACTER SARA AA
		iso_8859_11_chars[0xD3] = (char) (0x0E33); //THAI CHARACTER SARA AM
		iso_8859_11_chars[0xD4] = (char) (0x0E34); //THAI CHARACTER SARA I
		iso_8859_11_chars[0xD5] = (char) (0x0E35); //THAI CHARACTER SARA II
		iso_8859_11_chars[0xD6] = (char) (0x0E36); //THAI CHARACTER SARA UE
		iso_8859_11_chars[0xD7] = (char) (0x0E37); //THAI CHARACTER SARA UEE
		iso_8859_11_chars[0xD8] = (char) (0x0E38); //THAI CHARACTER SARA U
		iso_8859_11_chars[0xD9] = (char) (0x0E39); //THAI CHARACTER SARA UU
		iso_8859_11_chars[0xDA] = (char) (0x0E3A); //THAI CHARACTER PHINTHU
		iso_8859_11_chars[0xDF] = (char) (0x0E3F); //THAI CURRENCY SYMBOL BAHT
		iso_8859_11_chars[0xE0] = (char) (0x0E40); //THAI CHARACTER SARA E
		iso_8859_11_chars[0xE1] = (char) (0x0E41); //THAI CHARACTER SARA AE
		iso_8859_11_chars[0xE2] = (char) (0x0E42); //THAI CHARACTER SARA O
		iso_8859_11_chars[0xE3] = (char) (0x0E43); //THAI CHARACTER SARA AI MAIMUAN
		iso_8859_11_chars[0xE4] = (char) (0x0E44); //THAI CHARACTER SARA AI MAIMALAI
		iso_8859_11_chars[0xE5] = (char) (0x0E45); //THAI CHARACTER LAKKHANGYAO
		iso_8859_11_chars[0xE6] = (char) (0x0E46); //THAI CHARACTER MAIYAMOK
		iso_8859_11_chars[0xE7] = (char) (0x0E47); //THAI CHARACTER MAITAIKHU
		iso_8859_11_chars[0xE8] = (char) (0x0E48); //THAI CHARACTER MAI EK
		iso_8859_11_chars[0xE9] = (char) (0x0E49); //THAI CHARACTER MAI THO
		iso_8859_11_chars[0xEA] = (char) (0x0E4A); //THAI CHARACTER MAI TRI
		iso_8859_11_chars[0xEB] = (char) (0x0E4B); //THAI CHARACTER MAI CHATTAWA
		iso_8859_11_chars[0xEC] = (char) (0x0E4C); //THAI CHARACTER THANTHAKHAT
		iso_8859_11_chars[0xED] = (char) (0x0E4D); //THAI CHARACTER NIKHAHIT
		iso_8859_11_chars[0xEE] = (char) (0x0E4E); //THAI CHARACTER YAMAKKAN
		iso_8859_11_chars[0xEF] = (char) (0x0E4F); //THAI CHARACTER FONGMAN
		iso_8859_11_chars[0xF0] = (char) (0x0E50); //THAI DIGIT ZERO
		iso_8859_11_chars[0xF1] = (char) (0x0E51); //THAI DIGIT ONE
		iso_8859_11_chars[0xF2] = (char) (0x0E52); //THAI DIGIT TWO
		iso_8859_11_chars[0xF3] = (char) (0x0E53); //THAI DIGIT THREE
		iso_8859_11_chars[0xF4] = (char) (0x0E54); //THAI DIGIT FOUR
		iso_8859_11_chars[0xF5] = (char) (0x0E55); //THAI DIGIT FIVE
		iso_8859_11_chars[0xF6] = (char) (0x0E56); //THAI DIGIT SIX
		iso_8859_11_chars[0xF7] = (char) (0x0E57); //THAI DIGIT SEVEN
		iso_8859_11_chars[0xF8] = (char) (0x0E58); //THAI DIGIT EIGHT
		iso_8859_11_chars[0xF9] = (char) (0x0E59); //THAI DIGIT NINE
		iso_8859_11_chars[0xFA] = (char) (0x0E5A); //THAI CHARACTER ANGKHANKHU
		iso_8859_11_chars[0xFB] = (char) (0x0E5B); //THAI CHARACTER KHOMUT

		iso_8859_11_chars_ready = TRUE;
	}
}


void iso_8859_13_chars_init(){
	int i;
	if (iso_8859_13_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			iso_8859_13_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			iso_8859_13_chars[i] = (char) (0xfffd);
		}
		iso_8859_13_chars[0xA0] = (char) (0x00A0);
		iso_8859_13_chars[0xA1] = (char) (0x201D);
		iso_8859_13_chars[0xA2] = (char) (0x00A2);
		iso_8859_13_chars[0xA3] = (char) (0x00A3);
		iso_8859_13_chars[0xA4] = (char) (0x00A4);
		iso_8859_13_chars[0xA5] = (char) (0x201E);
		iso_8859_13_chars[0xA6] = (char) (0x00A6);
		iso_8859_13_chars[0xA7] = (char) (0x00A7);
		iso_8859_13_chars[0xA8] = (char) (0x00D8);
		iso_8859_13_chars[0xA9] = (char) (0x00A9);
		iso_8859_13_chars[0xAA] = (char) (0x0156);
		iso_8859_13_chars[0xAB] = (char) (0x00AB);
		iso_8859_13_chars[0xAC] = (char) (0x00AC);
		iso_8859_13_chars[0xAD] = (char) (0x00AD);
		iso_8859_13_chars[0xAE] = (char) (0x00AE);
		iso_8859_13_chars[0xAF] = (char) (0x00C6);
		iso_8859_13_chars[0xB0] = (char) (0x00B0);
		iso_8859_13_chars[0xB1] = (char) (0x00B1);
		iso_8859_13_chars[0xB2] = (char) (0x00B2);
		iso_8859_13_chars[0xB3] = (char) (0x00B3);
		iso_8859_13_chars[0xB4] = (char) (0x201C);
		iso_8859_13_chars[0xB5] = (char) (0x00B5);
		iso_8859_13_chars[0xB6] = (char) (0x00B6);
		iso_8859_13_chars[0xB7] = (char) (0x00B7);
		iso_8859_13_chars[0xB8] = (char) (0x00F8);
		iso_8859_13_chars[0xB9] = (char) (0x00B9);
		iso_8859_13_chars[0xBA] = (char) (0x0157); //LATIN SMALL LETTER R WITH CEDILLA
		iso_8859_13_chars[0xBB] = (char) (0x00BB);
		iso_8859_13_chars[0xBC] = (char) (0x00BC);
		iso_8859_13_chars[0xBD] = (char) (0x00BD);
		iso_8859_13_chars[0xBE] = (char) (0x00BE);
		iso_8859_13_chars[0xBF] = (char) (0x00E6); //LATIN SMALL LETTER AE
		iso_8859_13_chars[0xC0] = (char) (0x0104); //LATIN CAPITAL LETTER A WITH OGONEK
		iso_8859_13_chars[0xC1] = (char) (0x012E); //LATIN CAPITAL LETTER I WITH OGONEK
		iso_8859_13_chars[0xC2] = (char) (0x0100); //LATIN CAPITAL LETTER A WITH MACRON
		iso_8859_13_chars[0xC3] = (char) (0x0106); //LATIN CAPITAL LETTER C WITH ACUTE
		iso_8859_13_chars[0xC4] = (char) (0x00C4); //LATIN CAPITAL LETTER A WITH DIAERESIS
		iso_8859_13_chars[0xC5] = (char) (0x00C5); //LATIN CAPITAL LETTER A WITH RING ABOVE
		iso_8859_13_chars[0xC6] = (char) (0x0118); //LATIN CAPITAL LETTER E WITH OGONEK
		iso_8859_13_chars[0xC7] = (char) (0x0112); //LATIN CAPITAL LETTER E WITH MACRON
		iso_8859_13_chars[0xC8] = (char) (0x010C); //LATIN CAPITAL LETTER C WITH CARON
		iso_8859_13_chars[0xC9] = (char) (0x00C9); //LATIN CAPITAL LETTER E WITH ACUTE
		iso_8859_13_chars[0xCA] = (char) (0x0179); //LATIN CAPITAL LETTER Z WITH ACUTE
		iso_8859_13_chars[0xCB] = (char) (0x0116); //LATIN CAPITAL LETTER E WITH DOT ABOVE
		iso_8859_13_chars[0xCC] = (char) (0x0122); //LATIN CAPITAL LETTER G WITH CEDILLA
		iso_8859_13_chars[0xCD] = (char) (0x0136); //LATIN CAPITAL LETTER K WITH CEDILLA
		iso_8859_13_chars[0xCE] = (char) (0x012A); //LATIN CAPITAL LETTER I WITH MACRON
		iso_8859_13_chars[0xCF] = (char) (0x013B); //LATIN CAPITAL LETTER L WITH CEDILLA
		iso_8859_13_chars[0xD0] = (char) (0x0160); //LATIN CAPITAL LETTER S WITH CARON
		iso_8859_13_chars[0xD1] = (char) (0x0143); //LATIN CAPITAL LETTER N WITH ACUTE
		iso_8859_13_chars[0xD2] = (char) (0x0145); //LATIN CAPITAL LETTER N WITH CEDILLA
		iso_8859_13_chars[0xD3] = (char) (0x00D3); //LATIN CAPITAL LETTER O WITH ACUTE
		iso_8859_13_chars[0xD4] = (char) (0x014C); //LATIN CAPITAL LETTER O WITH MACRON
		iso_8859_13_chars[0xD5] = (char) (0x00D5); //LATIN CAPITAL LETTER O WITH TILDE
		iso_8859_13_chars[0xD6] = (char) (0x00D6); //LATIN CAPITAL LETTER O WITH DIAERESIS
		iso_8859_13_chars[0xD7] = (char) (0x00D7);
		iso_8859_13_chars[0xD8] = (char) (0x0172); //LATIN CAPITAL LETTER U WITH OGONEK
		iso_8859_13_chars[0xD9] = (char) (0x0141); //LATIN CAPITAL LETTER L WITH STROKE
		iso_8859_13_chars[0xDA] = (char) (0x015A); //LATIN CAPITAL LETTER S WITH ACUTE
		iso_8859_13_chars[0xDB] = (char) (0x016A); //LATIN CAPITAL LETTER U WITH MACRON
		iso_8859_13_chars[0xDC] = (char) (0x00DC); //LATIN CAPITAL LETTER U WITH DIAERESIS
		iso_8859_13_chars[0xDD] = (char) (0x017B); //LATIN CAPITAL LETTER Z WITH DOT ABOVE
		iso_8859_13_chars[0xDE] = (char) (0x017D); //LATIN CAPITAL LETTER Z WITH CARON
		iso_8859_13_chars[0xDF] = (char) (0x00DF); //LATIN SMALL LETTER SHARP S
		iso_8859_13_chars[0xE0] = (char) (0x0105); //LATIN SMALL LETTER A WITH OGONEK
		iso_8859_13_chars[0xE1] = (char) (0x012F); //LATIN SMALL LETTER I WITH OGONEK
		iso_8859_13_chars[0xE2] = (char) (0x0101); //LATIN SMALL LETTER A WITH MACRON
		iso_8859_13_chars[0xE3] = (char) (0x0107); //LATIN SMALL LETTER C WITH ACUTE
		iso_8859_13_chars[0xE4] = (char) (0x00E4); //LATIN SMALL LETTER A WITH DIAERESIS
		iso_8859_13_chars[0xE5] = (char) (0x00E5); //LATIN SMALL LETTER A WITH RING ABOVE
		iso_8859_13_chars[0xE6] = (char) (0x0119); //LATIN SMALL LETTER E WITH OGONEK
		iso_8859_13_chars[0xE7] = (char) (0x0113); //LATIN SMALL LETTER E WITH MACRON
		iso_8859_13_chars[0xE8] = (char) (0x010D); //LATIN SMALL LETTER C WITH CARON
		iso_8859_13_chars[0xE9] = (char) (0x00E9); //LATIN SMALL LETTER E WITH ACUTE
		iso_8859_13_chars[0xEA] = (char) (0x017A); //LATIN SMALL LETTER Z WITH ACUTE
		iso_8859_13_chars[0xEB] = (char) (0x0117); //LATIN SMALL LETTER E WITH DOT ABOVE
		iso_8859_13_chars[0xEC] = (char) (0x0123); //LATIN SMALL LETTER G WITH CEDILLA
		iso_8859_13_chars[0xED] = (char) (0x0137); //LATIN SMALL LETTER K WITH CEDILLA
		iso_8859_13_chars[0xEE] = (char) (0x012B); //LATIN SMALL LETTER I WITH MACRON
		iso_8859_13_chars[0xEF] = (char) (0x013C); //LATIN SMALL LETTER L WITH CEDILLA
		iso_8859_13_chars[0xF0] = (char) (0x0161); //LATIN SMALL LETTER S WITH CARON
		iso_8859_13_chars[0xF1] = (char) (0x0144); //LATIN SMALL LETTER N WITH ACUTE
		iso_8859_13_chars[0xF2] = (char) (0x0146); //LATIN SMALL LETTER N WITH CEDILLA
		iso_8859_13_chars[0xF3] = (char) (0x00F3); //LATIN SMALL LETTER O WITH ACUTE
		iso_8859_13_chars[0xF4] = (char) (0x014D); //LATIN SMALL LETTER O WITH MACRON
		iso_8859_13_chars[0xF5] = (char) (0x00F5); //LATIN SMALL LETTER O WITH TILDE
		iso_8859_13_chars[0xF6] = (char) (0x00F6); //LATIN SMALL LETTER O WITH DIAERESIS
		iso_8859_13_chars[0xF7] = (char) (0x00F7);
		iso_8859_13_chars[0xF8] = (char) (0x0173); //LATIN SMALL LETTER U WITH OGONEK
		iso_8859_13_chars[0xF9] = (char) (0x0142); //LATIN SMALL LETTER L WITH STROKE
		iso_8859_13_chars[0xFA] = (char) (0x015B); //LATIN SMALL LETTER S WITH ACUTE
		iso_8859_13_chars[0xFB] = (char) (0x016B); //LATIN SMALL LETTER U WITH MACRON
		iso_8859_13_chars[0xFC] = (char) (0x00FC); //LATIN SMALL LETTER U WITH DIAERESIS
		iso_8859_13_chars[0xFD] = (char) (0x017C); //LATIN SMALL LETTER Z WITH DOT ABOVE
		iso_8859_13_chars[0xFE] = (char) (0x017E); //LATIN SMALL LETTER Z WITH CARON
		iso_8859_13_chars[0xFF] = (char) (0x2019);
		iso_8859_13_chars_ready = TRUE;
	}
}


void iso_8859_14_chars_init(){
	int i;
	if (iso_8859_14_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			iso_8859_14_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			iso_8859_14_chars[i] = (char) (0xfffd);
		}

		iso_8859_14_chars[0xA0] = (char) (0x00A0); //NO-BREAK SPACE
		iso_8859_14_chars[0xA1] = (char) (0x1E02); //LATIN CAPITAL LETTER B WITH DOT ABOVE
		iso_8859_14_chars[0xA2] = (char) (0x1E03); //LATIN SMALL LETTER B WITH DOT ABOVE
		iso_8859_14_chars[0xA3] = (char) (0x00A3); //POUND SIGN
		iso_8859_14_chars[0xA4] = (char) (0x010A); //LATIN CAPITAL LETTER C WITH DOT ABOVE
		iso_8859_14_chars[0xA5] = (char) (0x010B); //LATIN SMALL LETTER C WITH DOT ABOVE
		iso_8859_14_chars[0xA6] = (char) (0x1E0A); //LATIN CAPITAL LETTER D WITH DOT ABOVE
		iso_8859_14_chars[0xA7] = (char) (0x00A7); //SECTION SIGN
		iso_8859_14_chars[0xA8] = (char) (0x1E80); //LATIN CAPITAL LETTER W WITH GRAVE
		iso_8859_14_chars[0xA9] = (char) (0x00A9); //COPYRIGHT SIGN
		iso_8859_14_chars[0xAA] = (char) (0x1E82); //LATIN CAPITAL LETTER W WITH ACUTE
		iso_8859_14_chars[0xAB] = (char) (0x1E0B); //LATIN SMALL LETTER D WITH DOT ABOVE
		iso_8859_14_chars[0xAC] = (char) (0x1EF2); //LATIN CAPITAL LETTER Y WITH GRAVE
		iso_8859_14_chars[0xAD] = (char) (0x00AD); //SOFT HYPHEN
		iso_8859_14_chars[0xAE] = (char) (0x00AE); //REGISTERED SIGN
		iso_8859_14_chars[0xAF] = (char) (0x0178); //LATIN CAPITAL LETTER Y WITH DIAERESIS
		iso_8859_14_chars[0xB0] = (char) (0x1E1E); //LATIN CAPITAL LETTER F WITH DOT ABOVE
		iso_8859_14_chars[0xB1] = (char) (0x1E1F); //LATIN SMALL LETTER F WITH DOT ABOVE
		iso_8859_14_chars[0xB2] = (char) (0x0120); //LATIN CAPITAL LETTER G WITH DOT ABOVE
		iso_8859_14_chars[0xB3] = (char) (0x0121); //LATIN SMALL LETTER G WITH DOT ABOVE
		iso_8859_14_chars[0xB4] = (char) (0x1E40); //LATIN CAPITAL LETTER M WITH DOT ABOVE
		iso_8859_14_chars[0xB5] = (char) (0x1E41); //LATIN SMALL LETTER M WITH DOT ABOVE
		iso_8859_14_chars[0xB6] = (char) (0x00B6); //PILCROW SIGN
		iso_8859_14_chars[0xB7] = (char) (0x1E56); //LATIN CAPITAL LETTER P WITH DOT ABOVE
		iso_8859_14_chars[0xB8] = (char) (0x1E81); //LATIN SMALL LETTER W WITH GRAVE
		iso_8859_14_chars[0xB9] = (char) (0x1E57); //LATIN SMALL LETTER P WITH DOT ABOVE
		iso_8859_14_chars[0xBA] = (char) (0x1E83); //LATIN SMALL LETTER W WITH ACUTE
		iso_8859_14_chars[0xBB] = (char) (0x1E60); //LATIN CAPITAL LETTER S WITH DOT ABOVE
		iso_8859_14_chars[0xBC] = (char) (0x1EF3); //LATIN SMALL LETTER Y WITH GRAVE
		iso_8859_14_chars[0xBD] = (char) (0x1E84); //LATIN CAPITAL LETTER W WITH DIAERESIS
		iso_8859_14_chars[0xBE] = (char) (0x1E85); //LATIN SMALL LETTER W WITH DIAERESIS
		iso_8859_14_chars[0xBF] = (char) (0x1E61); //LATIN SMALL LETTER S WITH DOT ABOVE
		iso_8859_14_chars[0xC0] = (char) (0x00C0); //LATIN CAPITAL LETTER A WITH GRAVE
		iso_8859_14_chars[0xC1] = (char) (0x00C1); //LATIN CAPITAL LETTER A WITH ACUTE
		iso_8859_14_chars[0xC2] = (char) (0x00C2); //LATIN CAPITAL LETTER A WITH CIRCUMFLEX
		iso_8859_14_chars[0xC3] = (char) (0x00C3); //LATIN CAPITAL LETTER A WITH TILDE
		iso_8859_14_chars[0xC4] = (char) (0x00C4); //LATIN CAPITAL LETTER A WITH DIAERESIS
		iso_8859_14_chars[0xC5] = (char) (0x00C5); //LATIN CAPITAL LETTER A WITH RING ABOVE
		iso_8859_14_chars[0xC6] = (char) (0x00C6); //LATIN CAPITAL LETTER AE
		iso_8859_14_chars[0xC7] = (char) (0x00C7); //LATIN CAPITAL LETTER C WITH CEDILLA
		iso_8859_14_chars[0xC8] = (char) (0x00C8); //LATIN CAPITAL LETTER E WITH GRAVE
		iso_8859_14_chars[0xC9] = (char) (0x00C9); //LATIN CAPITAL LETTER E WITH ACUTE
		iso_8859_14_chars[0xCA] = (char) (0x00CA); //LATIN CAPITAL LETTER E WITH CIRCUMFLEX
		iso_8859_14_chars[0xCB] = (char) (0x00CB); //LATIN CAPITAL LETTER E WITH DIAERESIS
		iso_8859_14_chars[0xCC] = (char) (0x00CC); //LATIN CAPITAL LETTER I WITH GRAVE
		iso_8859_14_chars[0xCD] = (char) (0x00CD); //LATIN CAPITAL LETTER I WITH ACUTE
		iso_8859_14_chars[0xCE] = (char) (0x00CE); //LATIN CAPITAL LETTER I WITH CIRCUMFLEX
		iso_8859_14_chars[0xCF] = (char) (0x00CF); //LATIN CAPITAL LETTER I WITH DIAERESIS
		iso_8859_14_chars[0xD0] = (char) (0x0174); //LATIN CAPITAL LETTER W WITH CIRCUMFLEX
		iso_8859_14_chars[0xD1] = (char) (0x00D1); //LATIN CAPITAL LETTER N WITH TILDE
		iso_8859_14_chars[0xD2] = (char) (0x00D2); //LATIN CAPITAL LETTER O WITH GRAVE
		iso_8859_14_chars[0xD3] = (char) (0x00D3); //LATIN CAPITAL LETTER O WITH ACUTE
		iso_8859_14_chars[0xD4] = (char) (0x00D4); //LATIN CAPITAL LETTER O WITH CIRCUMFLEX
		iso_8859_14_chars[0xD5] = (char) (0x00D5); //LATIN CAPITAL LETTER O WITH TILDE
		iso_8859_14_chars[0xD6] = (char) (0x00D6); //LATIN CAPITAL LETTER O WITH DIAERESIS
		iso_8859_14_chars[0xD7] = (char) (0x1E6A); //LATIN CAPITAL LETTER T WITH DOT ABOVE
		iso_8859_14_chars[0xD8] = (char) (0x00D8); //LATIN CAPITAL LETTER O WITH STROKE
		iso_8859_14_chars[0xD9] = (char) (0x00D9); //LATIN CAPITAL LETTER U WITH GRAVE
		iso_8859_14_chars[0xDA] = (char) (0x00DA); //LATIN CAPITAL LETTER U WITH ACUTE
		iso_8859_14_chars[0xDB] = (char) (0x00DB); //LATIN CAPITAL LETTER U WITH CIRCUMFLEX
		iso_8859_14_chars[0xDC] = (char) (0x00DC); //LATIN CAPITAL LETTER U WITH DIAERESIS
		iso_8859_14_chars[0xDD] = (char) (0x00DD); //LATIN CAPITAL LETTER Y WITH ACUTE
		iso_8859_14_chars[0xDE] = (char) (0x0176); //LATIN CAPITAL LETTER Y WITH CIRCUMFLEX
		iso_8859_14_chars[0xDF] = (char) (0x00DF); //LATIN SMALL LETTER SHARP S
		iso_8859_14_chars[0xE0] = (char) (0x00E0); //LATIN SMALL LETTER A WITH GRAVE
		iso_8859_14_chars[0xE1] = (char) (0x00E1); //LATIN SMALL LETTER A WITH ACUTE
		iso_8859_14_chars[0xE2] = (char) (0x00E2); //LATIN SMALL LETTER A WITH CIRCUMFLEX
		iso_8859_14_chars[0xE3] = (char) (0x00E3); //LATIN SMALL LETTER A WITH TILDE
		iso_8859_14_chars[0xE4] = (char) (0x00E4); //LATIN SMALL LETTER A WITH DIAERESIS
		iso_8859_14_chars[0xE5] = (char) (0x00E5); //LATIN SMALL LETTER A WITH RING ABOVE
		iso_8859_14_chars[0xE6] = (char) (0x00E6); //LATIN SMALL LETTER AE
		iso_8859_14_chars[0xE7] = (char) (0x00E7); //LATIN SMALL LETTER C WITH CEDILLA
		iso_8859_14_chars[0xE8] = (char) (0x00E8); //LATIN SMALL LETTER E WITH GRAVE
		iso_8859_14_chars[0xE9] = (char) (0x00E9); //LATIN SMALL LETTER E WITH ACUTE
		iso_8859_14_chars[0xEA] = (char) (0x00EA); //LATIN SMALL LETTER E WITH CIRCUMFLEX
		iso_8859_14_chars[0xEB] = (char) (0x00EB); //LATIN SMALL LETTER E WITH DIAERESIS
		iso_8859_14_chars[0xEC] = (char) (0x00EC); //LATIN SMALL LETTER I WITH GRAVE
		iso_8859_14_chars[0xED] = (char) (0x00ED); //LATIN SMALL LETTER I WITH ACUTE
		iso_8859_14_chars[0xEE] = (char) (0x00EE); //LATIN SMALL LETTER I WITH CIRCUMFLEX
		iso_8859_14_chars[0xEF] = (char) (0x00EF); //LATIN SMALL LETTER I WITH DIAERESIS
		iso_8859_14_chars[0xF0] = (char) (0x0175); //LATIN SMALL LETTER W WITH CIRCUMFLEX
		iso_8859_14_chars[0xF1] = (char) (0x00F1); //LATIN SMALL LETTER N WITH TILDE
		iso_8859_14_chars[0xF2] = (char) (0x00F2); //LATIN SMALL LETTER O WITH GRAVE
		iso_8859_14_chars[0xF3] = (char) (0x00F3); //LATIN SMALL LETTER O WITH ACUTE
		iso_8859_14_chars[0xF4] = (char) (0x00F4); //LATIN SMALL LETTER O WITH CIRCUMFLEX
		iso_8859_14_chars[0xF5] = (char) (0x00F5); //LATIN SMALL LETTER O WITH TILDE
		iso_8859_14_chars[0xF6] = (char) (0x00F6); //LATIN SMALL LETTER O WITH DIAERESIS
		iso_8859_14_chars[0xF7] = (char) (0x1E6B); //LATIN SMALL LETTER T WITH DOT ABOVE
		iso_8859_14_chars[0xF8] = (char) (0x00F8); //LATIN SMALL LETTER O WITH STROKE
		iso_8859_14_chars[0xF9] = (char) (0x00F9); //LATIN SMALL LETTER U WITH GRAVE
		iso_8859_14_chars[0xFA] = (char) (0x00FA); //LATIN SMALL LETTER U WITH ACUTE
		iso_8859_14_chars[0xFB] = (char) (0x00FB); //LATIN SMALL LETTER U WITH CIRCUMFLEX
		iso_8859_14_chars[0xFC] = (char) (0x00FC); //LATIN SMALL LETTER U WITH DIAERESIS
		iso_8859_14_chars[0xFD] = (char) (0x00FD); //LATIN SMALL LETTER Y WITH ACUTE
		iso_8859_14_chars[0xFE] = (char) (0x0177); //LATIN SMALL LETTER Y WITH CIRCUMFLEX
		iso_8859_14_chars[0xFF] = (char) (0x00FF); //LATIN SMALL LETTER Y WITH DIAERESIS

		iso_8859_14_chars_ready = TRUE;
	}
}


void iso_8859_15_chars_init(){
	int i;
	if (iso_8859_15_chars_ready)
		return;
	else{
		for (i = 0; i < 256; i++)
		{
			iso_8859_15_chars[i] = (char) i;
		}

		iso_8859_15_chars[0xA4] = (char) (0x20AC);
		iso_8859_15_chars[0xA6] = (char) (0x0160);
		iso_8859_15_chars[0xA8] = (char) (0x0161);
		iso_8859_15_chars[0xB4] = (char) (0x017D);
		iso_8859_15_chars[0xB8] = (char) (0x017E);
		iso_8859_15_chars[0xBC] = (char) (0x0152);
		iso_8859_15_chars[0xBD] = (char) (0x0153);
		iso_8859_15_chars[0xBE] = (char) (0x0178);
		iso_8859_15_chars_ready = TRUE;
	}
}


void windows_1250_chars_init(){
	int i;
	if (windows_1250_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			windows_1250_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			windows_1250_chars[i] = (char) (0xfffd);
		}
					
		windows_1250_chars[0x80] = (char) (0x20AC); /* EURO SIGN*/
		windows_1250_chars[0x82] = (char) (0x201A); /* SINGLE LOW-9 QUOTATION MARK*/
		windows_1250_chars[0x84] = (char) (0x201E); /* DOUBLE LOW-9 QUOTATION MARK*/
		windows_1250_chars[0x85] = (char) (0x2026); /* HORIZONTAL ELLIPSIS*/
		windows_1250_chars[0x86] = (char) (0x2020); /* DAGGER*/
		windows_1250_chars[0x87] = (char) (0x2021); /* DOUBLE DAGGER*/
		windows_1250_chars[0x89] = (char) (0x2030); /* PER MILLE SIGN*/
		windows_1250_chars[0x8A] = (char) (0x0160); /* LATIN CAPITAL LETTER S WITH CARON*/
		windows_1250_chars[0x8B] = (char) (0x2039); /* SINGLE LEFT-POINTING ANGLE QUOTATION MARK*/
		windows_1250_chars[0x8C] = (char) (0x015A); /* LATIN CAPITAL LETTER S WITH ACUTE*/
		windows_1250_chars[0x8D] = (char) (0x0164); /* LATIN CAPITAL LETTER T WITH CARON*/
		windows_1250_chars[0x8E] = (char) (0x017D); /* LATIN CAPITAL LETTER Z WITH CARON*/
		windows_1250_chars[0x8F] = (char) (0x0179); /* LATIN CAPITAL LETTER Z WITH ACUTE*/
		windows_1250_chars[0x91] = (char) (0x2018); /* LEFT SINGLE QUOTATION MARK*/
		windows_1250_chars[0x92] = (char) (0x2019); /* RIGHT SINGLE QUOTATION MARK*/
		windows_1250_chars[0x93] = (char) (0x201C); /* LEFT DOUBLE QUOTATION MARK*/
		windows_1250_chars[0x94] = (char) (0x201D); /* RIGHT DOUBLE QUOTATION MARK*/
		windows_1250_chars[0x95] = (char) (0x2022); /* BULLET*/
		windows_1250_chars[0x96] = (char) (0x2013); /* EN DASH*/
		windows_1250_chars[0x97] = (char) (0x2014); /* EM DASH*/
		windows_1250_chars[0x99] = (char) (0x2122); /* TRADE MARK SIGN*/
		windows_1250_chars[0x9A] = (char) (0x0161); /* LATIN SMALL LETTER S WITH CARON*/
		windows_1250_chars[0x9B] = (char) (0x203A); /* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK*/
		windows_1250_chars[0x9C] = (char) (0x015B); /* LATIN SMALL LETTER S WITH ACUTE*/
		windows_1250_chars[0x9D] = (char) (0x0165); /* LATIN SMALL LETTER T WITH CARON*/
		windows_1250_chars[0x9E] = (char) (0x017E); /* LATIN SMALL LETTER Z WITH CARON*/
		windows_1250_chars[0x9F] = (char) (0x017A); /* LATIN SMALL LETTER Z WITH ACUTE*/
		windows_1250_chars[0xA0] = (char) (0x00A0); /* NO-BREAK SPACE*/
		windows_1250_chars[0xA1] = (char) (0x02C7); /* CARON*/
		windows_1250_chars[0xA2] = (char) (0x02D8); /* BREVE*/
		windows_1250_chars[0xA3] = (char) (0x0141); /* LATIN CAPITAL LETTER L WITH STROKE*/
		windows_1250_chars[0xA4] = (char) (0x00A4); /* CURRENCY SIGN*/
		windows_1250_chars[0xA5] = (char) (0x0104); /* LATIN CAPITAL LETTER A WITH OGONEK*/
		windows_1250_chars[0xA6] = (char) (0x00A6); /* BROKEN BAR*/
		windows_1250_chars[0xA7] = (char) (0x00A7); /* SECTION SIGN*/
		windows_1250_chars[0xA8] = (char) (0x00A8); /* DIAERESIS*/
		windows_1250_chars[0xA9] = (char) (0x00A9); /* COPYRIGHT SIGN*/
		windows_1250_chars[0xAA] = (char) (0x015E); /* LATIN CAPITAL LETTER S WITH CEDILLA*/
		windows_1250_chars[0xAB] = (char) (0x00AB); /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1250_chars[0xAC] = (char) (0x00AC); /* NOT SIGN*/
		windows_1250_chars[0xAD] = (char) (0x00AD); /* SOFT HYPHEN*/
		windows_1250_chars[0xAE] = (char) (0x00AE); /* REGISTERED SIGN*/
		windows_1250_chars[0xAF] = (char) (0x017B); /* LATIN CAPITAL LETTER Z WITH DOT ABOVE*/
		windows_1250_chars[0xB0] = (char) (0x00B0); /* DEGREE SIGN*/
		windows_1250_chars[0xB1] = (char) (0x00B1); /* PLUS-MINUS SIGN*/
		windows_1250_chars[0xB2] = (char) (0x02DB); /* OGONEK*/
		windows_1250_chars[0xB3] = (char) (0x0142); /* LATIN SMALL LETTER L WITH STROKE*/
		windows_1250_chars[0xB4] = (char) (0x00B4); /* ACUTE ACCENT*/
		windows_1250_chars[0xB5] = (char) (0x00B5); /* MICRO SIGN*/
		windows_1250_chars[0xB6] = (char) (0x00B6); /* PILCROW SIGN*/
		windows_1250_chars[0xB7] = (char) (0x00B7); /* MIDDLE DOT*/
		windows_1250_chars[0xB8] = (char) (0x00B8); /* CEDILLA*/
		windows_1250_chars[0xB9] = (char) (0x0105); /* LATIN SMALL LETTER A WITH OGONEK*/
		windows_1250_chars[0xBA] = (char) (0x015F); /* LATIN SMALL LETTER S WITH CEDILLA*/
		windows_1250_chars[0xBB] = (char) (0x00BB); /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1250_chars[0xBC] = (char) (0x013D); /* LATIN CAPITAL LETTER L WITH CARON*/
		windows_1250_chars[0xBD] = (char) (0x02DD); /* DOUBLE ACUTE ACCENT*/
		windows_1250_chars[0xBE] = (char) (0x013E); /* LATIN SMALL LETTER L WITH CARON*/
		windows_1250_chars[0xBF] = (char) (0x017C); /* LATIN SMALL LETTER Z WITH DOT ABOVE*/
		windows_1250_chars[0xC0] = (char) (0x0154); /* LATIN CAPITAL LETTER R WITH ACUTE*/
		windows_1250_chars[0xC1] = (char) (0x00C1); /* LATIN CAPITAL LETTER A WITH ACUTE*/
		windows_1250_chars[0xC2] = (char) (0x00C2); /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX*/
		windows_1250_chars[0xC3] = (char) (0x0102); /* LATIN CAPITAL LETTER A WITH BREVE*/
		windows_1250_chars[0xC4] = (char) (0x00C4); /* LATIN CAPITAL LETTER A WITH DIAERESIS*/
		windows_1250_chars[0xC5] = (char) (0x0139); /* LATIN CAPITAL LETTER L WITH ACUTE*/
		windows_1250_chars[0xC6] = (char) (0x0106); /* LATIN CAPITAL LETTER C WITH ACUTE*/
		windows_1250_chars[0xC7] = (char) (0x00C7); /* LATIN CAPITAL LETTER C WITH CEDILLA*/
		windows_1250_chars[0xC8] = (char) (0x010C); /* LATIN CAPITAL LETTER C WITH CARON*/
		windows_1250_chars[0xC9] = (char) (0x00C9); /* LATIN CAPITAL LETTER E WITH ACUTE*/
		windows_1250_chars[0xCA] = (char) (0x0118); /* LATIN CAPITAL LETTER E WITH OGONEK*/
		windows_1250_chars[0xCB] = (char) (0x00CB); /* LATIN CAPITAL LETTER E WITH DIAERESIS*/
		windows_1250_chars[0xCC] = (char) (0x011A); /* LATIN CAPITAL LETTER E WITH CARON*/
		windows_1250_chars[0xCD] = (char) (0x00CD); /* LATIN CAPITAL LETTER I WITH ACUTE*/
		windows_1250_chars[0xCE] = (char) (0x00CE); /* LATIN CAPITAL LETTER I WITH CIRCUMFLEX*/
		windows_1250_chars[0xCF] = (char) (0x010E); /* LATIN CAPITAL LETTER D WITH CARON*/
		windows_1250_chars[0xD0] = (char) (0x0110); /* LATIN CAPITAL LETTER D WITH STROKE*/
		windows_1250_chars[0xD1] = (char) (0x0143); /* LATIN CAPITAL LETTER N WITH ACUTE*/
		windows_1250_chars[0xD2] = (char) (0x0147); /* LATIN CAPITAL LETTER N WITH CARON*/
		windows_1250_chars[0xD3] = (char) (0x00D3); /* LATIN CAPITAL LETTER O WITH ACUTE*/
		windows_1250_chars[0xD4] = (char) (0x00D4); /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX*/
		windows_1250_chars[0xD5] = (char) (0x0150); /* LATIN CAPITAL LETTER O WITH DOUBLE ACUTE*/
		windows_1250_chars[0xD6] = (char) (0x00D6); /* LATIN CAPITAL LETTER O WITH DIAERESIS*/
		windows_1250_chars[0xD7] = (char) (0x00D7); /* MULTIPLICATION SIGN*/
		windows_1250_chars[0xD8] = (char) (0x0158); /* LATIN CAPITAL LETTER R WITH CARON*/
		windows_1250_chars[0xD9] = (char) (0x016E); /* LATIN CAPITAL LETTER U WITH RING ABOVE*/
		windows_1250_chars[0xDA] = (char) (0x00DA); /* LATIN CAPITAL LETTER U WITH ACUTE*/
		windows_1250_chars[0xDB] = (char) (0x0170); /* LATIN CAPITAL LETTER U WITH DOUBLE ACUTE*/
		windows_1250_chars[0xDC] = (char) (0x00DC); /* LATIN CAPITAL LETTER U WITH DIAERESIS*/
		windows_1250_chars[0xDD] = (char) (0x00DD); /* LATIN CAPITAL LETTER Y WITH ACUTE*/
		windows_1250_chars[0xDE] = (char) (0x0162); /* LATIN CAPITAL LETTER T WITH CEDILLA*/
		windows_1250_chars[0xDF] = (char) (0x00DF); /* LATIN SMALL LETTER SHARP S*/
		windows_1250_chars[0xE0] = (char) (0x0155); /* LATIN SMALL LETTER R WITH ACUTE*/
		windows_1250_chars[0xE1] = (char) (0x00E1); /* LATIN SMALL LETTER A WITH ACUTE*/
		windows_1250_chars[0xE2] = (char) (0x00E2); /* LATIN SMALL LETTER A WITH CIRCUMFLEX*/
		windows_1250_chars[0xE3] = (char) (0x0103); /* LATIN SMALL LETTER A WITH BREVE*/
		windows_1250_chars[0xE4] = (char) (0x00E4); /* LATIN SMALL LETTER A WITH DIAERESIS*/
		windows_1250_chars[0xE5] = (char) (0x013A); /* LATIN SMALL LETTER L WITH ACUTE*/
		windows_1250_chars[0xE6] = (char) (0x0107); /* LATIN SMALL LETTER C WITH ACUTE*/
		windows_1250_chars[0xE7] = (char) (0x00E7); /* LATIN SMALL LETTER C WITH CEDILLA*/
		windows_1250_chars[0xE8] = (char) (0x010D); /* LATIN SMALL LETTER C WITH CARON*/
		windows_1250_chars[0xE9] = (char) (0x00E9); /* LATIN SMALL LETTER E WITH ACUTE*/
		windows_1250_chars[0xEA] = (char) (0x0119); /* LATIN SMALL LETTER E WITH OGONEK*/
		windows_1250_chars[0xEB] = (char) (0x00EB); /* LATIN SMALL LETTER E WITH DIAERESIS*/
		windows_1250_chars[0xEC] = (char) (0x011B); /* LATIN SMALL LETTER E WITH CARON*/
		windows_1250_chars[0xED] = (char) (0x00ED); /* LATIN SMALL LETTER I WITH ACUTE*/
		windows_1250_chars[0xEE] = (char) (0x00EE); /* LATIN SMALL LETTER I WITH CIRCUMFLEX*/
		windows_1250_chars[0xEF] = (char) (0x010F); /* LATIN SMALL LETTER D WITH CARON*/
		windows_1250_chars[0xF0] = (char) (0x0111); /* LATIN SMALL LETTER D WITH STROKE*/
		windows_1250_chars[0xF1] = (char) (0x0144); /* LATIN SMALL LETTER N WITH ACUTE*/
		windows_1250_chars[0xF2] = (char) (0x0148); /* LATIN SMALL LETTER N WITH CARON*/
		windows_1250_chars[0xF3] = (char) (0x00F3); /* LATIN SMALL LETTER O WITH ACUTE*/
		windows_1250_chars[0xF4] = (char) (0x00F4); /* LATIN SMALL LETTER O WITH CIRCUMFLEX*/
		windows_1250_chars[0xF5] = (char) (0x0151); /* LATIN SMALL LETTER O WITH DOUBLE ACUTE*/
		windows_1250_chars[0xF6] = (char) (0x00F6); /* LATIN SMALL LETTER O WITH DIAERESIS*/
		windows_1250_chars[0xF7] = (char) (0x00F7); /* DIVISION SIGN*/
		windows_1250_chars[0xF8] = (char) (0x0159); /* LATIN SMALL LETTER R WITH CARON*/
		windows_1250_chars[0xF9] = (char) (0x016F); /* LATIN SMALL LETTER U WITH RING ABOVE*/
		windows_1250_chars[0xFA] = (char) (0x00FA); /* LATIN SMALL LETTER U WITH ACUTE*/
		windows_1250_chars[0xFB] = (char) (0x0171); /* LATIN SMALL LETTER U WITH DOUBLE ACUTE*/
		windows_1250_chars[0xFC] = (char) (0x00FC); /* LATIN SMALL LETTER U WITH DIAERESIS*/
		windows_1250_chars[0xFD] = (char) (0x00FD); /* LATIN SMALL LETTER Y WITH ACUTE*/
		windows_1250_chars[0xFE] = (char) (0x0163); /* LATIN SMALL LETTER T WITH CEDILLA*/
		windows_1250_chars[0xFF] = (char) (0x02D9); /* DOT ABOVE*/

		windows_1250_chars_ready = TRUE;
	}
}
void windows_1251_chars_init(){
	int i;
	if (windows_1250_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			windows_1250_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			windows_1250_chars[i] = (char) (0xfffd);
		}
		windows_1251_chars[0x80] = (char) (0x0402); /* CYRILLIC CAPITAL LETTER DJE*/
		windows_1251_chars[0x81] = (char) (0x0403); /* CYRILLIC CAPITAL LETTER GJE*/
		windows_1251_chars[0x82] = (char) (0x201A); /* SINGLE LOW-9 QUOTATION MARK*/
		windows_1251_chars[0x83] = (char) (0x0453); /* CYRILLIC SMALL LETTER GJE*/
		windows_1251_chars[0x84] = (char) (0x201E); /* DOUBLE LOW-9 QUOTATION MARK*/
		windows_1251_chars[0x85] = (char) (0x2026); /* HORIZONTAL ELLIPSIS*/
		windows_1251_chars[0x86] = (char) (0x2020); /* DAGGER*/
		windows_1251_chars[0x87] = (char) (0x2021); /* DOUBLE DAGGER*/
		windows_1251_chars[0x88] = (char) (0x20AC); /* EURO SIGN*/
		windows_1251_chars[0x89] = (char) (0x2030); /* PER MILLE SIGN*/
		windows_1251_chars[0x8A] = (char) (0x0409); /* CYRILLIC CAPITAL LETTER LJE*/
		windows_1251_chars[0x8B] = (char) (0x2039); /* SINGLE LEFT-POINTING ANGLE QUOTATION MARK*/
		windows_1251_chars[0x8C] = (char) (0x040A); /* CYRILLIC CAPITAL LETTER NJE*/
		windows_1251_chars[0x8D] = (char) (0x040C); /* CYRILLIC CAPITAL LETTER KJE*/
		windows_1251_chars[0x8E] = (char) (0x040B); /* CYRILLIC CAPITAL LETTER TSHE*/
		windows_1251_chars[0x8F] = (char) (0x040F); /* CYRILLIC CAPITAL LETTER DZHE*/
		windows_1251_chars[0x90] = (char) (0x0452); /* CYRILLIC SMALL LETTER DJE*/
		windows_1251_chars[0x91] = (char) (0x2018); /* LEFT SINGLE QUOTATION MARK*/
		windows_1251_chars[0x92] = (char) (0x2019); /* RIGHT SINGLE QUOTATION MARK*/
		windows_1251_chars[0x93] = (char) (0x201C); /* LEFT DOUBLE QUOTATION MARK*/
		windows_1251_chars[0x94] = (char) (0x201D); /* RIGHT DOUBLE QUOTATION MARK*/
		windows_1251_chars[0x95] = (char) (0x2022); /* BULLET*/
		windows_1251_chars[0x96] = (char) (0x2013); /* EN DASH*/
		windows_1251_chars[0x97] = (char) (0x2014); /* EM DASH*/
		windows_1251_chars[0x99] = (char) (0x2122); /* TRADE MARK SIGN*/
		windows_1251_chars[0x9A] = (char) (0x0459); /* CYRILLIC SMALL LETTER LJE*/
		windows_1251_chars[0x9B] = (char) (0x203A); /* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK*/
		windows_1251_chars[0x9C] = (char) (0x045A); /* CYRILLIC SMALL LETTER NJE*/
		windows_1251_chars[0x9D] = (char) (0x045C); /* CYRILLIC SMALL LETTER KJE*/
		windows_1251_chars[0x9E] = (char) (0x045B); /* CYRILLIC SMALL LETTER TSHE*/
		windows_1251_chars[0x9F] = (char) (0x045F); /* CYRILLIC SMALL LETTER DZHE*/
		windows_1251_chars[0xA0] = (char) (0x00A0); /* NO-BREAK SPACE*/
		windows_1251_chars[0xA1] = (char) (0x040E); /* CYRILLIC CAPITAL LETTER SHORT U*/
		windows_1251_chars[0xA2] = (char) (0x045E); /* CYRILLIC SMALL LETTER SHORT U*/
		windows_1251_chars[0xA3] = (char) (0x0408); /* CYRILLIC CAPITAL LETTER JE*/
		windows_1251_chars[0xA4] = (char) (0x00A4); /* CURRENCY SIGN*/
		windows_1251_chars[0xA5] = (char) (0x0490); /* CYRILLIC CAPITAL LETTER GHE WITH UPTURN*/
		windows_1251_chars[0xA6] = (char) (0x00A6); /* BROKEN BAR*/
		windows_1251_chars[0xA7] = (char) (0x00A7); /* SECTION SIGN*/
		windows_1251_chars[0xA8] = (char) (0x0401); /* CYRILLIC CAPITAL LETTER IO*/
		windows_1251_chars[0xA9] = (char) (0x00A9); /* COPYRIGHT SIGN*/
		windows_1251_chars[0xAA] = (char) (0x0404); /* CYRILLIC CAPITAL LETTER UKRAINIAN IE*/
		windows_1251_chars[0xAB] = (char) (0x00AB); /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1251_chars[0xAC] = (char) (0x00AC); /* NOT SIGN*/
		windows_1251_chars[0xAD] = (char) (0x00AD); /* SOFT HYPHEN*/
		windows_1251_chars[0xAE] = (char) (0x00AE); /* REGISTERED SIGN*/
		windows_1251_chars[0xAF] = (char) (0x0407); /* CYRILLIC CAPITAL LETTER YI*/
		windows_1251_chars[0xB0] = (char) (0x00B0); /* DEGREE SIGN*/
		windows_1251_chars[0xB1] = (char) (0x00B1); /* PLUS-MINUS SIGN*/
		windows_1251_chars[0xB2] = (char) (0x0406); /* CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I*/
		windows_1251_chars[0xB3] = (char) (0x0456); /* CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I*/
		windows_1251_chars[0xB4] = (char) (0x0491); /* CYRILLIC SMALL LETTER GHE WITH UPTURN*/
		windows_1251_chars[0xB5] = (char) (0x00B5); /* MICRO SIGN*/
		windows_1251_chars[0xB6] = (char) (0x00B6); /* PILCROW SIGN*/
		windows_1251_chars[0xB7] = (char) (0x00B7); /* MIDDLE DOT*/
		windows_1251_chars[0xB8] = (char) (0x0451); /* CYRILLIC SMALL LETTER IO*/
		windows_1251_chars[0xB9] = (char) (0x2116); /* NUMERO SIGN*/
		windows_1251_chars[0xBA] = (char) (0x0454); /* CYRILLIC SMALL LETTER UKRAINIAN IE*/
		windows_1251_chars[0xBB] = (char) (0x00BB); /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1251_chars[0xBC] = (char) (0x0458); /* CYRILLIC SMALL LETTER JE*/
		windows_1251_chars[0xBD] = (char) (0x0405); /* CYRILLIC CAPITAL LETTER DZE*/
		windows_1251_chars[0xBE] = (char) (0x0455); /* CYRILLIC SMALL LETTER DZE*/
		windows_1251_chars[0xBF] = (char) (0x0457); /* CYRILLIC SMALL LETTER YI*/
		windows_1251_chars[0xC0] = (char) (0x0410); /* CYRILLIC CAPITAL LETTER A*/
		windows_1251_chars[0xC1] = (char) (0x0411); /* CYRILLIC CAPITAL LETTER BE*/
		windows_1251_chars[0xC2] = (char) (0x0412); /* CYRILLIC CAPITAL LETTER VE*/
		windows_1251_chars[0xC3] = (char) (0x0413); /* CYRILLIC CAPITAL LETTER GHE*/
		windows_1251_chars[0xC4] = (char) (0x0414); /* CYRILLIC CAPITAL LETTER DE*/
		windows_1251_chars[0xC5] = (char) (0x0415); /* CYRILLIC CAPITAL LETTER IE*/
		windows_1251_chars[0xC6] = (char) (0x0416); /* CYRILLIC CAPITAL LETTER ZHE*/
		windows_1251_chars[0xC7] = (char) (0x0417); /* CYRILLIC CAPITAL LETTER ZE*/
		windows_1251_chars[0xC8] = (char) (0x0418); /* CYRILLIC CAPITAL LETTER I*/
		windows_1251_chars[0xC9] = (char) (0x0419); /* CYRILLIC CAPITAL LETTER SHORT I*/
		windows_1251_chars[0xCA] = (char) (0x041A); /* CYRILLIC CAPITAL LETTER KA*/
		windows_1251_chars[0xCB] = (char) (0x041B); /* CYRILLIC CAPITAL LETTER EL*/
		windows_1251_chars[0xCC] = (char) (0x041C); /* CYRILLIC CAPITAL LETTER EM*/
		windows_1251_chars[0xCD] = (char) (0x041D); /* CYRILLIC CAPITAL LETTER EN*/
		windows_1251_chars[0xCE] = (char) (0x041E); /* CYRILLIC CAPITAL LETTER O*/
		windows_1251_chars[0xCF] = (char) (0x041F); /* CYRILLIC CAPITAL LETTER PE*/
		windows_1251_chars[0xD0] = (char) (0x0420); /* CYRILLIC CAPITAL LETTER ER*/
		windows_1251_chars[0xD1] = (char) (0x0421); /* CYRILLIC CAPITAL LETTER ES*/
		windows_1251_chars[0xD2] = (char) (0x0422); /* CYRILLIC CAPITAL LETTER TE*/
		windows_1251_chars[0xD3] = (char) (0x0423); /* CYRILLIC CAPITAL LETTER U*/
		windows_1251_chars[0xD4] = (char) (0x0424); /* CYRILLIC CAPITAL LETTER EF*/
		windows_1251_chars[0xD5] = (char) (0x0425); /* CYRILLIC CAPITAL LETTER HA*/
		windows_1251_chars[0xD6] = (char) (0x0426); /* CYRILLIC CAPITAL LETTER TSE*/
		windows_1251_chars[0xD7] = (char) (0x0427); /* CYRILLIC CAPITAL LETTER CHE*/
		windows_1251_chars[0xD8] = (char) (0x0428); /* CYRILLIC CAPITAL LETTER SHA*/
		windows_1251_chars[0xD9] = (char) (0x0429); /* CYRILLIC CAPITAL LETTER SHCHA*/
		windows_1251_chars[0xDA] = (char) (0x042A); /* CYRILLIC CAPITAL LETTER HARD SIGN*/
		windows_1251_chars[0xDB] = (char) (0x042B); /* CYRILLIC CAPITAL LETTER YERU*/
		windows_1251_chars[0xDC] = (char) (0x042C); /* CYRILLIC CAPITAL LETTER SOFT SIGN*/
		windows_1251_chars[0xDD] = (char) (0x042D); /* CYRILLIC CAPITAL LETTER E*/
		windows_1251_chars[0xDE] = (char) (0x042E); /* CYRILLIC CAPITAL LETTER YU*/
		windows_1251_chars[0xDF] = (char) (0x042F); /* CYRILLIC CAPITAL LETTER YA*/
		windows_1251_chars[0xE0] = (char) (0x0430); /* CYRILLIC SMALL LETTER A*/
		windows_1251_chars[0xE1] = (char) (0x0431); /* CYRILLIC SMALL LETTER BE*/
		windows_1251_chars[0xE2] = (char) (0x0432); /* CYRILLIC SMALL LETTER VE*/
		windows_1251_chars[0xE3] = (char) (0x0433); /* CYRILLIC SMALL LETTER GHE*/
		windows_1251_chars[0xE4] = (char) (0x0434); /* CYRILLIC SMALL LETTER DE*/
		windows_1251_chars[0xE5] = (char) (0x0435); /* CYRILLIC SMALL LETTER IE*/
		windows_1251_chars[0xE6] = (char) (0x0436); /* CYRILLIC SMALL LETTER ZHE*/
		windows_1251_chars[0xE7] = (char) (0x0437); /* CYRILLIC SMALL LETTER ZE*/
		windows_1251_chars[0xE8] = (char) (0x0438); /* CYRILLIC SMALL LETTER I*/
		windows_1251_chars[0xE9] = (char) (0x0439); /* CYRILLIC SMALL LETTER SHORT I*/
		windows_1251_chars[0xEA] = (char) (0x043A); /* CYRILLIC SMALL LETTER KA*/
		windows_1251_chars[0xEB] = (char) (0x043B); /* CYRILLIC SMALL LETTER EL*/
		windows_1251_chars[0xEC] = (char) (0x043C); /* CYRILLIC SMALL LETTER EM*/
		windows_1251_chars[0xED] = (char) (0x043D); /* CYRILLIC SMALL LETTER EN*/
		windows_1251_chars[0xEE] = (char) (0x043E); /* CYRILLIC SMALL LETTER O*/
		windows_1251_chars[0xEF] = (char) (0x043F); /* CYRILLIC SMALL LETTER PE*/
		windows_1251_chars[0xF0] = (char) (0x0440); /* CYRILLIC SMALL LETTER ER*/
		windows_1251_chars[0xF1] = (char) (0x0441); /* CYRILLIC SMALL LETTER ES*/
		windows_1251_chars[0xF2] = (char) (0x0442); /* CYRILLIC SMALL LETTER TE*/
		windows_1251_chars[0xF3] = (char) (0x0443); /* CYRILLIC SMALL LETTER U*/
		windows_1251_chars[0xF4] = (char) (0x0444); /* CYRILLIC SMALL LETTER EF*/
		windows_1251_chars[0xF5] = (char) (0x0445); /* CYRILLIC SMALL LETTER HA*/
		windows_1251_chars[0xF6] = (char) (0x0446); /* CYRILLIC SMALL LETTER TSE*/
		windows_1251_chars[0xF7] = (char) (0x0447); /* CYRILLIC SMALL LETTER CHE*/
		windows_1251_chars[0xF8] = (char) (0x0448); /* CYRILLIC SMALL LETTER SHA*/
		windows_1251_chars[0xF9] = (char) (0x0449); /* CYRILLIC SMALL LETTER SHCHA*/
		windows_1251_chars[0xFA] = (char) (0x044A); /* CYRILLIC SMALL LETTER HARD SIGN*/
		windows_1251_chars[0xFB] = (char) (0x044B); /* CYRILLIC SMALL LETTER YERU*/
		windows_1251_chars[0xFC] = (char) (0x044C); /* CYRILLIC SMALL LETTER SOFT SIGN*/
		windows_1251_chars[0xFD] = (char) (0x044D); /* CYRILLIC SMALL LETTER E*/
		windows_1251_chars[0xFE] = (char) (0x044E); /* CYRILLIC SMALL LETTER YU*/
		windows_1251_chars[0xFF] = (char) (0x044F); /* CYRILLIC SMALL LETTER YA*/

		windows_1250_chars_ready = TRUE;
	}
}
void windows_1252_chars_init(){
	int i;
	if (windows_1252_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			windows_1252_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			windows_1252_chars[i] = (char) (0xfffd);
		}
		windows_1252_chars[0x80] = (char) (0x20AC); /* EURO SIGN*/
		windows_1252_chars[0x82] = (char) (0x201A); /* SINGLE LOW-9 QUOTATION MARK*/
		windows_1252_chars[0x83] = (char) (0x0192); /* LATIN SMALL LETTER F WITH HOOK*/
		windows_1252_chars[0x84] = (char) (0x201E); /* DOUBLE LOW-9 QUOTATION MARK*/
		windows_1252_chars[0x85] = (char) (0x2026); /* HORIZONTAL ELLIPSIS*/
		windows_1252_chars[0x86] = (char) (0x2020); /* DAGGER*/
		windows_1252_chars[0x87] = (char) (0x2021); /* DOUBLE DAGGER*/
		windows_1252_chars[0x88] = (char) (0x02C6); /* MODIFIER LETTER CIRCUMFLEX ACCENT*/
		windows_1252_chars[0x89] = (char) (0x2030); /* PER MILLE SIGN*/
		windows_1252_chars[0x8A] = (char) (0x0160); /* LATIN CAPITAL LETTER S WITH CARON*/
		windows_1252_chars[0x8B] = (char) (0x2039); /* SINGLE LEFT-POINTING ANGLE QUOTATION MARK*/
		windows_1252_chars[0x8C] = (char) (0x0152); /* LATIN CAPITAL LIGATURE OE*/
		windows_1252_chars[0x8E] = (char) (0x017D); /* LATIN CAPITAL LETTER Z WITH CARON*/
		windows_1252_chars[0x91] = (char) (0x2018); /* LEFT SINGLE QUOTATION MARK*/
		windows_1252_chars[0x92] = (char) (0x2019); /* RIGHT SINGLE QUOTATION MARK*/
		windows_1252_chars[0x93] = (char) (0x201C); /* LEFT DOUBLE QUOTATION MARK*/
		windows_1252_chars[0x94] = (char) (0x201D); /* RIGHT DOUBLE QUOTATION MARK*/
		windows_1252_chars[0x95] = (char) (0x2022); /* BULLET*/
		windows_1252_chars[0x96] = (char) (0x2013); /* EN DASH*/
		windows_1252_chars[0x97] = (char) (0x2014); /* EM DASH*/
		windows_1252_chars[0x98] = (char) (0x02DC); /* SMALL TILDE*/
		windows_1252_chars[0x99] = (char) (0x2122); /* TRADE MARK SIGN*/
		windows_1252_chars[0x9A] = (char) (0x0161); /* LATIN SMALL LETTER S WITH CARON*/
		windows_1252_chars[0x9B] = (char) (0x203A); /* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK*/
		windows_1252_chars[0x9C] = (char) (0x0153); /* LATIN SMALL LIGATURE OE*/
		windows_1252_chars[0x9E] = (char) (0x017E); /* LATIN SMALL LETTER Z WITH CARON*/
		windows_1252_chars[0x9F] = (char) (0x0178); /* LATIN CAPITAL LETTER Y WITH DIAERESIS*/
		windows_1252_chars[0xA0] = (char) (0x00A0); /* NO-BREAK SPACE*/
		windows_1252_chars[0xA1] = (char) (0x00A1); /* INVERTED EXCLAMATION MARK*/
		windows_1252_chars[0xA2] = (char) (0x00A2); /* CENT SIGN*/
		windows_1252_chars[0xA3] = (char) (0x00A3); /* POUND SIGN*/
		windows_1252_chars[0xA4] = (char) (0x00A4); /* CURRENCY SIGN*/
		windows_1252_chars[0xA5] = (char) (0x00A5); /* YEN SIGN*/
		windows_1252_chars[0xA6] = (char) (0x00A6); /* BROKEN BAR*/
		windows_1252_chars[0xA7] = (char) (0x00A7); /* SECTION SIGN*/
		windows_1252_chars[0xA8] = (char) (0x00A8); /* DIAERESIS*/
		windows_1252_chars[0xA9] = (char) (0x00A9); /* COPYRIGHT SIGN*/
		windows_1252_chars[0xAA] = (char) (0x00AA); /* FEMININE ORDINAL INDICATOR*/
		windows_1252_chars[0xAB] = (char) (0x00AB); /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1252_chars[0xAC] = (char) (0x00AC); /* NOT SIGN*/
		windows_1252_chars[0xAD] = (char) (0x00AD); /* SOFT HYPHEN*/
		windows_1252_chars[0xAE] = (char) (0x00AE); /* REGISTERED SIGN*/
		windows_1252_chars[0xAF] = (char) (0x00AF); /* MACRON*/
		windows_1252_chars[0xB0] = (char) (0x00B0); /* DEGREE SIGN*/
		windows_1252_chars[0xB1] = (char) (0x00B1); /* PLUS-MINUS SIGN*/
		windows_1252_chars[0xB2] = (char) (0x00B2); /* SUPERSCRIPT TWO*/
		windows_1252_chars[0xB3] = (char) (0x00B3); /* SUPERSCRIPT THREE*/
		windows_1252_chars[0xB4] = (char) (0x00B4); /* ACUTE ACCENT*/
		windows_1252_chars[0xB5] = (char) (0x00B5); /* MICRO SIGN*/
		windows_1252_chars[0xB6] = (char) (0x00B6); /* PILCROW SIGN*/
		windows_1252_chars[0xB7] = (char) (0x00B7); /* MIDDLE DOT*/
		windows_1252_chars[0xB8] = (char) (0x00B8); /* CEDILLA*/
		windows_1252_chars[0xB9] = (char) (0x00B9); /* SUPERSCRIPT ONE*/
		windows_1252_chars[0xBA] = (char) (0x00BA); /* MASCULINE ORDINAL INDICATOR*/
		windows_1252_chars[0xBB] = (char) (0x00BB); /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1252_chars[0xBC] = (char) (0x00BC); /* VULGAR FRACTION ONE QUARTER*/
		windows_1252_chars[0xBD] = (char) (0x00BD); /* VULGAR FRACTION ONE HALF*/
		windows_1252_chars[0xBE] = (char) (0x00BE); /* VULGAR FRACTION THREE QUARTERS*/
		windows_1252_chars[0xBF] = (char) (0x00BF); /* INVERTED QUESTION MARK*/
		windows_1252_chars[0xC0] = (char) (0x00C0); /* LATIN CAPITAL LETTER A WITH GRAVE*/
		windows_1252_chars[0xC1] = (char) (0x00C1); /* LATIN CAPITAL LETTER A WITH ACUTE*/
		windows_1252_chars[0xC2] = (char) (0x00C2); /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX*/
		windows_1252_chars[0xC3] = (char) (0x00C3); /* LATIN CAPITAL LETTER A WITH TILDE*/
		windows_1252_chars[0xC4] = (char) (0x00C4); /* LATIN CAPITAL LETTER A WITH DIAERESIS*/
		windows_1252_chars[0xC5] = (char) (0x00C5); /* LATIN CAPITAL LETTER A WITH RING ABOVE*/
		windows_1252_chars[0xC6] = (char) (0x00C6); /* LATIN CAPITAL LETTER AE*/
		windows_1252_chars[0xC7] = (char) (0x00C7); /* LATIN CAPITAL LETTER C WITH CEDILLA*/
		windows_1252_chars[0xC8] = (char) (0x00C8); /* LATIN CAPITAL LETTER E WITH GRAVE*/
		windows_1252_chars[0xC9] = (char) (0x00C9); /* LATIN CAPITAL LETTER E WITH ACUTE*/
		windows_1252_chars[0xCA] = (char) (0x00CA); /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX*/
		windows_1252_chars[0xCB] = (char) (0x00CB); /* LATIN CAPITAL LETTER E WITH DIAERESIS*/
		windows_1252_chars[0xCC] = (char) (0x00CC); /* LATIN CAPITAL LETTER I WITH GRAVE*/
		windows_1252_chars[0xCD] = (char) (0x00CD); /* LATIN CAPITAL LETTER I WITH ACUTE*/
		windows_1252_chars[0xCE] = (char) (0x00CE); /* LATIN CAPITAL LETTER I WITH CIRCUMFLEX*/
		windows_1252_chars[0xCF] = (char) (0x00CF); /* LATIN CAPITAL LETTER I WITH DIAERESIS*/
		windows_1252_chars[0xD0] = (char) (0x00D0); /* LATIN CAPITAL LETTER ETH*/
		windows_1252_chars[0xD1] = (char) (0x00D1); /* LATIN CAPITAL LETTER N WITH TILDE*/
		windows_1252_chars[0xD2] = (char) (0x00D2); /* LATIN CAPITAL LETTER O WITH GRAVE*/
		windows_1252_chars[0xD3] = (char) (0x00D3); /* LATIN CAPITAL LETTER O WITH ACUTE*/
		windows_1252_chars[0xD4] = (char) (0x00D4); /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX*/
		windows_1252_chars[0xD5] = (char) (0x00D5); /* LATIN CAPITAL LETTER O WITH TILDE*/
		windows_1252_chars[0xD6] = (char) (0x00D6); /* LATIN CAPITAL LETTER O WITH DIAERESIS*/
		windows_1252_chars[0xD7] = (char) (0x00D7); /* MULTIPLICATION SIGN*/
		windows_1252_chars[0xD8] = (char) (0x00D8); /* LATIN CAPITAL LETTER O WITH STROKE*/
		windows_1252_chars[0xD9] = (char) (0x00D9); /* LATIN CAPITAL LETTER U WITH GRAVE*/
		windows_1252_chars[0xDA] = (char) (0x00DA); /* LATIN CAPITAL LETTER U WITH ACUTE*/
		windows_1252_chars[0xDB] = (char) (0x00DB); /* LATIN CAPITAL LETTER U WITH CIRCUMFLEX*/
		windows_1252_chars[0xDC] = (char) (0x00DC); /* LATIN CAPITAL LETTER U WITH DIAERESIS*/
		windows_1252_chars[0xDD] = (char) (0x00DD); /* LATIN CAPITAL LETTER Y WITH ACUTE*/
		windows_1252_chars[0xDE] = (char) (0x00DE); /* LATIN CAPITAL LETTER THORN*/
		windows_1252_chars[0xDF] = (char) (0x00DF); /* LATIN SMALL LETTER SHARP S*/
		windows_1252_chars[0xE0] = (char) (0x00E0); /* LATIN SMALL LETTER A WITH GRAVE*/
		windows_1252_chars[0xE1] = (char) (0x00E1); /* LATIN SMALL LETTER A WITH ACUTE*/
		windows_1252_chars[0xE2] = (char) (0x00E2); /* LATIN SMALL LETTER A WITH CIRCUMFLEX*/
		windows_1252_chars[0xE3] = (char) (0x00E3); /* LATIN SMALL LETTER A WITH TILDE*/
		windows_1252_chars[0xE4] = (char) (0x00E4); /* LATIN SMALL LETTER A WITH DIAERESIS*/
		windows_1252_chars[0xE5] = (char) (0x00E5); /* LATIN SMALL LETTER A WITH RING ABOVE*/
		windows_1252_chars[0xE6] = (char) (0x00E6); /* LATIN SMALL LETTER AE*/
		windows_1252_chars[0xE7] = (char) (0x00E7); /* LATIN SMALL LETTER C WITH CEDILLA*/
		windows_1252_chars[0xE8] = (char) (0x00E8); /* LATIN SMALL LETTER E WITH GRAVE*/
		windows_1252_chars[0xE9] = (char) (0x00E9); /* LATIN SMALL LETTER E WITH ACUTE*/
		windows_1252_chars[0xEA] = (char) (0x00EA); /* LATIN SMALL LETTER E WITH CIRCUMFLEX*/
		windows_1252_chars[0xEB] = (char) (0x00EB); /* LATIN SMALL LETTER E WITH DIAERESIS*/
		windows_1252_chars[0xEC] = (char) (0x00EC); /* LATIN SMALL LETTER I WITH GRAVE*/
		windows_1252_chars[0xED] = (char) (0x00ED); /* LATIN SMALL LETTER I WITH ACUTE*/
		windows_1252_chars[0xEE] = (char) (0x00EE); /* LATIN SMALL LETTER I WITH CIRCUMFLEX*/
		windows_1252_chars[0xEF] = (char) (0x00EF); /* LATIN SMALL LETTER I WITH DIAERESIS*/
		windows_1252_chars[0xF0] = (char) (0x00F0); /* LATIN SMALL LETTER ETH*/
		windows_1252_chars[0xF1] = (char) (0x00F1); /* LATIN SMALL LETTER N WITH TILDE*/
		windows_1252_chars[0xF2] = (char) (0x00F2); /* LATIN SMALL LETTER O WITH GRAVE*/
		windows_1252_chars[0xF3] = (char) (0x00F3); /* LATIN SMALL LETTER O WITH ACUTE*/
		windows_1252_chars[0xF4] = (char) (0x00F4); /* LATIN SMALL LETTER O WITH CIRCUMFLEX*/
		windows_1252_chars[0xF5] = (char) (0x00F5); /* LATIN SMALL LETTER O WITH TILDE*/
		windows_1252_chars[0xF6] = (char) (0x00F6); /* LATIN SMALL LETTER O WITH DIAERESIS*/
		windows_1252_chars[0xF7] = (char) (0x00F7); /* DIVISION SIGN*/
		windows_1252_chars[0xF8] = (char) (0x00F8); /* LATIN SMALL LETTER O WITH STROKE*/
		windows_1252_chars[0xF9] = (char) (0x00F9); /* LATIN SMALL LETTER U WITH GRAVE*/
		windows_1252_chars[0xFA] = (char) (0x00FA); /* LATIN SMALL LETTER U WITH ACUTE*/
		windows_1252_chars[0xFB] = (char) (0x00FB); /* LATIN SMALL LETTER U WITH CIRCUMFLEX*/
		windows_1252_chars[0xFC] = (char) (0x00FC); /* LATIN SMALL LETTER U WITH DIAERESIS*/
		windows_1252_chars[0xFD] = (char) (0x00FD); /* LATIN SMALL LETTER Y WITH ACUTE*/
		windows_1252_chars[0xFE] = (char) (0x00FE); /* LATIN SMALL LETTER THORN*/
		windows_1252_chars[0xFF] = (char) (0x00FF); /* LATIN SMALL LETTER Y WITH DIAERESIS*/

		windows_1252_chars_ready = TRUE;
	}
}
void windows_1253_chars_init(){
	int i;
	if (windows_1253_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			windows_1253_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			windows_1253_chars[i] = (char) (0xfffd);
		}
		windows_1253_chars[0x80] = (char) (0x20AC); /* EURO SIGN*/
		windows_1253_chars[0x82] = (char) (0x201A); /* SINGLE LOW-9 QUOTATION MARK*/
		windows_1253_chars[0x83] = (char) (0x0192); /* LATIN SMALL LETTER F WITH HOOK*/
		windows_1253_chars[0x84] = (char) (0x201E); /* DOUBLE LOW-9 QUOTATION MARK*/
		windows_1253_chars[0x85] = (char) (0x2026); /* HORIZONTAL ELLIPSIS*/
		windows_1253_chars[0x86] = (char) (0x2020); /* DAGGER*/
		windows_1253_chars[0x87] = (char) (0x2021); /* DOUBLE DAGGER*/
		windows_1253_chars[0x89] = (char) (0x2030); /* PER MILLE SIGN*/
		windows_1253_chars[0x8B] = (char) (0x2039); /* SINGLE LEFT-POINTING ANGLE QUOTATION MARK*/
		windows_1253_chars[0x91] = (char) (0x2018); /* LEFT SINGLE QUOTATION MARK*/
		windows_1253_chars[0x92] = (char) (0x2019); /* RIGHT SINGLE QUOTATION MARK*/
		windows_1253_chars[0x93] = (char) (0x201C); /* LEFT DOUBLE QUOTATION MARK*/
		windows_1253_chars[0x94] = (char) (0x201D); /* RIGHT DOUBLE QUOTATION MARK*/
		windows_1253_chars[0x95] = (char) (0x2022); /* BULLET*/
		windows_1253_chars[0x96] = (char) (0x2013); /* EN DASH*/
		windows_1253_chars[0x97] = (char) (0x2014); /* EM DASH*/
		windows_1253_chars[0x99] = (char) (0x2122); /* TRADE MARK SIGN*/
		windows_1253_chars[0x9B] = (char) (0x203A); /* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK*/
		windows_1253_chars[0xA0] = (char) (0x00A0); /* NO-BREAK SPACE*/
		windows_1253_chars[0xA1] = (char) (0x0385); /* GREEK DIALYTIKA TONOS*/
		windows_1253_chars[0xA2] = (char) (0x0386); /* GREEK CAPITAL LETTER ALPHA WITH TONOS*/
		windows_1253_chars[0xA3] = (char) (0x00A3); /* POUND SIGN*/
		windows_1253_chars[0xA4] = (char) (0x00A4); /* CURRENCY SIGN*/
		windows_1253_chars[0xA5] = (char) (0x00A5); /* YEN SIGN*/
		windows_1253_chars[0xA6] = (char) (0x00A6); /* BROKEN BAR*/
		windows_1253_chars[0xA7] = (char) (0x00A7); /* SECTION SIGN*/
		windows_1253_chars[0xA8] = (char) (0x00A8); /* DIAERESIS*/
		windows_1253_chars[0xA9] = (char) (0x00A9); /* COPYRIGHT SIGN*/
		windows_1253_chars[0xAB] = (char) (0x00AB); /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1253_chars[0xAC] = (char) (0x00AC); /* NOT SIGN*/
		windows_1253_chars[0xAD] = (char) (0x00AD); /* SOFT HYPHEN*/
		windows_1253_chars[0xAE] = (char) (0x00AE); /* REGISTERED SIGN*/
		windows_1253_chars[0xAF] = (char) (0x2015); /* HORIZONTAL BAR*/
		windows_1253_chars[0xB0] = (char) (0x00B0); /* DEGREE SIGN*/
		windows_1253_chars[0xB1] = (char) (0x00B1); /* PLUS-MINUS SIGN*/
		windows_1253_chars[0xB2] = (char) (0x00B2); /* SUPERSCRIPT TWO*/
		windows_1253_chars[0xB3] = (char) (0x00B3); /* SUPERSCRIPT THREE*/
		windows_1253_chars[0xB4] = (char) (0x0384); /* GREEK TONOS*/
		windows_1253_chars[0xB5] = (char) (0x00B5); /* MICRO SIGN*/
		windows_1253_chars[0xB6] = (char) (0x00B6); /* PILCROW SIGN*/
		windows_1253_chars[0xB7] = (char) (0x00B7); /* MIDDLE DOT*/
		windows_1253_chars[0xB8] = (char) (0x0388); /* GREEK CAPITAL LETTER EPSILON WITH TONOS*/
		windows_1253_chars[0xB9] = (char) (0x0389); /* GREEK CAPITAL LETTER ETA WITH TONOS*/
		windows_1253_chars[0xBA] = (char) (0x038A); /* GREEK CAPITAL LETTER IOTA WITH TONOS*/
		windows_1253_chars[0xBB] = (char) (0x00BB); /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1253_chars[0xBC] = (char) (0x038C); /* GREEK CAPITAL LETTER OMICRON WITH TONOS*/
		windows_1253_chars[0xBD] = (char) (0x00BD); /* VULGAR FRACTION ONE HALF*/
		windows_1253_chars[0xBE] = (char) (0x038E); /* GREEK CAPITAL LETTER UPSILON WITH TONOS*/
		windows_1253_chars[0xBF] = (char) (0x038F); /* GREEK CAPITAL LETTER OMEGA WITH TONOS*/
		windows_1253_chars[0xC0] = (char) (0x0390); /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS*/
		windows_1253_chars[0xC1] = (char) (0x0391); /* GREEK CAPITAL LETTER ALPHA*/
		windows_1253_chars[0xC2] = (char) (0x0392); /* GREEK CAPITAL LETTER BETA*/
		windows_1253_chars[0xC3] = (char) (0x0393); /* GREEK CAPITAL LETTER GAMMA*/
		windows_1253_chars[0xC4] = (char) (0x0394); /* GREEK CAPITAL LETTER DELTA*/
		windows_1253_chars[0xC5] = (char) (0x0395); /* GREEK CAPITAL LETTER EPSILON*/
		windows_1253_chars[0xC6] = (char) (0x0396); /* GREEK CAPITAL LETTER ZETA*/
		windows_1253_chars[0xC7] = (char) (0x0397); /* GREEK CAPITAL LETTER ETA*/
		windows_1253_chars[0xC8] = (char) (0x0398); /* GREEK CAPITAL LETTER THETA*/
		windows_1253_chars[0xC9] = (char) (0x0399); /* GREEK CAPITAL LETTER IOTA*/
		windows_1253_chars[0xCA] = (char) (0x039A); /* GREEK CAPITAL LETTER KAPPA*/
		windows_1253_chars[0xCB] = (char) (0x039B); /* GREEK CAPITAL LETTER LAMDA*/
		windows_1253_chars[0xCC] = (char) (0x039C); /* GREEK CAPITAL LETTER MU*/
		windows_1253_chars[0xCD] = (char) (0x039D); /* GREEK CAPITAL LETTER NU*/
		windows_1253_chars[0xCE] = (char) (0x039E); /* GREEK CAPITAL LETTER XI*/
		windows_1253_chars[0xCF] = (char) (0x039F); /* GREEK CAPITAL LETTER OMICRON*/
		windows_1253_chars[0xD0] = (char) (0x03A0); /* GREEK CAPITAL LETTER PI*/
		windows_1253_chars[0xD1] = (char) (0x03A1); /* GREEK CAPITAL LETTER RHO*/
		windows_1253_chars[0xD3] = (char) (0x03A3); /* GREEK CAPITAL LETTER SIGMA*/
		windows_1253_chars[0xD4] = (char) (0x03A4); /* GREEK CAPITAL LETTER TAU*/
		windows_1253_chars[0xD5] = (char) (0x03A5); /* GREEK CAPITAL LETTER UPSILON*/
		windows_1253_chars[0xD6] = (char) (0x03A6); /* GREEK CAPITAL LETTER PHI*/
		windows_1253_chars[0xD7] = (char) (0x03A7); /* GREEK CAPITAL LETTER CHI*/
		windows_1253_chars[0xD8] = (char) (0x03A8); /* GREEK CAPITAL LETTER PSI*/
		windows_1253_chars[0xD9] = (char) (0x03A9); /* GREEK CAPITAL LETTER OMEGA*/
		windows_1253_chars[0xDA] = (char) (0x03AA); /* GREEK CAPITAL LETTER IOTA WITH DIALYTIKA*/
		windows_1253_chars[0xDB] = (char) (0x03AB); /* GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA*/
		windows_1253_chars[0xDC] = (char) (0x03AC); /* GREEK SMALL LETTER ALPHA WITH TONOS*/
		windows_1253_chars[0xDD] = (char) (0x03AD); /* GREEK SMALL LETTER EPSILON WITH TONOS*/
		windows_1253_chars[0xDE] = (char) (0x03AE); /* GREEK SMALL LETTER ETA WITH TONOS*/
		windows_1253_chars[0xDF] = (char) (0x03AF); /* GREEK SMALL LETTER IOTA WITH TONOS*/
		windows_1253_chars[0xE0] = (char) (0x03B0); /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS*/
		windows_1253_chars[0xE1] = (char) (0x03B1); /* GREEK SMALL LETTER ALPHA*/
		windows_1253_chars[0xE2] = (char) (0x03B2); /* GREEK SMALL LETTER BETA*/
		windows_1253_chars[0xE3] = (char) (0x03B3); /* GREEK SMALL LETTER GAMMA*/
		windows_1253_chars[0xE4] = (char) (0x03B4); /* GREEK SMALL LETTER DELTA*/
		windows_1253_chars[0xE5] = (char) (0x03B5); /* GREEK SMALL LETTER EPSILON*/
		windows_1253_chars[0xE6] = (char) (0x03B6); /* GREEK SMALL LETTER ZETA*/
		windows_1253_chars[0xE7] = (char) (0x03B7); /* GREEK SMALL LETTER ETA*/
		windows_1253_chars[0xE8] = (char) (0x03B8); /* GREEK SMALL LETTER THETA*/
		windows_1253_chars[0xE9] = (char) (0x03B9); /* GREEK SMALL LETTER IOTA*/
		windows_1253_chars[0xEA] = (char) (0x03BA); /* GREEK SMALL LETTER KAPPA*/
		windows_1253_chars[0xEB] = (char) (0x03BB); /* GREEK SMALL LETTER LAMDA*/
		windows_1253_chars[0xEC] = (char) (0x03BC); /* GREEK SMALL LETTER MU*/
		windows_1253_chars[0xED] = (char) (0x03BD); /* GREEK SMALL LETTER NU*/
		windows_1253_chars[0xEE] = (char) (0x03BE); /* GREEK SMALL LETTER XI*/
		windows_1253_chars[0xEF] = (char) (0x03BF); /* GREEK SMALL LETTER OMICRON*/
		windows_1253_chars[0xF0] = (char) (0x03C0); /* GREEK SMALL LETTER PI*/
		windows_1253_chars[0xF1] = (char) (0x03C1); /* GREEK SMALL LETTER RHO*/
		windows_1253_chars[0xF2] = (char) (0x03C2); /* GREEK SMALL LETTER FINAL SIGMA*/
		windows_1253_chars[0xF3] = (char) (0x03C3); /* GREEK SMALL LETTER SIGMA*/
		windows_1253_chars[0xF4] = (char) (0x03C4); /* GREEK SMALL LETTER TAU*/
		windows_1253_chars[0xF5] = (char) (0x03C5); /* GREEK SMALL LETTER UPSILON*/
		windows_1253_chars[0xF6] = (char) (0x03C6); /* GREEK SMALL LETTER PHI*/
		windows_1253_chars[0xF7] = (char) (0x03C7); /* GREEK SMALL LETTER CHI*/
		windows_1253_chars[0xF8] = (char) (0x03C8); /* GREEK SMALL LETTER PSI*/
		windows_1253_chars[0xF9] = (char) (0x03C9); /* GREEK SMALL LETTER OMEGA*/
		windows_1253_chars[0xFA] = (char) (0x03CA); /* GREEK SMALL LETTER IOTA WITH DIALYTIKA*/
		windows_1253_chars[0xFB] = (char) (0x03CB); /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA*/
		windows_1253_chars[0xFC] = (char) (0x03CC); /* GREEK SMALL LETTER OMICRON WITH TONOS*/
		windows_1253_chars[0xFD] = (char) (0x03CD); /* GREEK SMALL LETTER UPSILON WITH TONOS*/
		windows_1253_chars[0xFE] = (char) (0x03CE); /* GREEK SMALL LETTER OMEGA WITH TONOS*/

		windows_1253_chars_ready = TRUE;
	}
}
void windows_1254_chars_init(){
	int i;
	if (windows_1254_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			windows_1254_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			windows_1254_chars[i] = (char) (0xfffd);
		}
		windows_1254_chars[0x80] = (char) (0x20AC); /* EURO SIGN*/
		windows_1254_chars[0x82] = (char) (0x201A); /* SINGLE LOW-9 QUOTATION MARK*/
		windows_1254_chars[0x83] = (char) (0x0192); /* LATIN SMALL LETTER F WITH HOOK*/
		windows_1254_chars[0x84] = (char) (0x201E); /* DOUBLE LOW-9 QUOTATION MARK*/
		windows_1254_chars[0x85] = (char) (0x2026); /* HORIZONTAL ELLIPSIS*/
		windows_1254_chars[0x86] = (char) (0x2020); /* DAGGER*/
		windows_1254_chars[0x87] = (char) (0x2021); /* DOUBLE DAGGER*/
		windows_1254_chars[0x88] = (char) (0x02C6); /* MODIFIER LETTER CIRCUMFLEX ACCENT*/
		windows_1254_chars[0x89] = (char) (0x2030); /* PER MILLE SIGN*/
		windows_1254_chars[0x8A] = (char) (0x0160); /* LATIN CAPITAL LETTER S WITH CARON*/
		windows_1254_chars[0x8B] = (char) (0x2039); /* SINGLE LEFT-POINTING ANGLE QUOTATION MARK*/
		windows_1254_chars[0x8C] = (char) (0x0152); /* LATIN CAPITAL LIGATURE OE*/
		windows_1254_chars[0x91] = (char) (0x2018); /* LEFT SINGLE QUOTATION MARK*/
		windows_1254_chars[0x92] = (char) (0x2019); /* RIGHT SINGLE QUOTATION MARK*/
		windows_1254_chars[0x93] = (char) (0x201C); /* LEFT DOUBLE QUOTATION MARK*/
		windows_1254_chars[0x94] = (char) (0x201D); /* RIGHT DOUBLE QUOTATION MARK*/
		windows_1254_chars[0x95] = (char) (0x2022); /* BULLET*/
		windows_1254_chars[0x96] = (char) (0x2013); /* EN DASH*/
		windows_1254_chars[0x97] = (char) (0x2014); /* EM DASH*/
		windows_1254_chars[0x98] = (char) (0x02DC); /* SMALL TILDE*/
		windows_1254_chars[0x99] = (char) (0x2122); /* TRADE MARK SIGN*/
		windows_1254_chars[0x9A] = (char) (0x0161); /* LATIN SMALL LETTER S WITH CARON*/
		windows_1254_chars[0x9B] = (char) (0x203A); /* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK*/
		windows_1254_chars[0x9C] = (char) (0x0153); /* LATIN SMALL LIGATURE OE*/
		windows_1254_chars[0x9F] = (char) (0x0178); /* LATIN CAPITAL LETTER Y WITH DIAERESIS*/
		windows_1254_chars[0xA0] = (char) (0x00A0); /* NO-BREAK SPACE*/
		windows_1254_chars[0xA1] = (char) (0x00A1); /* INVERTED EXCLAMATION MARK*/
		windows_1254_chars[0xA2] = (char) (0x00A2); /* CENT SIGN*/
		windows_1254_chars[0xA3] = (char) (0x00A3); /* POUND SIGN*/
		windows_1254_chars[0xA4] = (char) (0x00A4); /* CURRENCY SIGN*/
		windows_1254_chars[0xA5] = (char) (0x00A5); /* YEN SIGN*/
		windows_1254_chars[0xA6] = (char) (0x00A6); /* BROKEN BAR*/
		windows_1254_chars[0xA7] = (char) (0x00A7); /* SECTION SIGN*/
		windows_1254_chars[0xA8] = (char) (0x00A8); /* DIAERESIS*/
		windows_1254_chars[0xA9] = (char) (0x00A9); /* COPYRIGHT SIGN*/
		windows_1254_chars[0xAA] = (char) (0x00AA); /* FEMININE ORDINAL INDICATOR*/
		windows_1254_chars[0xAB] = (char) (0x00AB); /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1254_chars[0xAC] = (char) (0x00AC); /* NOT SIGN*/
		windows_1254_chars[0xAD] = (char) (0x00AD); /* SOFT HYPHEN*/
		windows_1254_chars[0xAE] = (char) (0x00AE); /* REGISTERED SIGN*/
		windows_1254_chars[0xAF] = (char) (0x00AF); /* MACRON*/
		windows_1254_chars[0xB0] = (char) (0x00B0); /* DEGREE SIGN*/
		windows_1254_chars[0xB1] = (char) (0x00B1); /* PLUS-MINUS SIGN*/
		windows_1254_chars[0xB2] = (char) (0x00B2); /* SUPERSCRIPT TWO*/
		windows_1254_chars[0xB3] = (char) (0x00B3); /* SUPERSCRIPT THREE*/
		windows_1254_chars[0xB4] = (char) (0x00B4); /* ACUTE ACCENT*/
		windows_1254_chars[0xB5] = (char) (0x00B5); /* MICRO SIGN*/
		windows_1254_chars[0xB6] = (char) (0x00B6); /* PILCROW SIGN*/
		windows_1254_chars[0xB7] = (char) (0x00B7); /* MIDDLE DOT*/
		windows_1254_chars[0xB8] = (char) (0x00B8); /* CEDILLA*/
		windows_1254_chars[0xB9] = (char) (0x00B9); /* SUPERSCRIPT ONE*/
		windows_1254_chars[0xBA] = (char) (0x00BA); /* MASCULINE ORDINAL INDICATOR*/
		windows_1254_chars[0xBB] = (char) (0x00BB); /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1254_chars[0xBC] = (char) (0x00BC); /* VULGAR FRACTION ONE QUARTER*/
		windows_1254_chars[0xBD] = (char) (0x00BD); /* VULGAR FRACTION ONE HALF*/
		windows_1254_chars[0xBE] = (char) (0x00BE); /* VULGAR FRACTION THREE QUARTERS*/
		windows_1254_chars[0xBF] = (char) (0x00BF); /* INVERTED QUESTION MARK*/
		windows_1254_chars[0xC0] = (char) (0x00C0); /* LATIN CAPITAL LETTER A WITH GRAVE*/
		windows_1254_chars[0xC1] = (char) (0x00C1); /* LATIN CAPITAL LETTER A WITH ACUTE*/
		windows_1254_chars[0xC2] = (char) (0x00C2); /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX*/
		windows_1254_chars[0xC3] = (char) (0x00C3); /* LATIN CAPITAL LETTER A WITH TILDE*/
		windows_1254_chars[0xC4] = (char) (0x00C4); /* LATIN CAPITAL LETTER A WITH DIAERESIS*/
		windows_1254_chars[0xC5] = (char) (0x00C5); /* LATIN CAPITAL LETTER A WITH RING ABOVE*/
		windows_1254_chars[0xC6] = (char) (0x00C6); /* LATIN CAPITAL LETTER AE*/
		windows_1254_chars[0xC7] = (char) (0x00C7); /* LATIN CAPITAL LETTER C WITH CEDILLA*/
		windows_1254_chars[0xC8] = (char) (0x00C8); /* LATIN CAPITAL LETTER E WITH GRAVE*/
		windows_1254_chars[0xC9] = (char) (0x00C9); /* LATIN CAPITAL LETTER E WITH ACUTE*/
		windows_1254_chars[0xCA] = (char) (0x00CA); /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX*/
		windows_1254_chars[0xCB] = (char) (0x00CB); /* LATIN CAPITAL LETTER E WITH DIAERESIS*/
		windows_1254_chars[0xCC] = (char) (0x00CC); /* LATIN CAPITAL LETTER I WITH GRAVE*/
		windows_1254_chars[0xCD] = (char) (0x00CD); /* LATIN CAPITAL LETTER I WITH ACUTE*/
		windows_1254_chars[0xCE] = (char) (0x00CE); /* LATIN CAPITAL LETTER I WITH CIRCUMFLEX*/
		windows_1254_chars[0xCF] = (char) (0x00CF); /* LATIN CAPITAL LETTER I WITH DIAERESIS*/
		windows_1254_chars[0xD0] = (char) (0x011E); /* LATIN CAPITAL LETTER G WITH BREVE*/
		windows_1254_chars[0xD1] = (char) (0x00D1); /* LATIN CAPITAL LETTER N WITH TILDE*/
		windows_1254_chars[0xD2] = (char) (0x00D2); /* LATIN CAPITAL LETTER O WITH GRAVE*/
		windows_1254_chars[0xD3] = (char) (0x00D3); /* LATIN CAPITAL LETTER O WITH ACUTE*/
		windows_1254_chars[0xD4] = (char) (0x00D4); /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX*/
		windows_1254_chars[0xD5] = (char) (0x00D5); /* LATIN CAPITAL LETTER O WITH TILDE*/
		windows_1254_chars[0xD6] = (char) (0x00D6); /* LATIN CAPITAL LETTER O WITH DIAERESIS*/
		windows_1254_chars[0xD7] = (char) (0x00D7); /* MULTIPLICATION SIGN*/
		windows_1254_chars[0xD8] = (char) (0x00D8); /* LATIN CAPITAL LETTER O WITH STROKE*/
		windows_1254_chars[0xD9] = (char) (0x00D9); /* LATIN CAPITAL LETTER U WITH GRAVE*/
		windows_1254_chars[0xDA] = (char) (0x00DA); /* LATIN CAPITAL LETTER U WITH ACUTE*/
		windows_1254_chars[0xDB] = (char) (0x00DB); /* LATIN CAPITAL LETTER U WITH CIRCUMFLEX*/
		windows_1254_chars[0xDC] = (char) (0x00DC); /* LATIN CAPITAL LETTER U WITH DIAERESIS*/
		windows_1254_chars[0xDD] = (char) (0x0130); /* LATIN CAPITAL LETTER I WITH DOT ABOVE*/
		windows_1254_chars[0xDE] = (char) (0x015E); /* LATIN CAPITAL LETTER S WITH CEDILLA*/
		windows_1254_chars[0xDF] = (char) (0x00DF); /* LATIN SMALL LETTER SHARP S*/
		windows_1254_chars[0xE0] = (char) (0x00E0); /* LATIN SMALL LETTER A WITH GRAVE*/
		windows_1254_chars[0xE1] = (char) (0x00E1); /* LATIN SMALL LETTER A WITH ACUTE*/
		windows_1254_chars[0xE2] = (char) (0x00E2); /* LATIN SMALL LETTER A WITH CIRCUMFLEX*/
		windows_1254_chars[0xE3] = (char) (0x00E3); /* LATIN SMALL LETTER A WITH TILDE*/
		windows_1254_chars[0xE4] = (char) (0x00E4); /* LATIN SMALL LETTER A WITH DIAERESIS*/
		windows_1254_chars[0xE5] = (char) (0x00E5); /* LATIN SMALL LETTER A WITH RING ABOVE*/
		windows_1254_chars[0xE6] = (char) (0x00E6); /* LATIN SMALL LETTER AE*/
		windows_1254_chars[0xE7] = (char) (0x00E7); /* LATIN SMALL LETTER C WITH CEDILLA*/
		windows_1254_chars[0xE8] = (char) (0x00E8); /* LATIN SMALL LETTER E WITH GRAVE*/
		windows_1254_chars[0xE9] = (char) (0x00E9); /* LATIN SMALL LETTER E WITH ACUTE*/
		windows_1254_chars[0xEA] = (char) (0x00EA); /* LATIN SMALL LETTER E WITH CIRCUMFLEX*/
		windows_1254_chars[0xEB] = (char) (0x00EB); /* LATIN SMALL LETTER E WITH DIAERESIS*/
		windows_1254_chars[0xEC] = (char) (0x00EC); /* LATIN SMALL LETTER I WITH GRAVE*/
		windows_1254_chars[0xED] = (char) (0x00ED); /* LATIN SMALL LETTER I WITH ACUTE*/
		windows_1254_chars[0xEE] = (char) (0x00EE); /* LATIN SMALL LETTER I WITH CIRCUMFLEX*/
		windows_1254_chars[0xEF] = (char) (0x00EF); /* LATIN SMALL LETTER I WITH DIAERESIS*/
		windows_1254_chars[0xF0] = (char) (0x011F); /* LATIN SMALL LETTER G WITH BREVE*/
		windows_1254_chars[0xF1] = (char) (0x00F1); /* LATIN SMALL LETTER N WITH TILDE*/
		windows_1254_chars[0xF2] = (char) (0x00F2); /* LATIN SMALL LETTER O WITH GRAVE*/
		windows_1254_chars[0xF3] = (char) (0x00F3); /* LATIN SMALL LETTER O WITH ACUTE*/
		windows_1254_chars[0xF4] = (char) (0x00F4); /* LATIN SMALL LETTER O WITH CIRCUMFLEX*/
		windows_1254_chars[0xF5] = (char) (0x00F5); /* LATIN SMALL LETTER O WITH TILDE*/
		windows_1254_chars[0xF6] = (char) (0x00F6); /* LATIN SMALL LETTER O WITH DIAERESIS*/
		windows_1254_chars[0xF7] = (char) (0x00F7); /* DIVISION SIGN*/
		windows_1254_chars[0xF8] = (char) (0x00F8); /* LATIN SMALL LETTER O WITH STROKE*/
		windows_1254_chars[0xF9] = (char) (0x00F9); /* LATIN SMALL LETTER U WITH GRAVE*/
		windows_1254_chars[0xFA] = (char) (0x00FA); /* LATIN SMALL LETTER U WITH ACUTE*/
		windows_1254_chars[0xFB] = (char) (0x00FB); /* LATIN SMALL LETTER U WITH CIRCUMFLEX*/
		windows_1254_chars[0xFC] = (char) (0x00FC); /* LATIN SMALL LETTER U WITH DIAERESIS*/
		windows_1254_chars[0xFD] = (char) (0x0131); /* LATIN SMALL LETTER DOTLESS I*/
		windows_1254_chars[0xFE] = (char) (0x015F); /* LATIN SMALL LETTER S WITH CEDILLA*/
		windows_1254_chars[0xFF] = (char) (0x00FF); /* LATIN SMALL LETTER Y WITH DIAERESIS*/

		windows_1254_chars_ready = TRUE;
	}
}
void windows_1255_chars_init(){
	int i;
	if (windows_1255_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			windows_1255_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			windows_1255_chars[i] = (char) (0xfffd);
		}
		windows_1255_chars[0x80] = (char) (0x20AC); /* EURO SIGN*/
		windows_1255_chars[0x82] = (char) (0x201A); /* SINGLE LOW-9 QUOTATION MARK*/
		windows_1255_chars[0x83] = (char) (0x0192); /* LATIN SMALL LETTER F WITH HOOK*/
		windows_1255_chars[0x84] = (char) (0x201E); /* DOUBLE LOW-9 QUOTATION MARK*/
		windows_1255_chars[0x85] = (char) (0x2026); /* HORIZONTAL ELLIPSIS*/
		windows_1255_chars[0x86] = (char) (0x2020); /* DAGGER*/
		windows_1255_chars[0x87] = (char) (0x2021); /* DOUBLE DAGGER*/
		windows_1255_chars[0x88] = (char) (0x02C6); /* MODIFIER LETTER CIRCUMFLEX ACCENT*/
		windows_1255_chars[0x89] = (char) (0x2030); /* PER MILLE SIGN*/
		windows_1255_chars[0x8B] = (char) (0x2039); /* SINGLE LEFT-POINTING ANGLE QUOTATION MARK*/
		windows_1255_chars[0x91] = (char) (0x2018); /* LEFT SINGLE QUOTATION MARK*/
		windows_1255_chars[0x92] = (char) (0x2019); /* RIGHT SINGLE QUOTATION MARK*/
		windows_1255_chars[0x93] = (char) (0x201C); /* LEFT DOUBLE QUOTATION MARK*/
		windows_1255_chars[0x94] = (char) (0x201D); /* RIGHT DOUBLE QUOTATION MARK*/
		windows_1255_chars[0x95] = (char) (0x2022); /* BULLET*/
		windows_1255_chars[0x96] = (char) (0x2013); /* EN DASH*/
		windows_1255_chars[0x97] = (char) (0x2014); /* EM DASH*/
		windows_1255_chars[0x98] = (char) (0x02DC); /* SMALL TILDE*/
		windows_1255_chars[0x99] = (char) (0x2122); /* TRADE MARK SIGN*/
		windows_1255_chars[0x9B] = (char) (0x203A); /* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK*/
		windows_1255_chars[0xA0] = (char) (0x00A0); /* NO-BREAK SPACE*/
		windows_1255_chars[0xA1] = (char) (0x00A1); /* INVERTED EXCLAMATION MARK*/
		windows_1255_chars[0xA2] = (char) (0x00A2); /* CENT SIGN*/
		windows_1255_chars[0xA3] = (char) (0x00A3); /* POUND SIGN*/
		windows_1255_chars[0xA4] = (char) (0x20AA); /* NEW SHEQEL SIGN*/
		windows_1255_chars[0xA5] = (char) (0x00A5); /* YEN SIGN*/
		windows_1255_chars[0xA6] = (char) (0x00A6); /* BROKEN BAR*/
		windows_1255_chars[0xA7] = (char) (0x00A7); /* SECTION SIGN*/
		windows_1255_chars[0xA8] = (char) (0x00A8); /* DIAERESIS*/
		windows_1255_chars[0xA9] = (char) (0x00A9); /* COPYRIGHT SIGN*/
		windows_1255_chars[0xAA] = (char) (0x00D7); /* MULTIPLICATION SIGN*/
		windows_1255_chars[0xAB] = (char) (0x00AB); /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1255_chars[0xAC] = (char) (0x00AC); /* NOT SIGN*/
		windows_1255_chars[0xAD] = (char) (0x00AD); /* SOFT HYPHEN*/
		windows_1255_chars[0xAE] = (char) (0x00AE); /* REGISTERED SIGN*/
		windows_1255_chars[0xAF] = (char) (0x00AF); /* MACRON*/
		windows_1255_chars[0xB0] = (char) (0x00B0); /* DEGREE SIGN*/
		windows_1255_chars[0xB1] = (char) (0x00B1); /* PLUS-MINUS SIGN*/
		windows_1255_chars[0xB2] = (char) (0x00B2); /* SUPERSCRIPT TWO*/
		windows_1255_chars[0xB3] = (char) (0x00B3); /* SUPERSCRIPT THREE*/
		windows_1255_chars[0xB4] = (char) (0x00B4); /* ACUTE ACCENT*/
		windows_1255_chars[0xB5] = (char) (0x00B5); /* MICRO SIGN*/
		windows_1255_chars[0xB6] = (char) (0x00B6); /* PILCROW SIGN*/
		windows_1255_chars[0xB7] = (char) (0x00B7); /* MIDDLE DOT*/
		windows_1255_chars[0xB8] = (char) (0x00B8); /* CEDILLA*/
		windows_1255_chars[0xB9] = (char) (0x00B9); /* SUPERSCRIPT ONE*/
		windows_1255_chars[0xBA] = (char) (0x00F7); /* DIVISION SIGN*/
		windows_1255_chars[0xBB] = (char) (0x00BB); /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1255_chars[0xBC] = (char) (0x00BC); /* VULGAR FRACTION ONE QUARTER*/
		windows_1255_chars[0xBD] = (char) (0x00BD); /* VULGAR FRACTION ONE HALF*/
		windows_1255_chars[0xBE] = (char) (0x00BE); /* VULGAR FRACTION THREE QUARTERS*/
		windows_1255_chars[0xBF] = (char) (0x00BF); /* INVERTED QUESTION MARK*/
		windows_1255_chars[0xC0] = (char) (0x05B0); /* HEBREW POINT SHEVA*/
		windows_1255_chars[0xC1] = (char) (0x05B1); /* HEBREW POINT HATAF SEGOL*/
		windows_1255_chars[0xC2] = (char) (0x05B2); /* HEBREW POINT HATAF PATAH*/
		windows_1255_chars[0xC3] = (char) (0x05B3); /* HEBREW POINT HATAF QAMATS*/
		windows_1255_chars[0xC4] = (char) (0x05B4); /* HEBREW POINT HIRIQ*/
		windows_1255_chars[0xC5] = (char) (0x05B5); /* HEBREW POINT TSERE*/
		windows_1255_chars[0xC6] = (char) (0x05B6); /* HEBREW POINT SEGOL*/
		windows_1255_chars[0xC7] = (char) (0x05B7); /* HEBREW POINT PATAH*/
		windows_1255_chars[0xC8] = (char) (0x05B8); /* HEBREW POINT QAMATS*/
		windows_1255_chars[0xC9] = (char) (0x05B9); /* HEBREW POINT HOLAM*/
		windows_1255_chars[0xCB] = (char) (0x05BB); /* HEBREW POINT QUBUTS*/
		windows_1255_chars[0xCC] = (char) (0x05BC); /* HEBREW POINT DAGESH OR MAPIQ*/
		windows_1255_chars[0xCD] = (char) (0x05BD); /* HEBREW POINT METEG*/
		windows_1255_chars[0xCE] = (char) (0x05BE); /* HEBREW PUNCTUATION MAQAF*/
		windows_1255_chars[0xCF] = (char) (0x05BF); /* HEBREW POINT RAFE*/
		windows_1255_chars[0xD0] = (char) (0x05C0); /* HEBREW PUNCTUATION PASEQ*/
		windows_1255_chars[0xD1] = (char) (0x05C1); /* HEBREW POINT SHIN DOT*/
		windows_1255_chars[0xD2] = (char) (0x05C2); /* HEBREW POINT SIN DOT*/
		windows_1255_chars[0xD3] = (char) (0x05C3); /* HEBREW PUNCTUATION SOF PASUQ*/
		windows_1255_chars[0xD4] = (char) (0x05F0); /* HEBREW LIGATURE YIDDISH DOUBLE VAV*/
		windows_1255_chars[0xD5] = (char) (0x05F1); /* HEBREW LIGATURE YIDDISH VAV YOD*/
		windows_1255_chars[0xD6] = (char) (0x05F2); /* HEBREW LIGATURE YIDDISH DOUBLE YOD*/
		windows_1255_chars[0xD7] = (char) (0x05F3); /* HEBREW PUNCTUATION GERESH*/
		windows_1255_chars[0xD8] = (char) (0x05F4); /* HEBREW PUNCTUATION GERSHAYIM*/
		windows_1255_chars[0xE0] = (char) (0x05D0); /* HEBREW LETTER ALEF*/
		windows_1255_chars[0xE1] = (char) (0x05D1); /* HEBREW LETTER BET*/
		windows_1255_chars[0xE2] = (char) (0x05D2); /* HEBREW LETTER GIMEL*/
		windows_1255_chars[0xE3] = (char) (0x05D3); /* HEBREW LETTER DALET*/
		windows_1255_chars[0xE4] = (char) (0x05D4); /* HEBREW LETTER HE*/
		windows_1255_chars[0xE5] = (char) (0x05D5); /* HEBREW LETTER VAV*/
		windows_1255_chars[0xE6] = (char) (0x05D6); /* HEBREW LETTER ZAYIN*/
		windows_1255_chars[0xE7] = (char) (0x05D7); /* HEBREW LETTER HET*/
		windows_1255_chars[0xE8] = (char) (0x05D8); /* HEBREW LETTER TET*/
		windows_1255_chars[0xE9] = (char) (0x05D9); /* HEBREW LETTER YOD*/
		windows_1255_chars[0xEA] = (char) (0x05DA); /* HEBREW LETTER FINAL KAF*/
		windows_1255_chars[0xEB] = (char) (0x05DB); /* HEBREW LETTER KAF*/
		windows_1255_chars[0xEC] = (char) (0x05DC); /* HEBREW LETTER LAMED*/
		windows_1255_chars[0xED] = (char) (0x05DD); /* HEBREW LETTER FINAL MEM*/
		windows_1255_chars[0xEE] = (char) (0x05DE); /* HEBREW LETTER MEM*/
		windows_1255_chars[0xEF] = (char) (0x05DF); /* HEBREW LETTER FINAL NUN*/
		windows_1255_chars[0xF0] = (char) (0x05E0); /* HEBREW LETTER NUN*/
		windows_1255_chars[0xF1] = (char) (0x05E1); /* HEBREW LETTER SAMEKH*/
		windows_1255_chars[0xF2] = (char) (0x05E2); /* HEBREW LETTER AYIN*/
		windows_1255_chars[0xF3] = (char) (0x05E3); /* HEBREW LETTER FINAL PE*/
		windows_1255_chars[0xF4] = (char) (0x05E4); /* HEBREW LETTER PE*/
		windows_1255_chars[0xF5] = (char) (0x05E5); /* HEBREW LETTER FINAL TSADI*/
		windows_1255_chars[0xF6] = (char) (0x05E6); /* HEBREW LETTER TSADI*/
		windows_1255_chars[0xF7] = (char) (0x05E7); /* HEBREW LETTER QOF*/
		windows_1255_chars[0xF8] = (char) (0x05E8); /* HEBREW LETTER RESH*/
		windows_1255_chars[0xF9] = (char) (0x05E9); /* HEBREW LETTER SHIN*/
		windows_1255_chars[0xFA] = (char) (0x05EA); /* HEBREW LETTER TAV*/
		windows_1255_chars[0xFD] = (char) (0x200E); /* LEFT-TO-RIGHT MARK*/
		windows_1255_chars[0xFE] = (char) (0x200F); /* RIGHT-TO-LEFT MARK*/

		windows_1255_chars_ready = TRUE;
	}
}
void windows_1256_chars_init(){
	int i;
	if (windows_1256_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			windows_1256_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			windows_1256_chars[i] = (char) (0xfffd);
		}
		windows_1256_chars[0x80] = (char) (0x20AC); /* EURO SIGN*/
		windows_1256_chars[0x81] = (char) (0x067E); /* ARABIC LETTER PEH*/
		windows_1256_chars[0x82] = (char) (0x201A); /* SINGLE LOW-9 QUOTATION MARK*/
		windows_1256_chars[0x83] = (char) (0x0192); /* LATIN SMALL LETTER F WITH HOOK*/
		windows_1256_chars[0x84] = (char) (0x201E); /* DOUBLE LOW-9 QUOTATION MARK*/
		windows_1256_chars[0x85] = (char) (0x2026); /* HORIZONTAL ELLIPSIS*/
		windows_1256_chars[0x86] = (char) (0x2020); /* DAGGER*/
		windows_1256_chars[0x87] = (char) (0x2021); /* DOUBLE DAGGER*/
		windows_1256_chars[0x88] = (char) (0x02C6); /* MODIFIER LETTER CIRCUMFLEX ACCENT*/
		windows_1256_chars[0x89] = (char) (0x2030); /* PER MILLE SIGN*/
		windows_1256_chars[0x8A] = (char) (0x0679); /* ARABIC LETTER TTEH*/
		windows_1256_chars[0x8B] = (char) (0x2039); /* SINGLE LEFT-POINTING ANGLE QUOTATION MARK*/
		windows_1256_chars[0x8C] = (char) (0x0152); /* LATIN CAPITAL LIGATURE OE*/
		windows_1256_chars[0x8D] = (char) (0x0686); /* ARABIC LETTER TCHEH*/
		windows_1256_chars[0x8E] = (char) (0x0698); /* ARABIC LETTER JEH*/
		windows_1256_chars[0x8F] = (char) (0x0688); /* ARABIC LETTER DDAL*/
		windows_1256_chars[0x90] = (char) (0x06AF); /* ARABIC LETTER GAF*/
		windows_1256_chars[0x91] = (char) (0x2018); /* LEFT SINGLE QUOTATION MARK*/
		windows_1256_chars[0x92] = (char) (0x2019); /* RIGHT SINGLE QUOTATION MARK*/
		windows_1256_chars[0x93] = (char) (0x201C); /* LEFT DOUBLE QUOTATION MARK*/
		windows_1256_chars[0x94] = (char) (0x201D); /* RIGHT DOUBLE QUOTATION MARK*/
		windows_1256_chars[0x95] = (char) (0x2022); /* BULLET*/
		windows_1256_chars[0x96] = (char) (0x2013); /* EN DASH*/
		windows_1256_chars[0x97] = (char) (0x2014); /* EM DASH*/
		windows_1256_chars[0x98] = (char) (0x06A9); /* ARABIC LETTER KEHEH*/
		windows_1256_chars[0x99] = (char) (0x2122); /* TRADE MARK SIGN*/
		windows_1256_chars[0x9A] = (char) (0x0691); /* ARABIC LETTER RREH*/
		windows_1256_chars[0x9B] = (char) (0x203A); /* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK*/
		windows_1256_chars[0x9C] = (char) (0x0153); /* LATIN SMALL LIGATURE OE*/
		windows_1256_chars[0x9D] = (char) (0x200C); /* ZERO WIDTH NON-JOINER*/
		windows_1256_chars[0x9E] = (char) (0x200D); /* ZERO WIDTH JOINER*/
		windows_1256_chars[0x9F] = (char) (0x06BA); /* ARABIC LETTER NOON GHUNNA*/
		windows_1256_chars[0xA0] = (char) (0x00A0); /* NO-BREAK SPACE*/
		windows_1256_chars[0xA1] = (char) (0x060C); /* ARABIC COMMA*/
		windows_1256_chars[0xA2] = (char) (0x00A2); /* CENT SIGN*/
		windows_1256_chars[0xA3] = (char) (0x00A3); /* POUND SIGN*/
		windows_1256_chars[0xA4] = (char) (0x00A4); /* CURRENCY SIGN*/
		windows_1256_chars[0xA5] = (char) (0x00A5); /* YEN SIGN*/
		windows_1256_chars[0xA6] = (char) (0x00A6); /* BROKEN BAR*/
		windows_1256_chars[0xA7] = (char) (0x00A7); /* SECTION SIGN*/
		windows_1256_chars[0xA8] = (char) (0x00A8); /* DIAERESIS*/
		windows_1256_chars[0xA9] = (char) (0x00A9); /* COPYRIGHT SIGN*/
		windows_1256_chars[0xAA] = (char) (0x06BE); /* ARABIC LETTER HEH DOACHASHMEE*/
		windows_1256_chars[0xAB] = (char) (0x00AB); /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1256_chars[0xAC] = (char) (0x00AC); /* NOT SIGN*/
		windows_1256_chars[0xAD] = (char) (0x00AD); /* SOFT HYPHEN*/
		windows_1256_chars[0xAE] = (char) (0x00AE); /* REGISTERED SIGN*/
		windows_1256_chars[0xAF] = (char) (0x00AF); /* MACRON*/
		windows_1256_chars[0xB0] = (char) (0x00B0); /* DEGREE SIGN*/
		windows_1256_chars[0xB1] = (char) (0x00B1); /* PLUS-MINUS SIGN*/
		windows_1256_chars[0xB2] = (char) (0x00B2); /* SUPERSCRIPT TWO*/
		windows_1256_chars[0xB3] = (char) (0x00B3); /* SUPERSCRIPT THREE*/
		windows_1256_chars[0xB4] = (char) (0x00B4); /* ACUTE ACCENT*/
		windows_1256_chars[0xB5] = (char) (0x00B5); /* MICRO SIGN*/
		windows_1256_chars[0xB6] = (char) (0x00B6); /* PILCROW SIGN*/
		windows_1256_chars[0xB7] = (char) (0x00B7); /* MIDDLE DOT*/
		windows_1256_chars[0xB8] = (char) (0x00B8); /* CEDILLA*/
		windows_1256_chars[0xB9] = (char) (0x00B9); /* SUPERSCRIPT ONE*/
		windows_1256_chars[0xBA] = (char) (0x061B); /* ARABIC SEMICOLON*/
		windows_1256_chars[0xBB] = (char) (0x00BB); /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1256_chars[0xBC] = (char) (0x00BC); /* VULGAR FRACTION ONE QUARTER*/
		windows_1256_chars[0xBD] = (char) (0x00BD); /* VULGAR FRACTION ONE HALF*/
		windows_1256_chars[0xBE] = (char) (0x00BE); /* VULGAR FRACTION THREE QUARTERS*/
		windows_1256_chars[0xBF] = (char) (0x061F); /* ARABIC QUESTION MARK*/
		windows_1256_chars[0xC0] = (char) (0x06C1); /* ARABIC LETTER HEH GOAL*/
		windows_1256_chars[0xC1] = (char) (0x0621); /* ARABIC LETTER HAMZA*/
		windows_1256_chars[0xC2] = (char) (0x0622); /* ARABIC LETTER ALEF WITH MADDA ABOVE*/
		windows_1256_chars[0xC3] = (char) (0x0623); /* ARABIC LETTER ALEF WITH HAMZA ABOVE*/
		windows_1256_chars[0xC4] = (char) (0x0624); /* ARABIC LETTER WAW WITH HAMZA ABOVE*/
		windows_1256_chars[0xC5] = (char) (0x0625); /* ARABIC LETTER ALEF WITH HAMZA BELOW*/
		windows_1256_chars[0xC6] = (char) (0x0626); /* ARABIC LETTER YEH WITH HAMZA ABOVE*/
		windows_1256_chars[0xC7] = (char) (0x0627); /* ARABIC LETTER ALEF*/
		windows_1256_chars[0xC8] = (char) (0x0628); /* ARABIC LETTER BEH*/
		windows_1256_chars[0xC9] = (char) (0x0629); /* ARABIC LETTER TEH MARBUTA*/
		windows_1256_chars[0xCA] = (char) (0x062A); /* ARABIC LETTER TEH*/
		windows_1256_chars[0xCB] = (char) (0x062B); /* ARABIC LETTER THEH*/
		windows_1256_chars[0xCC] = (char) (0x062C); /* ARABIC LETTER JEEM*/
		windows_1256_chars[0xCD] = (char) (0x062D); /* ARABIC LETTER HAH*/
		windows_1256_chars[0xCE] = (char) (0x062E); /* ARABIC LETTER KHAH*/
		windows_1256_chars[0xCF] = (char) (0x062F); /* ARABIC LETTER DAL*/
		windows_1256_chars[0xD0] = (char) (0x0630); /* ARABIC LETTER THAL*/
		windows_1256_chars[0xD1] = (char) (0x0631); /* ARABIC LETTER REH*/
		windows_1256_chars[0xD2] = (char) (0x0632); /* ARABIC LETTER ZAIN*/
		windows_1256_chars[0xD3] = (char) (0x0633); /* ARABIC LETTER SEEN*/
		windows_1256_chars[0xD4] = (char) (0x0634); /* ARABIC LETTER SHEEN*/
		windows_1256_chars[0xD5] = (char) (0x0635); /* ARABIC LETTER SAD*/
		windows_1256_chars[0xD6] = (char) (0x0636); /* ARABIC LETTER DAD*/
		windows_1256_chars[0xD7] = (char) (0x00D7); /* MULTIPLICATION SIGN*/
		windows_1256_chars[0xD8] = (char) (0x0637); /* ARABIC LETTER TAH*/
		windows_1256_chars[0xD9] = (char) (0x0638); /* ARABIC LETTER ZAH*/
		windows_1256_chars[0xDA] = (char) (0x0639); /* ARABIC LETTER AIN*/
		windows_1256_chars[0xDB] = (char) (0x063A); /* ARABIC LETTER GHAIN*/
		windows_1256_chars[0xDC] = (char) (0x0640); /* ARABIC TATWEEL*/
		windows_1256_chars[0xDD] = (char) (0x0641); /* ARABIC LETTER FEH*/
		windows_1256_chars[0xDE] = (char) (0x0642); /* ARABIC LETTER QAF*/
		windows_1256_chars[0xDF] = (char) (0x0643); /* ARABIC LETTER KAF*/
		windows_1256_chars[0xE0] = (char) (0x00E0); /* LATIN SMALL LETTER A WITH GRAVE*/
		windows_1256_chars[0xE1] = (char) (0x0644); /* ARABIC LETTER LAM*/
		windows_1256_chars[0xE2] = (char) (0x00E2); /* LATIN SMALL LETTER A WITH CIRCUMFLEX*/
		windows_1256_chars[0xE3] = (char) (0x0645); /* ARABIC LETTER MEEM*/
		windows_1256_chars[0xE4] = (char) (0x0646); /* ARABIC LETTER NOON*/
		windows_1256_chars[0xE5] = (char) (0x0647); /* ARABIC LETTER HEH*/
		windows_1256_chars[0xE6] = (char) (0x0648); /* ARABIC LETTER WAW*/
		windows_1256_chars[0xE7] = (char) (0x00E7); /* LATIN SMALL LETTER C WITH CEDILLA*/
		windows_1256_chars[0xE8] = (char) (0x00E8); /* LATIN SMALL LETTER E WITH GRAVE*/
		windows_1256_chars[0xE9] = (char) (0x00E9); /* LATIN SMALL LETTER E WITH ACUTE*/
		windows_1256_chars[0xEA] = (char) (0x00EA); /* LATIN SMALL LETTER E WITH CIRCUMFLEX*/
		windows_1256_chars[0xEB] = (char) (0x00EB); /* LATIN SMALL LETTER E WITH DIAERESIS*/
		windows_1256_chars[0xEC] = (char) (0x0649); /* ARABIC LETTER ALEF MAKSURA*/
		windows_1256_chars[0xED] = (char) (0x064A); /* ARABIC LETTER YEH*/
		windows_1256_chars[0xEE] = (char) (0x00EE); /* LATIN SMALL LETTER I WITH CIRCUMFLEX*/
		windows_1256_chars[0xEF] = (char) (0x00EF); /* LATIN SMALL LETTER I WITH DIAERESIS*/
		windows_1256_chars[0xF0] = (char) (0x064B); /* ARABIC FATHATAN*/
		windows_1256_chars[0xF1] = (char) (0x064C); /* ARABIC DAMMATAN*/
		windows_1256_chars[0xF2] = (char) (0x064D); /* ARABIC KASRATAN*/
		windows_1256_chars[0xF3] = (char) (0x064E); /* ARABIC FATHA*/
		windows_1256_chars[0xF4] = (char) (0x00F4); /* LATIN SMALL LETTER O WITH CIRCUMFLEX*/
		windows_1256_chars[0xF5] = (char) (0x064F); /* ARABIC DAMMA*/
		windows_1256_chars[0xF6] = (char) (0x0650); /* ARABIC KASRA*/
		windows_1256_chars[0xF7] = (char) (0x00F7); /* DIVISION SIGN*/
		windows_1256_chars[0xF8] = (char) (0x0651); /* ARABIC SHADDA*/
		windows_1256_chars[0xF9] = (char) (0x00F9); /* LATIN SMALL LETTER U WITH GRAVE*/
		windows_1256_chars[0xFA] = (char) (0x0652); /* ARABIC SUKUN*/
		windows_1256_chars[0xFB] = (char) (0x00FB); /* LATIN SMALL LETTER U WITH CIRCUMFLEX*/
		windows_1256_chars[0xFC] = (char) (0x00FC); /* LATIN SMALL LETTER U WITH DIAERESIS*/
		windows_1256_chars[0xFD] = (char) (0x200E); /* LEFT-TO-RIGHT MARK*/
		windows_1256_chars[0xFE] = (char) (0x200F); /* RIGHT-TO-LEFT MARK*/
		windows_1256_chars[0xFF] = (char) (0x06D2); /* ARABIC LETTER YEH BARREE*/
		windows_1256_chars_ready = TRUE;
	}
}
void windows_1257_chars_init(){
	int i;
	if (windows_1257_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			windows_1257_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			windows_1257_chars[i] = (char) (0xfffd);
		}
		windows_1257_chars[0x80] = (char) (0x20AC); /* EURO SIGN*/
		windows_1257_chars[0x82] = (char) (0x201A); /* SINGLE LOW-9 QUOTATION MARK*/
		windows_1257_chars[0x84] = (char) (0x201E); /* DOUBLE LOW-9 QUOTATION MARK*/
		windows_1257_chars[0x85] = (char) (0x2026); /* HORIZONTAL ELLIPSIS*/
		windows_1257_chars[0x86] = (char) (0x2020); /* DAGGER*/
		windows_1257_chars[0x87] = (char) (0x2021); /* DOUBLE DAGGER*/
		windows_1257_chars[0x89] = (char) (0x2030); /* PER MILLE SIGN*/
		windows_1257_chars[0x8B] = (char) (0x2039); /* SINGLE LEFT-POINTING ANGLE QUOTATION MARK*/
		windows_1257_chars[0x8D] = (char) (0x00A8); /* DIAERESIS*/
		windows_1257_chars[0x8E] = (char) (0x02C7); /* CARON*/
		windows_1257_chars[0x8F] = (char) (0x00B8); /* CEDILLA*/
		windows_1257_chars[0x91] = (char) (0x2018); /* LEFT SINGLE QUOTATION MARK*/
		windows_1257_chars[0x92] = (char) (0x2019); /* RIGHT SINGLE QUOTATION MARK*/
		windows_1257_chars[0x93] = (char) (0x201C); /* LEFT DOUBLE QUOTATION MARK*/
		windows_1257_chars[0x94] = (char) (0x201D); /* RIGHT DOUBLE QUOTATION MARK*/
		windows_1257_chars[0x95] = (char) (0x2022); /* BULLET*/
		windows_1257_chars[0x96] = (char) (0x2013); /* EN DASH*/
		windows_1257_chars[0x97] = (char) (0x2014); /* EM DASH*/
		windows_1257_chars[0x99] = (char) (0x2122); /* TRADE MARK SIGN*/
		windows_1257_chars[0x9B] = (char) (0x203A); /* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK*/
		windows_1257_chars[0x9D] = (char) (0x00AF); /* MACRON*/
		windows_1257_chars[0x9E] = (char) (0x02DB); /* OGONEK*/
		windows_1257_chars[0xA0] = (char) (0x00A0); /* NO-BREAK SPACE*/
		windows_1257_chars[0xA2] = (char) (0x00A2); /* CENT SIGN*/
		windows_1257_chars[0xA3] = (char) (0x00A3); /* POUND SIGN*/
		windows_1257_chars[0xA4] = (char) (0x00A4); /* CURRENCY SIGN*/
		windows_1257_chars[0xA6] = (char) (0x00A6); /* BROKEN BAR*/
		windows_1257_chars[0xA7] = (char) (0x00A7); /* SECTION SIGN*/
		windows_1257_chars[0xA8] = (char) (0x00D8); /* LATIN CAPITAL LETTER O WITH STROKE*/
		windows_1257_chars[0xA9] = (char) (0x00A9); /* COPYRIGHT SIGN*/
		windows_1257_chars[0xAA] = (char) (0x0156); /* LATIN CAPITAL LETTER R WITH CEDILLA*/
		windows_1257_chars[0xAB] = (char) (0x00AB); /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1257_chars[0xAC] = (char) (0x00AC); /* NOT SIGN*/
		windows_1257_chars[0xAD] = (char) (0x00AD); /* SOFT HYPHEN*/
		windows_1257_chars[0xAE] = (char) (0x00AE); /* REGISTERED SIGN*/
		windows_1257_chars[0xAF] = (char) (0x00C6); /* LATIN CAPITAL LETTER AE*/
		windows_1257_chars[0xB0] = (char) (0x00B0); /* DEGREE SIGN*/
		windows_1257_chars[0xB1] = (char) (0x00B1); /* PLUS-MINUS SIGN*/
		windows_1257_chars[0xB2] = (char) (0x00B2); /* SUPERSCRIPT TWO*/
		windows_1257_chars[0xB3] = (char) (0x00B3); /* SUPERSCRIPT THREE*/
		windows_1257_chars[0xB4] = (char) (0x00B4); /* ACUTE ACCENT*/
		windows_1257_chars[0xB5] = (char) (0x00B5); /* MICRO SIGN*/
		windows_1257_chars[0xB6] = (char) (0x00B6); /* PILCROW SIGN*/
		windows_1257_chars[0xB7] = (char) (0x00B7); /* MIDDLE DOT*/
		windows_1257_chars[0xB8] = (char) (0x00F8); /* LATIN SMALL LETTER O WITH STROKE*/
		windows_1257_chars[0xB9] = (char) (0x00B9); /* SUPERSCRIPT ONE*/
		windows_1257_chars[0xBA] = (char) (0x0157); /* LATIN SMALL LETTER R WITH CEDILLA*/
		windows_1257_chars[0xBB] = (char) (0x00BB); /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1257_chars[0xBC] = (char) (0x00BC); /* VULGAR FRACTION ONE QUARTER*/
		windows_1257_chars[0xBD] = (char) (0x00BD); /* VULGAR FRACTION ONE HALF*/
		windows_1257_chars[0xBE] = (char) (0x00BE); /* VULGAR FRACTION THREE QUARTERS*/
		windows_1257_chars[0xBF] = (char) (0x00E6); /* LATIN SMALL LETTER AE*/
		windows_1257_chars[0xC0] = (char) (0x0104); /* LATIN CAPITAL LETTER A WITH OGONEK*/
		windows_1257_chars[0xC1] = (char) (0x012E); /* LATIN CAPITAL LETTER I WITH OGONEK*/
		windows_1257_chars[0xC2] = (char) (0x0100); /* LATIN CAPITAL LETTER A WITH MACRON*/
		windows_1257_chars[0xC3] = (char) (0x0106); /* LATIN CAPITAL LETTER C WITH ACUTE*/
		windows_1257_chars[0xC4] = (char) (0x00C4); /* LATIN CAPITAL LETTER A WITH DIAERESIS*/
		windows_1257_chars[0xC5] = (char) (0x00C5); /* LATIN CAPITAL LETTER A WITH RING ABOVE*/
		windows_1257_chars[0xC6] = (char) (0x0118); /* LATIN CAPITAL LETTER E WITH OGONEK*/
		windows_1257_chars[0xC7] = (char) (0x0112); /* LATIN CAPITAL LETTER E WITH MACRON*/
		windows_1257_chars[0xC8] = (char) (0x010C); /* LATIN CAPITAL LETTER C WITH CARON*/
		windows_1257_chars[0xC9] = (char) (0x00C9); /* LATIN CAPITAL LETTER E WITH ACUTE*/
		windows_1257_chars[0xCA] = (char) (0x0179); /* LATIN CAPITAL LETTER Z WITH ACUTE*/
		windows_1257_chars[0xCB] = (char) (0x0116); /* LATIN CAPITAL LETTER E WITH DOT ABOVE*/
		windows_1257_chars[0xCC] = (char) (0x0122); /* LATIN CAPITAL LETTER G WITH CEDILLA*/
		windows_1257_chars[0xCD] = (char) (0x0136); /* LATIN CAPITAL LETTER K WITH CEDILLA*/
		windows_1257_chars[0xCE] = (char) (0x012A); /* LATIN CAPITAL LETTER I WITH MACRON*/
		windows_1257_chars[0xCF] = (char) (0x013B); /* LATIN CAPITAL LETTER L WITH CEDILLA*/
		windows_1257_chars[0xD0] = (char) (0x0160); /* LATIN CAPITAL LETTER S WITH CARON*/
		windows_1257_chars[0xD1] = (char) (0x0143); /* LATIN CAPITAL LETTER N WITH ACUTE*/
		windows_1257_chars[0xD2] = (char) (0x0145); /* LATIN CAPITAL LETTER N WITH CEDILLA*/
		windows_1257_chars[0xD3] = (char) (0x00D3); /* LATIN CAPITAL LETTER O WITH ACUTE*/
		windows_1257_chars[0xD4] = (char) (0x014C); /* LATIN CAPITAL LETTER O WITH MACRON*/
		windows_1257_chars[0xD5] = (char) (0x00D5); /* LATIN CAPITAL LETTER O WITH TILDE*/
		windows_1257_chars[0xD6] = (char) (0x00D6); /* LATIN CAPITAL LETTER O WITH DIAERESIS*/
		windows_1257_chars[0xD7] = (char) (0x00D7); /* MULTIPLICATION SIGN*/
		windows_1257_chars[0xD8] = (char) (0x0172); /* LATIN CAPITAL LETTER U WITH OGONEK*/
		windows_1257_chars[0xD9] = (char) (0x0141); /* LATIN CAPITAL LETTER L WITH STROKE*/
		windows_1257_chars[0xDA] = (char) (0x015A); /* LATIN CAPITAL LETTER S WITH ACUTE*/
		windows_1257_chars[0xDB] = (char) (0x016A); /* LATIN CAPITAL LETTER U WITH MACRON*/
		windows_1257_chars[0xDC] = (char) (0x00DC); /* LATIN CAPITAL LETTER U WITH DIAERESIS*/
		windows_1257_chars[0xDD] = (char) (0x017B); /* LATIN CAPITAL LETTER Z WITH DOT ABOVE*/
		windows_1257_chars[0xDE] = (char) (0x017D); /* LATIN CAPITAL LETTER Z WITH CARON*/
		windows_1257_chars[0xDF] = (char) (0x00DF); /* LATIN SMALL LETTER SHARP S*/
		windows_1257_chars[0xE0] = (char) (0x0105); /* LATIN SMALL LETTER A WITH OGONEK*/
		windows_1257_chars[0xE1] = (char) (0x012F); /* LATIN SMALL LETTER I WITH OGONEK*/
		windows_1257_chars[0xE2] = (char) (0x0101); /* LATIN SMALL LETTER A WITH MACRON*/
		windows_1257_chars[0xE3] = (char) (0x0107); /* LATIN SMALL LETTER C WITH ACUTE*/
		windows_1257_chars[0xE4] = (char) (0x00E4); /* LATIN SMALL LETTER A WITH DIAERESIS*/
		windows_1257_chars[0xE5] = (char) (0x00E5); /* LATIN SMALL LETTER A WITH RING ABOVE*/
		windows_1257_chars[0xE6] = (char) (0x0119); /* LATIN SMALL LETTER E WITH OGONEK*/
		windows_1257_chars[0xE7] = (char) (0x0113); /* LATIN SMALL LETTER E WITH MACRON*/
		windows_1257_chars[0xE8] = (char) (0x010D); /* LATIN SMALL LETTER C WITH CARON*/
		windows_1257_chars[0xE9] = (char) (0x00E9); /* LATIN SMALL LETTER E WITH ACUTE*/
		windows_1257_chars[0xEA] = (char) (0x017A); /* LATIN SMALL LETTER Z WITH ACUTE*/
		windows_1257_chars[0xEB] = (char) (0x0117); /* LATIN SMALL LETTER E WITH DOT ABOVE*/
		windows_1257_chars[0xEC] = (char) (0x0123); /* LATIN SMALL LETTER G WITH CEDILLA*/
		windows_1257_chars[0xED] = (char) (0x0137); /* LATIN SMALL LETTER K WITH CEDILLA*/
		windows_1257_chars[0xEE] = (char) (0x012B); /* LATIN SMALL LETTER I WITH MACRON*/
		windows_1257_chars[0xEF] = (char) (0x013C); /* LATIN SMALL LETTER L WITH CEDILLA*/
		windows_1257_chars[0xF0] = (char) (0x0161); /* LATIN SMALL LETTER S WITH CARON*/
		windows_1257_chars[0xF1] = (char) (0x0144); /* LATIN SMALL LETTER N WITH ACUTE*/
		windows_1257_chars[0xF2] = (char) (0x0146); /* LATIN SMALL LETTER N WITH CEDILLA*/
		windows_1257_chars[0xF3] = (char) (0x00F3); /* LATIN SMALL LETTER O WITH ACUTE*/
		windows_1257_chars[0xF4] = (char) (0x014D); /* LATIN SMALL LETTER O WITH MACRON*/
		windows_1257_chars[0xF5] = (char) (0x00F5); /* LATIN SMALL LETTER O WITH TILDE*/
		windows_1257_chars[0xF6] = (char) (0x00F6); /* LATIN SMALL LETTER O WITH DIAERESIS*/
		windows_1257_chars[0xF7] = (char) (0x00F7); /* DIVISION SIGN*/
		windows_1257_chars[0xF8] = (char) (0x0173); /* LATIN SMALL LETTER U WITH OGONEK*/
		windows_1257_chars[0xF9] = (char) (0x0142); /* LATIN SMALL LETTER L WITH STROKE*/
		windows_1257_chars[0xFA] = (char) (0x015B); /* LATIN SMALL LETTER S WITH ACUTE*/
		windows_1257_chars[0xFB] = (char) (0x016B); /* LATIN SMALL LETTER U WITH MACRON*/
		windows_1257_chars[0xFC] = (char) (0x00FC); /* LATIN SMALL LETTER U WITH DIAERESIS*/
		windows_1257_chars[0xFD] = (char) (0x017C); /* LATIN SMALL LETTER Z WITH DOT ABOVE*/
		windows_1257_chars[0xFE] = (char) (0x017E); /* LATIN SMALL LETTER Z WITH CARON*/
		windows_1257_chars[0xFF] = (char) (0x02D9); /* DOT ABOVE*/

		windows_1257_chars_ready = TRUE;
	}
}
void windows_1258_chars_init(){
	int i;
	if (windows_1258_chars_ready)
		return;
	else{
		for (i = 0; i < 128; i++)
		{
			windows_1258_chars[i] = (char) i;
		}
		for (i = 128; i < 256; i++)
		{
			windows_1258_chars[i] = (char) (0xfffd);
		}
		windows_1258_chars[0x80] = (char) (0x20AC); /* EURO SIGN*/
		windows_1258_chars[0x82] = (char) (0x201A); /* SINGLE LOW-9 QUOTATION MARK*/
		windows_1258_chars[0x83] = (char) (0x0192); /* LATIN SMALL LETTER F WITH HOOK*/
		windows_1258_chars[0x84] = (char) (0x201E); /* DOUBLE LOW-9 QUOTATION MARK*/
		windows_1258_chars[0x85] = (char) (0x2026); /* HORIZONTAL ELLIPSIS*/
		windows_1258_chars[0x86] = (char) (0x2020); /* DAGGER*/
		windows_1258_chars[0x87] = (char) (0x2021); /* DOUBLE DAGGER*/
		windows_1258_chars[0x88] = (char) (0x02C6); /* MODIFIER LETTER CIRCUMFLEX ACCENT*/
		windows_1258_chars[0x89] = (char) (0x2030); /* PER MILLE SIGN*/
		windows_1258_chars[0x8B] = (char) (0x2039); /* SINGLE LEFT-POINTING ANGLE QUOTATION MARK*/
		windows_1258_chars[0x8C] = (char) (0x0152); /* LATIN CAPITAL LIGATURE OE*/
		windows_1258_chars[0x91] = (char) (0x2018); /* LEFT SINGLE QUOTATION MARK*/
		windows_1258_chars[0x92] = (char) (0x2019); /* RIGHT SINGLE QUOTATION MARK*/
		windows_1258_chars[0x93] = (char) (0x201C); /* LEFT DOUBLE QUOTATION MARK*/
		windows_1258_chars[0x94] = (char) (0x201D); /* RIGHT DOUBLE QUOTATION MARK*/
		windows_1258_chars[0x95] = (char) (0x2022); /* BULLET*/
		windows_1258_chars[0x96] = (char) (0x2013); /* EN DASH*/
		windows_1258_chars[0x97] = (char) (0x2014); /* EM DASH*/
		windows_1258_chars[0x98] = (char) (0x02DC); /* SMALL TILDE*/
		windows_1258_chars[0x99] = (char) (0x2122); /* TRADE MARK SIGN*/
		windows_1258_chars[0x9B] = (char) (0x203A); /* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK*/
		windows_1258_chars[0x9C] = (char) (0x0153); /* LATIN SMALL LIGATURE OE*/
		windows_1258_chars[0x9F] = (char) (0x0178); /* LATIN CAPITAL LETTER Y WITH DIAERESIS*/
		windows_1258_chars[0xA0] = (char) (0x00A0); /* NO-BREAK SPACE*/
		windows_1258_chars[0xA1] = (char) (0x00A1); /* INVERTED EXCLAMATION MARK*/
		windows_1258_chars[0xA2] = (char) (0x00A2); /* CENT SIGN*/
		windows_1258_chars[0xA3] = (char) (0x00A3); /* POUND SIGN*/
		windows_1258_chars[0xA4] = (char) (0x00A4); /* CURRENCY SIGN*/
		windows_1258_chars[0xA5] = (char) (0x00A5); /* YEN SIGN*/
		windows_1258_chars[0xA6] = (char) (0x00A6); /* BROKEN BAR*/
		windows_1258_chars[0xA7] = (char) (0x00A7); /* SECTION SIGN*/
		windows_1258_chars[0xA8] = (char) (0x00A8); /* DIAERESIS*/
		windows_1258_chars[0xA9] = (char) (0x00A9); /* COPYRIGHT SIGN*/
		windows_1258_chars[0xAA] = (char) (0x00AA); /* FEMININE ORDINAL INDICATOR*/
		windows_1258_chars[0xAB] = (char) (0x00AB); /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1258_chars[0xAC] = (char) (0x00AC); /* NOT SIGN*/
		windows_1258_chars[0xAD] = (char) (0x00AD); /* SOFT HYPHEN*/
		windows_1258_chars[0xAE] = (char) (0x00AE); /* REGISTERED SIGN*/
		windows_1258_chars[0xAF] = (char) (0x00AF); /* MACRON*/
		windows_1258_chars[0xB0] = (char) (0x00B0); /* DEGREE SIGN*/
		windows_1258_chars[0xB1] = (char) (0x00B1); /* PLUS-MINUS SIGN*/
		windows_1258_chars[0xB2] = (char) (0x00B2); /* SUPERSCRIPT TWO*/
		windows_1258_chars[0xB3] = (char) (0x00B3); /* SUPERSCRIPT THREE*/
		windows_1258_chars[0xB4] = (char) (0x00B4); /* ACUTE ACCENT*/
		windows_1258_chars[0xB5] = (char) (0x00B5); /* MICRO SIGN*/
		windows_1258_chars[0xB6] = (char) (0x00B6); /* PILCROW SIGN*/
		windows_1258_chars[0xB7] = (char) (0x00B7); /* MIDDLE DOT*/
		windows_1258_chars[0xB8] = (char) (0x00B8); /* CEDILLA*/
		windows_1258_chars[0xB9] = (char) (0x00B9); /* SUPERSCRIPT ONE*/
		windows_1258_chars[0xBA] = (char) (0x00BA); /* MASCULINE ORDINAL INDICATOR*/
		windows_1258_chars[0xBB] = (char) (0x00BB); /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK*/
		windows_1258_chars[0xBC] = (char) (0x00BC); /* VULGAR FRACTION ONE QUARTER*/
		windows_1258_chars[0xBD] = (char) (0x00BD); /* VULGAR FRACTION ONE HALF*/
		windows_1258_chars[0xBE] = (char) (0x00BE); /* VULGAR FRACTION THREE QUARTERS*/
		windows_1258_chars[0xBF] = (char) (0x00BF); /* INVERTED QUESTION MARK*/
		windows_1258_chars[0xC0] = (char) (0x00C0); /* LATIN CAPITAL LETTER A WITH GRAVE*/
		windows_1258_chars[0xC1] = (char) (0x00C1); /* LATIN CAPITAL LETTER A WITH ACUTE*/
		windows_1258_chars[0xC2] = (char) (0x00C2); /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX*/
		windows_1258_chars[0xC3] = (char) (0x0102); /* LATIN CAPITAL LETTER A WITH BREVE*/
		windows_1258_chars[0xC4] = (char) (0x00C4); /* LATIN CAPITAL LETTER A WITH DIAERESIS*/
		windows_1258_chars[0xC5] = (char) (0x00C5); /* LATIN CAPITAL LETTER A WITH RING ABOVE*/
		windows_1258_chars[0xC6] = (char) (0x00C6); /* LATIN CAPITAL LETTER AE*/
		windows_1258_chars[0xC7] = (char) (0x00C7); /* LATIN CAPITAL LETTER C WITH CEDILLA*/
		windows_1258_chars[0xC8] = (char) (0x00C8); /* LATIN CAPITAL LETTER E WITH GRAVE*/
		windows_1258_chars[0xC9] = (char) (0x00C9); /* LATIN CAPITAL LETTER E WITH ACUTE*/
		windows_1258_chars[0xCA] = (char) (0x00CA); /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX*/
		windows_1258_chars[0xCB] = (char) (0x00CB); /* LATIN CAPITAL LETTER E WITH DIAERESIS*/
		windows_1258_chars[0xCC] = (char) (0x0300); /* COMBINING GRAVE ACCENT*/
		windows_1258_chars[0xCD] = (char) (0x00CD); /* LATIN CAPITAL LETTER I WITH ACUTE*/
		windows_1258_chars[0xCE] = (char) (0x00CE); /* LATIN CAPITAL LETTER I WITH CIRCUMFLEX*/
		windows_1258_chars[0xCF] = (char) (0x00CF); /* LATIN CAPITAL LETTER I WITH DIAERESIS*/
		windows_1258_chars[0xD0] = (char) (0x0110); /* LATIN CAPITAL LETTER D WITH STROKE*/
		windows_1258_chars[0xD1] = (char) (0x00D1); /* LATIN CAPITAL LETTER N WITH TILDE*/
		windows_1258_chars[0xD2] = (char) (0x0309); /* COMBINING HOOK ABOVE*/
		windows_1258_chars[0xD3] = (char) (0x00D3); /* LATIN CAPITAL LETTER O WITH ACUTE*/
		windows_1258_chars[0xD4] = (char) (0x00D4); /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX*/
		windows_1258_chars[0xD5] = (char) (0x01A0); /* LATIN CAPITAL LETTER O WITH HORN*/
		windows_1258_chars[0xD6] = (char) (0x00D6); /* LATIN CAPITAL LETTER O WITH DIAERESIS*/
		windows_1258_chars[0xD7] = (char) (0x00D7); /* MULTIPLICATION SIGN*/
		windows_1258_chars[0xD8] = (char) (0x00D8); /* LATIN CAPITAL LETTER O WITH STROKE*/
		windows_1258_chars[0xD9] = (char) (0x00D9); /* LATIN CAPITAL LETTER U WITH GRAVE*/
		windows_1258_chars[0xDA] = (char) (0x00DA); /* LATIN CAPITAL LETTER U WITH ACUTE*/
		windows_1258_chars[0xDB] = (char) (0x00DB); /* LATIN CAPITAL LETTER U WITH CIRCUMFLEX*/
		windows_1258_chars[0xDC] = (char) (0x00DC); /* LATIN CAPITAL LETTER U WITH DIAERESIS*/
		windows_1258_chars[0xDD] = (char) (0x01AF); /* LATIN CAPITAL LETTER U WITH HORN*/
		windows_1258_chars[0xDE] = (char) (0x0303); /* COMBINING TILDE*/
		windows_1258_chars[0xDF] = (char) (0x00DF); /* LATIN SMALL LETTER SHARP S*/
		windows_1258_chars[0xE0] = (char) (0x00E0); /* LATIN SMALL LETTER A WITH GRAVE*/
		windows_1258_chars[0xE1] = (char) (0x00E1); /* LATIN SMALL LETTER A WITH ACUTE*/
		windows_1258_chars[0xE2] = (char) (0x00E2); /* LATIN SMALL LETTER A WITH CIRCUMFLEX*/
		windows_1258_chars[0xE3] = (char) (0x0103); /* LATIN SMALL LETTER A WITH BREVE*/
		windows_1258_chars[0xE4] = (char) (0x00E4); /* LATIN SMALL LETTER A WITH DIAERESIS*/
		windows_1258_chars[0xE5] = (char) (0x00E5); /* LATIN SMALL LETTER A WITH RING ABOVE*/
		windows_1258_chars[0xE6] = (char) (0x00E6); /* LATIN SMALL LETTER AE*/
		windows_1258_chars[0xE7] = (char) (0x00E7); /* LATIN SMALL LETTER C WITH CEDILLA*/
		windows_1258_chars[0xE8] = (char) (0x00E8); /* LATIN SMALL LETTER E WITH GRAVE*/
		windows_1258_chars[0xE9] = (char) (0x00E9); /* LATIN SMALL LETTER E WITH ACUTE*/
		windows_1258_chars[0xEA] = (char) (0x00EA); /* LATIN SMALL LETTER E WITH CIRCUMFLEX*/
		windows_1258_chars[0xEB] = (char) (0x00EB); /* LATIN SMALL LETTER E WITH DIAERESIS*/
		windows_1258_chars[0xEC] = (char) (0x0301); /* COMBINING ACUTE ACCENT*/
		windows_1258_chars[0xED] = (char) (0x00ED); /* LATIN SMALL LETTER I WITH ACUTE*/
		windows_1258_chars[0xEE] = (char) (0x00EE); /* LATIN SMALL LETTER I WITH CIRCUMFLEX*/
		windows_1258_chars[0xEF] = (char) (0x00EF); /* LATIN SMALL LETTER I WITH DIAERESIS*/
		windows_1258_chars[0xF0] = (char) (0x0111); /* LATIN SMALL LETTER D WITH STROKE*/
		windows_1258_chars[0xF1] = (char) (0x00F1); /* LATIN SMALL LETTER N WITH TILDE*/
		windows_1258_chars[0xF2] = (char) (0x0323); /* COMBINING DOT BELOW*/
		windows_1258_chars[0xF3] = (char) (0x00F3); /* LATIN SMALL LETTER O WITH ACUTE*/
		windows_1258_chars[0xF4] = (char) (0x00F4); /* LATIN SMALL LETTER O WITH CIRCUMFLEX*/
		windows_1258_chars[0xF5] = (char) (0x01A1); /* LATIN SMALL LETTER O WITH HORN*/
		windows_1258_chars[0xF6] = (char) (0x00F6); /* LATIN SMALL LETTER O WITH DIAERESIS*/
		windows_1258_chars[0xF7] = (char) (0x00F7); /* DIVISION SIGN*/
		windows_1258_chars[0xF8] = (char) (0x00F8); /* LATIN SMALL LETTER O WITH STROKE*/
		windows_1258_chars[0xF9] = (char) (0x00F9); /* LATIN SMALL LETTER U WITH GRAVE*/
		windows_1258_chars[0xFA] = (char) (0x00FA); /* LATIN SMALL LETTER U WITH ACUTE*/
		windows_1258_chars[0xFB] = (char) (0x00FB); /* LATIN SMALL LETTER U WITH CIRCUMFLEX*/
		windows_1258_chars[0xFC] = (char) (0x00FC); /* LATIN SMALL LETTER U WITH DIAERESIS*/
		windows_1258_chars[0xFD] = (char) (0x01B0); /* LATIN SMALL LETTER U WITH HORN*/
		windows_1258_chars[0xFE] = (char) (0x20AB); /* DONG SIGN*/
		windows_1258_chars[0xFF] = (char) (0x00FF); /* LATIN SMALL LETTER Y WITH DIAERESIS*/

		windows_1258_chars_ready = TRUE;
	}
}
