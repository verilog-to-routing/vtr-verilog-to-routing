// -------------------------------------------------------------------------
//Reed-Solomon decoder
//Copyright (C) Wed Apr 28 11:28:40 2004
//by Ming-Han Lei(hendrik@humanistic.org)
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
// --------------------------------------------------------------------------

module rs_decoder_2(x, error, with_error, enable, valid, k, clk, clrn);
	input enable, clk, clrn;
	input [8:0] k, x;
	output [8:0] error;
	wire [8:0] error;
	output with_error, valid;
	reg valid;
	wire with_error;

	wire [8:0] s0, s1, s2, s3, s4, s5, s6, s7;
	wire [8:0] lambda, omega, alpha;
	reg [3:0] count;
	reg [8:0] phase;
	wire [8:0] D0, D1, DI;
//	reg [8:0] D, D2;
	wire [8:0] D, D2;
	reg [8:0] u, length0, length2;
	wire [8:0] length1, length3;
	reg syn_enable, syn_init, syn_shift, berl_enable;
	reg chien_search, chien_load, shorten;

	always @ (chien_search or shorten)
		valid = chien_search & ~shorten;

	wire bit1;
	assign bit1 = syn_shift&phase[0];
	rsdec_syn x0 (s0, s1, s2, s3, s4, s5, s6, s7, u, syn_enable, bit1, syn_init, clk, clrn);
	rsdec_berl x1 (lambda, omega, s0, s7, s6, s5, s4, s3, s2, s1, D0, D2, count, phase[0], phase[8], berl_enable, clk, clrn);
	rsdec_chien x2 (error, alpha, lambda, omega, D1, DI, chien_search, chien_load, shorten, clk, clrn);
	inverse x3 (DI, D);

	always @ (posedge clk)// or negedge clrn)
	begin
		if (~clrn)
		begin
			syn_enable <= 0;
			syn_shift <= 0;
			berl_enable <= 0;
			chien_search <= 1;
			chien_load <= 0;
			length0 <= 0;
			length2 <= 511 - k;
			count <= -1;
			phase <= 1;
			u <= 0;
			shorten <= 1;
			syn_init <= 0;
		end
		else
		begin
			if (enable & ~syn_enable & ~syn_shift)
			begin
				syn_enable <= 1;
				syn_init <= 1;
			end
			else if (syn_enable)
			begin
				length0 <= length1;
				syn_init <= 0;
				if (length1 == k)
				begin
					syn_enable <= 0;
					syn_shift <= 1;
					berl_enable <= 1;
				end
			end
			else if (berl_enable & with_error)
			begin
				if (phase[0])
				begin
					count <= count + 1;
					if (count == 7)
					begin
						syn_shift <= 0;
						length0 <= 0;
						chien_load <= 1;
						length2 <= length0;
					end
				end
				phase <= {phase[7:0], phase[8]};
			end
			else if (berl_enable & ~with_error)
				if (&count)
				begin
					syn_shift <= 0;
					length0 <= 0;
					berl_enable <= 0;
				end
				else
					phase <= {phase[7:0], phase[8]};
			else if (chien_load & phase[8])
			begin
				berl_enable <= 0;
				chien_load <= 0;
				chien_search <= 1;
				count <= -1;
				phase <= 1;
			end
			else if (chien_search)// | shorten)
			begin
				length2 <= length3;
				if (length3 == 0)
					chien_search <= 0;
			end
			else if (enable) u <= x;
			else if (shorten == 1 && length2 == 0)
				shorten <= 0;
		end
	end

//	always @ (chien_search or D0 or D1)
//		if (chien_search) D = D1;
//		else D = D0;
	assign D = chien_search ? D1 : D0;

//	always @ (DI or alpha or chien_load)
//		if (chien_load) D2 = alpha;
//		else D2 = DI;
	assign D2 = chien_load ? alpha : DI;

	assign length1 = length0 + 1;
	assign length3 = length2 - 1;
//	always @ (syn_shift or s0 or s1 or s2 or s3 or s4 or s5 or s6 or s7 or s8 or s9 or s10 or s11)
//		if (syn_shift && (s0 | s1 | s2 | s3 | s4 | s5 | s6 | s7 | s8 | s9 | s10 | s11)!= 0)
//			with_error = 1;
//		else with_error = 0;
wire temp;
	assign temp = syn_shift && (s0 | s1 | s2 | s3 | s4 | s5 | s6 | s7 );
	assign with_error = temp != 0 ? 1'b1 : 1'b0;

endmodule

// -------------------------------------------------------------------------
//The inverse lookup table for Galois field
//Copyright (C) Wed Apr 28 11:17:28 2004
//by Ming-Han Lei(hendrik@humanistic.org)
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
// --------------------------------------------------------------------------

module inverse(y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;

	always @ (x)
	case (x) // synopsys full_case parallel_case
		1: y = 1; // 0 -> 511
		2: y = 264; // 1 -> 510
		4: y = 132; // 2 -> 509
		8: y = 66; // 3 -> 508
		16: y = 33; // 4 -> 507
		32: y = 280; // 5 -> 506
		64: y = 140; // 6 -> 505
		128: y = 70; // 7 -> 504
		256: y = 35; // 8 -> 503
		17: y = 281; // 9 -> 502
		34: y = 388; // 10 -> 501
		68: y = 194; // 11 -> 500
		136: y = 97; // 12 -> 499
		272: y = 312; // 13 -> 498
		49: y = 156; // 14 -> 497
		98: y = 78; // 15 -> 496
		196: y = 39; // 16 -> 495
		392: y = 283; // 17 -> 494
		257: y = 389; // 18 -> 493
		19: y = 458; // 19 -> 492
		38: y = 229; // 20 -> 491
		76: y = 378; // 21 -> 490
		152: y = 189; // 22 -> 489
		304: y = 342; // 23 -> 488
		113: y = 171; // 24 -> 487
		226: y = 349; // 25 -> 486
		452: y = 422; // 26 -> 485
		409: y = 211; // 27 -> 484
		291: y = 353; // 28 -> 483
		87: y = 440; // 29 -> 482
		174: y = 220; // 30 -> 481
		348: y = 110; // 31 -> 480
		169: y = 55; // 32 -> 479
		338: y = 275; // 33 -> 478
		181: y = 385; // 34 -> 477
		362: y = 456; // 35 -> 476
		197: y = 228; // 36 -> 475
		394: y = 114; // 37 -> 474
		261: y = 57; // 38 -> 473
		27: y = 276; // 39 -> 472
		54: y = 138; // 40 -> 471
		108: y = 69; // 41 -> 470
		216: y = 298; // 42 -> 469
		432: y = 149; // 43 -> 468
		369: y = 322; // 44 -> 467
		243: y = 161; // 45 -> 466
		486: y = 344; // 46 -> 465
		477: y = 172; // 47 -> 464
		427: y = 86; // 48 -> 463
		327: y = 43; // 49 -> 462
		159: y = 285; // 50 -> 461
		318: y = 390; // 51 -> 460
		109: y = 195; // 52 -> 459
		218: y = 361; // 53 -> 458
		436: y = 444; // 54 -> 457
		377: y = 222; // 55 -> 456
		227: y = 111; // 56 -> 455
		454: y = 319; // 57 -> 454
		413: y = 407; // 58 -> 453
		299: y = 451; // 59 -> 452
		71: y = 489; // 60 -> 451
		142: y = 508; // 61 -> 450
		284: y = 254; // 62 -> 449
		41: y = 127; // 63 -> 448
		82: y = 311; // 64 -> 447
		164: y = 403; // 65 -> 446
		328: y = 449; // 66 -> 445
		129: y = 488; // 67 -> 444
		258: y = 244; // 68 -> 443
		21: y = 122; // 69 -> 442
		42: y = 61; // 70 -> 441
		84: y = 278; // 71 -> 440
		168: y = 139; // 72 -> 439
		336: y = 333; // 73 -> 438
		177: y = 430; // 74 -> 437
		354: y = 215; // 75 -> 436
		213: y = 355; // 76 -> 435
		426: y = 441; // 77 -> 434
		325: y = 468; // 78 -> 433
		155: y = 234; // 79 -> 432
		310: y = 117; // 80 -> 431
		125: y = 306; // 81 -> 430
		250: y = 153; // 82 -> 429
		500: y = 324; // 83 -> 428
		505: y = 162; // 84 -> 427
		483: y = 81; // 85 -> 426
		471: y = 288; // 86 -> 425
		447: y = 144; // 87 -> 424
		367: y = 72; // 88 -> 423
		207: y = 36; // 89 -> 422
		414: y = 18; // 90 -> 421
		301: y = 9; // 91 -> 420
		75: y = 268; // 92 -> 419
		150: y = 134; // 93 -> 418
		300: y = 67; // 94 -> 417
		73: y = 297; // 95 -> 416
		146: y = 412; // 96 -> 415
		292: y = 206; // 97 -> 414
		89: y = 103; // 98 -> 413
		178: y = 315; // 99 -> 412
		356: y = 405; // 100 -> 411
		217: y = 450; // 101 -> 410
		434: y = 225; // 102 -> 409
		373: y = 376; // 103 -> 408
		251: y = 188; // 104 -> 407
		502: y = 94; // 105 -> 406
		509: y = 47; // 106 -> 405
		491: y = 287; // 107 -> 404
		455: y = 391; // 108 -> 403
		415: y = 459; // 109 -> 402
		303: y = 493; // 110 -> 401
		79: y = 510; // 111 -> 400
		158: y = 255; // 112 -> 399
		316: y = 375; // 113 -> 398
		105: y = 435; // 114 -> 397
		210: y = 465; // 115 -> 396
		420: y = 480; // 116 -> 395
		345: y = 240; // 117 -> 394
		163: y = 120; // 118 -> 393
		326: y = 60; // 119 -> 392
		157: y = 30; // 120 -> 391
		314: y = 15; // 121 -> 390
		101: y = 271; // 122 -> 389
		202: y = 399; // 123 -> 388
		404: y = 463; // 124 -> 387
		313: y = 495; // 125 -> 386
		99: y = 511; // 126 -> 385
		198: y = 503; // 127 -> 384
		396: y = 499; // 128 -> 383
		265: y = 497; // 129 -> 382
		3: y = 496; // 130 -> 381
		6: y = 248; // 131 -> 380
		12: y = 124; // 132 -> 379
		24: y = 62; // 133 -> 378
		48: y = 31; // 134 -> 377
		96: y = 263; // 135 -> 376
		192: y = 395; // 136 -> 375
		384: y = 461; // 137 -> 374
		273: y = 494; // 138 -> 373
		51: y = 247; // 139 -> 372
		102: y = 371; // 140 -> 371
		204: y = 433; // 141 -> 370
		408: y = 464; // 142 -> 369
		289: y = 232; // 143 -> 368
		83: y = 116; // 144 -> 367
		166: y = 58; // 145 -> 366
		332: y = 29; // 146 -> 365
		137: y = 262; // 147 -> 364
		274: y = 131; // 148 -> 363
		53: y = 329; // 149 -> 362
		106: y = 428; // 150 -> 361
		212: y = 214; // 151 -> 360
		424: y = 107; // 152 -> 359
		321: y = 317; // 153 -> 358
		147: y = 406; // 154 -> 357
		294: y = 203; // 155 -> 356
		93: y = 365; // 156 -> 355
		186: y = 446; // 157 -> 354
		372: y = 223; // 158 -> 353
		249: y = 359; // 159 -> 352
		498: y = 443; // 160 -> 351
		501: y = 469; // 161 -> 350
		507: y = 482; // 162 -> 349
		487: y = 241; // 163 -> 348
		479: y = 368; // 164 -> 347
		431: y = 184; // 165 -> 346
		335: y = 92; // 166 -> 345
		143: y = 46; // 167 -> 344
		286: y = 23; // 168 -> 343
		45: y = 259; // 169 -> 342
		90: y = 393; // 170 -> 341
		180: y = 460; // 171 -> 340
		360: y = 230; // 172 -> 339
		193: y = 115; // 173 -> 338
		386: y = 305; // 174 -> 337
		277: y = 400; // 175 -> 336
		59: y = 200; // 176 -> 335
		118: y = 100; // 177 -> 334
		236: y = 50; // 178 -> 333
		472: y = 25; // 179 -> 332
		417: y = 260; // 180 -> 331
		339: y = 130; // 181 -> 330
		183: y = 65; // 182 -> 329
		366: y = 296; // 183 -> 328
		205: y = 148; // 184 -> 327
		410: y = 74; // 185 -> 326
		293: y = 37; // 186 -> 325
		91: y = 282; // 187 -> 324
		182: y = 141; // 188 -> 323
		364: y = 334; // 189 -> 322
		201: y = 167; // 190 -> 321
		402: y = 347; // 191 -> 320
		309: y = 421; // 192 -> 319
		123: y = 474; // 193 -> 318
		246: y = 237; // 194 -> 317
		492: y = 382; // 195 -> 316
		457: y = 191; // 196 -> 315
		387: y = 343; // 197 -> 314
		279: y = 419; // 198 -> 313
		63: y = 473; // 199 -> 312
		126: y = 484; // 200 -> 311
		252: y = 242; // 201 -> 310
		504: y = 121; // 202 -> 309
		481: y = 308; // 203 -> 308
		467: y = 154; // 204 -> 307
		439: y = 77; // 205 -> 306
		383: y = 302; // 206 -> 305
		239: y = 151; // 207 -> 304
		478: y = 323; // 208 -> 303
		429: y = 425; // 209 -> 302
		331: y = 476; // 210 -> 301
		135: y = 238; // 211 -> 300
		270: y = 119; // 212 -> 299
		13: y = 307; // 213 -> 298
		26: y = 401; // 214 -> 297
		52: y = 448; // 215 -> 296
		104: y = 224; // 216 -> 295
		208: y = 112; // 217 -> 294
		416: y = 56; // 218 -> 293
		337: y = 28; // 219 -> 292
		179: y = 14; // 220 -> 291
		358: y = 7; // 221 -> 290
		221: y = 267; // 222 -> 289
		442: y = 397; // 223 -> 288
		357: y = 462; // 224 -> 287
		219: y = 231; // 225 -> 286
		438: y = 379; // 226 -> 285
		381: y = 437; // 227 -> 284
		235: y = 466; // 228 -> 283
		470: y = 233; // 229 -> 282
		445: y = 380; // 230 -> 281
		363: y = 190; // 231 -> 280
		199: y = 95; // 232 -> 279
		398: y = 295; // 233 -> 278
		269: y = 411; // 234 -> 277
		11: y = 453; // 235 -> 276
		22: y = 490; // 236 -> 275
		44: y = 245; // 237 -> 274
		88: y = 370; // 238 -> 273
		176: y = 185; // 239 -> 272
		352: y = 340; // 240 -> 271
		209: y = 170; // 241 -> 270
		418: y = 85; // 242 -> 269
		341: y = 290; // 243 -> 268
		187: y = 145; // 244 -> 267
		374: y = 320; // 245 -> 266
		253: y = 160; // 246 -> 265
		506: y = 80; // 247 -> 264
		485: y = 40; // 248 -> 263
		475: y = 20; // 249 -> 262
		423: y = 10; // 250 -> 261
		351: y = 5; // 251 -> 260
		175: y = 266; // 252 -> 259
		350: y = 133; // 253 -> 258
		173: y = 330; // 254 -> 257
		346: y = 165; // 255 -> 256
		165: y = 346; // 256 -> 255
		330: y = 173; // 257 -> 254
		133: y = 350; // 258 -> 253
		266: y = 175; // 259 -> 252
		5: y = 351; // 260 -> 251
		10: y = 423; // 261 -> 250
		20: y = 475; // 262 -> 249
		40: y = 485; // 263 -> 248
		80: y = 506; // 264 -> 247
		160: y = 253; // 265 -> 246
		320: y = 374; // 266 -> 245
		145: y = 187; // 267 -> 244
		290: y = 341; // 268 -> 243
		85: y = 418; // 269 -> 242
		170: y = 209; // 270 -> 241
		340: y = 352; // 271 -> 240
		185: y = 176; // 272 -> 239
		370: y = 88; // 273 -> 238
		245: y = 44; // 274 -> 237
		490: y = 22; // 275 -> 236
		453: y = 11; // 276 -> 235
		411: y = 269; // 277 -> 234
		295: y = 398; // 278 -> 233
		95: y = 199; // 279 -> 232
		190: y = 363; // 280 -> 231
		380: y = 445; // 281 -> 230
		233: y = 470; // 282 -> 229
		466: y = 235; // 283 -> 228
		437: y = 381; // 284 -> 227
		379: y = 438; // 285 -> 226
		231: y = 219; // 286 -> 225
		462: y = 357; // 287 -> 224
		397: y = 442; // 288 -> 223
		267: y = 221; // 289 -> 222
		7: y = 358; // 290 -> 221
		14: y = 179; // 291 -> 220
		28: y = 337; // 292 -> 219
		56: y = 416; // 293 -> 218
		112: y = 208; // 294 -> 217
		224: y = 104; // 295 -> 216
		448: y = 52; // 296 -> 215
		401: y = 26; // 297 -> 214
		307: y = 13; // 298 -> 213
		119: y = 270; // 299 -> 212
		238: y = 135; // 300 -> 211
		476: y = 331; // 301 -> 210
		425: y = 429; // 302 -> 209
		323: y = 478; // 303 -> 208
		151: y = 239; // 304 -> 207
		302: y = 383; // 305 -> 206
		77: y = 439; // 306 -> 205
		154: y = 467; // 307 -> 204
		308: y = 481; // 308 -> 203
		121: y = 504; // 309 -> 202
		242: y = 252; // 310 -> 201
		484: y = 126; // 311 -> 200
		473: y = 63; // 312 -> 199
		419: y = 279; // 313 -> 198
		343: y = 387; // 314 -> 197
		191: y = 457; // 315 -> 196
		382: y = 492; // 316 -> 195
		237: y = 246; // 317 -> 194
		474: y = 123; // 318 -> 193
		421: y = 309; // 319 -> 192
		347: y = 402; // 320 -> 191
		167: y = 201; // 321 -> 190
		334: y = 364; // 322 -> 189
		141: y = 182; // 323 -> 188
		282: y = 91; // 324 -> 187
		37: y = 293; // 325 -> 186
		74: y = 410; // 326 -> 185
		148: y = 205; // 327 -> 184
		296: y = 366; // 328 -> 183
		65: y = 183; // 329 -> 182
		130: y = 339; // 330 -> 181
		260: y = 417; // 331 -> 180
		25: y = 472; // 332 -> 179
		50: y = 236; // 333 -> 178
		100: y = 118; // 334 -> 177
		200: y = 59; // 335 -> 176
		400: y = 277; // 336 -> 175
		305: y = 386; // 337 -> 174
		115: y = 193; // 338 -> 173
		230: y = 360; // 339 -> 172
		460: y = 180; // 340 -> 171
		393: y = 90; // 341 -> 170
		259: y = 45; // 342 -> 169
		23: y = 286; // 343 -> 168
		46: y = 143; // 344 -> 167
		92: y = 335; // 345 -> 166
		184: y = 431; // 346 -> 165
		368: y = 479; // 347 -> 164
		241: y = 487; // 348 -> 163
		482: y = 507; // 349 -> 162
		469: y = 501; // 350 -> 161
		443: y = 498; // 351 -> 160
		359: y = 249; // 352 -> 159
		223: y = 372; // 353 -> 158
		446: y = 186; // 354 -> 157
		365: y = 93; // 355 -> 156
		203: y = 294; // 356 -> 155
		406: y = 147; // 357 -> 154
		317: y = 321; // 358 -> 153
		107: y = 424; // 359 -> 152
		214: y = 212; // 360 -> 151
		428: y = 106; // 361 -> 150
		329: y = 53; // 362 -> 149
		131: y = 274; // 363 -> 148
		262: y = 137; // 364 -> 147
		29: y = 332; // 365 -> 146
		58: y = 166; // 366 -> 145
		116: y = 83; // 367 -> 144
		232: y = 289; // 368 -> 143
		464: y = 408; // 369 -> 142
		433: y = 204; // 370 -> 141
		371: y = 102; // 371 -> 140
		247: y = 51; // 372 -> 139
		494: y = 273; // 373 -> 138
		461: y = 384; // 374 -> 137
		395: y = 192; // 375 -> 136
		263: y = 96; // 376 -> 135
		31: y = 48; // 377 -> 134
		62: y = 24; // 378 -> 133
		124: y = 12; // 379 -> 132
		248: y = 6; // 380 -> 131
		496: y = 3; // 381 -> 130
		497: y = 265; // 382 -> 129
		499: y = 396; // 383 -> 128
		503: y = 198; // 384 -> 127
		511: y = 99; // 385 -> 126
		495: y = 313; // 386 -> 125
		463: y = 404; // 387 -> 124
		399: y = 202; // 388 -> 123
		271: y = 101; // 389 -> 122
		15: y = 314; // 390 -> 121
		30: y = 157; // 391 -> 120
		60: y = 326; // 392 -> 119
		120: y = 163; // 393 -> 118
		240: y = 345; // 394 -> 117
		480: y = 420; // 395 -> 116
		465: y = 210; // 396 -> 115
		435: y = 105; // 397 -> 114
		375: y = 316; // 398 -> 113
		255: y = 158; // 399 -> 112
		510: y = 79; // 400 -> 111
		493: y = 303; // 401 -> 110
		459: y = 415; // 402 -> 109
		391: y = 455; // 403 -> 108
		287: y = 491; // 404 -> 107
		47: y = 509; // 405 -> 106
		94: y = 502; // 406 -> 105
		188: y = 251; // 407 -> 104
		376: y = 373; // 408 -> 103
		225: y = 434; // 409 -> 102
		450: y = 217; // 410 -> 101
		405: y = 356; // 411 -> 100
		315: y = 178; // 412 -> 99
		103: y = 89; // 413 -> 98
		206: y = 292; // 414 -> 97
		412: y = 146; // 415 -> 96
		297: y = 73; // 416 -> 95
		67: y = 300; // 417 -> 94
		134: y = 150; // 418 -> 93
		268: y = 75; // 419 -> 92
		9: y = 301; // 420 -> 91
		18: y = 414; // 421 -> 90
		36: y = 207; // 422 -> 89
		72: y = 367; // 423 -> 88
		144: y = 447; // 424 -> 87
		288: y = 471; // 425 -> 86
		81: y = 483; // 426 -> 85
		162: y = 505; // 427 -> 84
		324: y = 500; // 428 -> 83
		153: y = 250; // 429 -> 82
		306: y = 125; // 430 -> 81
		117: y = 310; // 431 -> 80
		234: y = 155; // 432 -> 79
		468: y = 325; // 433 -> 78
		441: y = 426; // 434 -> 77
		355: y = 213; // 435 -> 76
		215: y = 354; // 436 -> 75
		430: y = 177; // 437 -> 74
		333: y = 336; // 438 -> 73
		139: y = 168; // 439 -> 72
		278: y = 84; // 440 -> 71
		61: y = 42; // 441 -> 70
		122: y = 21; // 442 -> 69
		244: y = 258; // 443 -> 68
		488: y = 129; // 444 -> 67
		449: y = 328; // 445 -> 66
		403: y = 164; // 446 -> 65
		311: y = 82; // 447 -> 64
		127: y = 41; // 448 -> 63
		254: y = 284; // 449 -> 62
		508: y = 142; // 450 -> 61
		489: y = 71; // 451 -> 60
		451: y = 299; // 452 -> 59
		407: y = 413; // 453 -> 58
		319: y = 454; // 454 -> 57
		111: y = 227; // 455 -> 56
		222: y = 377; // 456 -> 55
		444: y = 436; // 457 -> 54
		361: y = 218; // 458 -> 53
		195: y = 109; // 459 -> 52
		390: y = 318; // 460 -> 51
		285: y = 159; // 461 -> 50
		43: y = 327; // 462 -> 49
		86: y = 427; // 463 -> 48
		172: y = 477; // 464 -> 47
		344: y = 486; // 465 -> 46
		161: y = 243; // 466 -> 45
		322: y = 369; // 467 -> 44
		149: y = 432; // 468 -> 43
		298: y = 216; // 469 -> 42
		69: y = 108; // 470 -> 41
		138: y = 54; // 471 -> 40
		276: y = 27; // 472 -> 39
		57: y = 261; // 473 -> 38
		114: y = 394; // 474 -> 37
		228: y = 197; // 475 -> 36
		456: y = 362; // 476 -> 35
		385: y = 181; // 477 -> 34
		275: y = 338; // 478 -> 33
		55: y = 169; // 479 -> 32
		110: y = 348; // 480 -> 31
		220: y = 174; // 481 -> 30
		440: y = 87; // 482 -> 29
		353: y = 291; // 483 -> 28
		211: y = 409; // 484 -> 27
		422: y = 452; // 485 -> 26
		349: y = 226; // 486 -> 25
		171: y = 113; // 487 -> 24
		342: y = 304; // 488 -> 23
		189: y = 152; // 489 -> 22
		378: y = 76; // 490 -> 21
		229: y = 38; // 491 -> 20
		458: y = 19; // 492 -> 19
		389: y = 257; // 493 -> 18
		283: y = 392; // 494 -> 17
		39: y = 196; // 495 -> 16
		78: y = 98; // 496 -> 15
		156: y = 49; // 497 -> 14
		312: y = 272; // 498 -> 13
		97: y = 136; // 499 -> 12
		194: y = 68; // 500 -> 11
		388: y = 34; // 501 -> 10
		281: y = 17; // 502 -> 9
		35: y = 256; // 503 -> 8
		70: y = 128; // 504 -> 7
		140: y = 64; // 505 -> 6
		280: y = 32; // 506 -> 5
		33: y = 16; // 507 -> 4
		66: y = 8; // 508 -> 3
		132: y = 4; // 509 -> 2
		264: y = 2; // 510 -> 1
	endcase
endmodule

// -------------------------------------------------------------------------
//Berlekamp circuit for Reed-Solomon decoder
//Copyright (C) Wed Apr 28 11:17:23 2004
//by Ming-Han Lei(hendrik@humanistic.org)
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
// --------------------------------------------------------------------------

module rsdec_berl (lambda_out, omega_out, syndrome0, syndrome1, syndrome2, syndrome3, syndrome4, syndrome5, syndrome6, syndrome7, 
		D, DI, count, phase0, phase8, enable, clk, clrn);
	input clk, clrn, enable, phase0, phase8;
	input [8:0] syndrome0;
	input [8:0] syndrome1;
	input [8:0] syndrome2;
	input [8:0] syndrome3;
	input [8:0] syndrome4;
	input [8:0] syndrome5;
	input [8:0] syndrome6;
	input [8:0] syndrome7;
	input [8:0] DI;
	input [3:0] count;
	output [8:0] lambda_out;
	output [8:0] omega_out;
	reg [8:0] lambda_out;
	reg [8:0] omega_out;
	output [8:0] D;
	reg [8:0] D;

	//integer j;
	reg init, delta;
	reg [1:0] L;
	reg [8:0] lambda0;
	reg [8:0] lambda1;
	reg [8:0] lambda2;
	reg [8:0] lambda3;
	reg [8:0] lambda4;
	reg [8:0] lambda5;
	reg [8:0] lambda6;
	reg [8:0] lambda7;
	reg [8:0] B0;
	reg [8:0] B1;
	reg [8:0] B2;
	reg [8:0] B3;
	reg [8:0] B4;
	reg [8:0] B5;
	reg [8:0] B6;
	reg [8:0] omega0;
	reg [8:0] omega1;
	reg [8:0] omega2;
	reg [8:0] omega3;
	reg [8:0] omega4;
	reg [8:0] omega5;
	reg [8:0] omega6;
	reg [8:0] omega7;
	reg [8:0] A0;
	reg [8:0] A1;
	reg [8:0] A2;
	reg [8:0] A3;
	reg [8:0] A4;
	reg [8:0] A5;
	reg [8:0] A6;

	wire [8:0] tmp0;
	wire [8:0] tmp1;
	wire [8:0] tmp2;
	wire [8:0] tmp3;
	wire [8:0] tmp4;
	wire [8:0] tmp5;
	wire [8:0] tmp6;
	wire [8:0] tmp7;

	always @ (tmp1) lambda_out = tmp1;
	always @ (tmp3) omega_out = tmp3;

	always @ (L or D or count)
		// delta = (D != 0 && 2*L <= i);
		if (D != 0 && count >= {L, 1'b0}) delta = 1;
		else delta = 0;

	rsdec_berl_multiply x0 (tmp0, B6, D, lambda0, syndrome0, phase0);
	rsdec_berl_multiply x1 (tmp1, lambda7, DI, lambda1, syndrome1, phase0);
	rsdec_berl_multiply x2 (tmp2, A6, D, lambda2, syndrome2, phase0);
	rsdec_berl_multiply x3 (tmp3, omega7, DI, lambda3, syndrome3, phase0);
	multiply x4 (tmp4, lambda4, syndrome4);
	multiply x5 (tmp5, lambda5, syndrome5);
	multiply x6 (tmp6, lambda6, syndrome6);
	multiply x7 (tmp7, lambda7, syndrome7);

	always @ (posedge clk)// or negedge clrn)
	begin
		// for (j = t-1; j >=0; j--)
		//	if (j != 0) lambda[j] += D * B[j-1];
/*		if (~clrn)
		begin

//			for (j = 0; j < 8; j = j + 1) lambda[j] <= 0;
//			for (j = 0; j < 7; j = j + 1) B[j] <= 0;
//			for (j = 0; j < 8; j = j + 1) omega[j] <= 0;
//			for (j = 0; j < 7; j = j + 1) A[j] <= 0;

			lambda0 <= 0;
			lambda1 <= 0;
			lambda2 <= 0;
			lambda3 <= 0;
			lambda4 <= 0;
			lambda5 <= 0;
			lambda6 <= 0;
			lambda7 <= 0;
			B0 <= 0;
			B1 <= 0;
			B2 <= 0;
			B3 <= 0;
			B4 <= 0;
			B5 <= 0;
			B6 <= 0;
			omega0 <= 0;
			omega1 <= 0;
			omega2 <= 0;
			omega3 <= 0;
			omega4 <= 0;
			omega5 <= 0;
			omega6 <= 0;
			omega7 <= 0;
			A0 <= 0;
			A1 <= 0;
			A2 <= 0;
			A3 <= 0;
			A4 <= 0;
			A5 <= 0;
			A6 <= 0;
		end
		else*/ if (~enable)
		begin
			/*
			lambda[0] <= 1;
			for (j = 1; j < 8; j = j +1) lambda[j] <= 0;
			B[0] <= 1;
			for (j = 1; j < 7; j = j +1) B[j] <= 0;
			omega[0] <= 1;
			for (j = 1; j < 8; j = j +1) omega[j] <= 0;
			for (j = 0; j < 7; j = j + 1) A[j] <= 0;
			*/
			lambda0 <= 1;
			lambda1 <= 0;
			lambda2 <= 0;
			lambda3 <= 0;
			lambda4 <= 0;
			lambda5 <= 0;
			lambda6 <= 0;
			lambda7 <= 0;
			//for (j = 1; j < 12; j = j +1) lambda[j] <= 0;
			B0 <= 1;
			B1 <= 0;
			B2 <= 0;
			B3 <= 0;
			B4 <= 0;
			B5 <= 0;
			B6 <= 0;
			//for (j = 1; j < 11; j = j +1) B[j] <= 0;
			omega0 <= 1;
			omega1 <= 0;
			omega2 <= 0;
			omega3 <= 0;
			omega4 <= 0;
			omega5 <= 0;
			omega6 <= 0;
			omega7 <= 0;
			//for (j = 1; j < 12; j = j +1) omega[j] <= 0;
			A0 <= 0;
			A1 <= 0;
			A2 <= 0;
			A3 <= 0;
			A4 <= 0;
			A5 <= 0;
			A6 <= 0;
		end
		else
		begin
			if (~phase0)
			begin
				if (~phase8) lambda0 <= lambda7 ^ tmp0;
				else lambda0 <= lambda7;
	//			for (j = 1; j < 8; j = j + 1)
	//				lambda[j] <= lambda[j-1];
				lambda1 <= lambda0;
				lambda2 <= lambda1;
				lambda3 <= lambda2;
				lambda4 <= lambda3;
				lambda5 <= lambda4;
				lambda6 <= lambda5;
				lambda7 <= lambda6;
//			end

		// for (j = t-1; j >=0; j--)
		//	if (delta) B[j] = lambda[j] *DI;
		//	else if (j != 0) B[j] = B[j-1];
		//	else B[j] = 0;
//			if (~phase0)
//			begin
				if (delta)	B0 <= tmp1;
				else if (~phase8) B0 <= B6;
				else B0 <= 0;
		//		for (j = 1; j < 7; j = j + 1)
	//				B[j] <= B[j-1];
				B1 <= B0;
				B2 <= B1;
				B3 <= B2;
				B4 <= B3;
				B5 <= B4;
				B6 <= B5;
//			end

		// for (j = t-1; j >=0; j--)
		//	if (j != 0) omega[j] += D * A[j-1];
//			if (~phase0)
//			begin
				if (~phase8) omega0 <= omega7 ^ tmp2;
				else omega0 <= omega7;
			//	for (j = 1; j < 8; j = j + 1)
			//		omega[j] <= omega[j-1];
				omega1 <= omega0;
				omega2 <= omega1;
				omega3 <= omega2;
				omega4 <= omega3;
				omega5 <= omega4;
				omega6 <= omega5;
				omega7 <= omega6;
//			end

		// for (j = t-1; j >=0; j--)
		//	if (delta) A[j] = omega[j] *DI;
		//	else if (j != 0) A[j] = A[j-1];
		//	else A[j] = 0;
//			if (~phase0)
//			begin
				if (delta)	A0 <= tmp3;
				else if (~phase8) A0 <= A6;
				else A0 <= 0;
			//	for (j = 1; j < 7; j = j + 1)
			//		A[j] <= A[j-1];
				A1 <= A0;
				A2 <= A1;
				A3 <= A2;
				A4 <= A3;
				A5 <= A4;
				A6 <= A5;
			end
		end
	end

	always @ (posedge clk)// or negedge clrn)
	begin
		if (~clrn)
		begin
			L <= 0;
			D <= 0;
		end
		else
		begin
		// if (delta) L = i - L + 1;
			if ((phase0 & delta) && (count != -1)) L <= count - L + 1;
		//for (D = j = 0; j < t; j = j + 1)
		//	D += lambda[j] * syndrome[t-j-1];
			if (phase0)
				D <= tmp0 ^ tmp1 ^ tmp2 ^ tmp3 ^ tmp4 ^ tmp5 ^ tmp6 ^ tmp7;
		end
	end

endmodule


module rsdec_berl_multiply (y, a, b, c, d, e);
	input [8:0] a, b, c, d;
	input e;
	output [8:0] y;
	wire [8:0] y;
	reg [8:0] p, q;

	always @ (a or c or e)
		if (e) p = c;
		else p = a;
	always @ (b or d or e)
		if (e) q = d;
		else q = b;

	multiply x0 (y, p, q);

endmodule

module multiply (y, a, b);
	input [8:0] a, b;
	output [8:0] y;
	wire [8:0] y;
	reg [17:0] tempy;

	assign y = tempy[8:0];

	always @(a or b)
	begin
		tempy = a * b;
	end
/*
	always @ (a or b)
	begin
		y[0] = (a[0] & b[0]) ^ (a[1] & b[8]) ^ (a[2] & b[7]) ^ (a[3] & b[6]) ^ (a[4] & b[5]) ^ (a[5] & b[4]) ^ (a[6] & b[3]) ^ (a[6] & b[8]) ^ (a[7] & b[2]) ^ (a[7] & b[7]) ^ (a[8] & b[1]) ^ (a[8] & b[6]);
		y[1] = (a[0] & b[1]) ^ (a[1] & b[0]) ^ (a[2] & b[8]) ^ (a[3] & b[7]) ^ (a[4] & b[6]) ^ (a[5] & b[5]) ^ (a[6] & b[4]) ^ (a[7] & b[3]) ^ (a[7] & b[8]) ^ (a[8] & b[2]) ^ (a[8] & b[7]);
		y[2] = (a[0] & b[2]) ^ (a[1] & b[1]) ^ (a[2] & b[0]) ^ (a[3] & b[8]) ^ (a[4] & b[7]) ^ (a[5] & b[6]) ^ (a[6] & b[5]) ^ (a[7] & b[4]) ^ (a[8] & b[3]) ^ (a[8] & b[8]);
		y[3] = (a[0] & b[3]) ^ (a[1] & b[2]) ^ (a[2] & b[1]) ^ (a[3] & b[0]) ^ (a[4] & b[8]) ^ (a[5] & b[7]) ^ (a[6] & b[6]) ^ (a[7] & b[5]) ^ (a[8] & b[4]);
		y[4] = (a[0] & b[4]) ^ (a[1] & b[3]) ^ (a[1] & b[8]) ^ (a[2] & b[2]) ^ (a[2] & b[7]) ^ (a[3] & b[1]) ^ (a[3] & b[6]) ^ (a[4] & b[0]) ^ (a[4] & b[5]) ^ (a[5] & b[4]) ^ (a[5] & b[8]) ^ (a[6] & b[3]) ^ (a[6] & b[7]) ^ (a[6] & b[8]) ^ (a[7] & b[2]) ^ (a[7] & b[6]) ^ (a[7] & b[7]) ^ (a[8] & b[1]) ^ (a[8] & b[5]) ^ (a[8] & b[6]);
		y[5] = (a[0] & b[5]) ^ (a[1] & b[4]) ^ (a[2] & b[3]) ^ (a[2] & b[8]) ^ (a[3] & b[2]) ^ (a[3] & b[7]) ^ (a[4] & b[1]) ^ (a[4] & b[6]) ^ (a[5] & b[0]) ^ (a[5] & b[5]) ^ (a[6] & b[4]) ^ (a[6] & b[8]) ^ (a[7] & b[3]) ^ (a[7] & b[7]) ^ (a[7] & b[8]) ^ (a[8] & b[2]) ^ (a[8] & b[6]) ^ (a[8] & b[7]);
		y[6] = (a[0] & b[6]) ^ (a[1] & b[5]) ^ (a[2] & b[4]) ^ (a[3] & b[3]) ^ (a[3] & b[8]) ^ (a[4] & b[2]) ^ (a[4] & b[7]) ^ (a[5] & b[1]) ^ (a[5] & b[6]) ^ (a[6] & b[0]) ^ (a[6] & b[5]) ^ (a[7] & b[4]) ^ (a[7] & b[8]) ^ (a[8] & b[3]) ^ (a[8] & b[7]) ^ (a[8] & b[8]);
		y[7] = (a[0] & b[7]) ^ (a[1] & b[6]) ^ (a[2] & b[5]) ^ (a[3] & b[4]) ^ (a[4] & b[3]) ^ (a[4] & b[8]) ^ (a[5] & b[2]) ^ (a[5] & b[7]) ^ (a[6] & b[1]) ^ (a[6] & b[6]) ^ (a[7] & b[0]) ^ (a[7] & b[5]) ^ (a[8] & b[4]) ^ (a[8] & b[8]);
		y[8] = (a[0] & b[8]) ^ (a[1] & b[7]) ^ (a[2] & b[6]) ^ (a[3] & b[5]) ^ (a[4] & b[4]) ^ (a[5] & b[3]) ^ (a[5] & b[8]) ^ (a[6] & b[2]) ^ (a[6] & b[7]) ^ (a[7] & b[1]) ^ (a[7] & b[6]) ^ (a[8] & b[0]) ^ (a[8] & b[5]);
	end
*/
endmodule


// -------------------------------------------------------------------------
//Syndrome generator circuit in Reed-Solomon Decoder
//Copyright (C) Wed Apr 28 11:28:10 2004
//by Ming-Han Lei(hendrik@humanistic.org)
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
// --------------------------------------------------------------------------

module rsdec_syn_m0 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;
	always @ (x)
	begin
		y[0] = x[8];
		y[1] = x[0];
		y[2] = x[1];
		y[3] = x[2];
		y[4] = x[3] ^ x[8];
		y[5] = x[4];
		y[6] = x[5];
		y[7] = x[6];
		y[8] = x[7];
	end
endmodule

module rsdec_syn_m1 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;
	always @ (x)
	begin
		y[0] = x[7];
		y[1] = x[8];
		y[2] = x[0];
		y[3] = x[1];
		y[4] = x[2] ^ x[7];
		y[5] = x[3] ^ x[8];
		y[6] = x[4];
		y[7] = x[5];
		y[8] = x[6];
	end
endmodule

module rsdec_syn_m2 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;
	always @ (x)
	begin
		y[0] = x[6];
		y[1] = x[7];
		y[2] = x[8];
		y[3] = x[0];
		y[4] = x[1] ^ x[6];
		y[5] = x[2] ^ x[7];
		y[6] = x[3] ^ x[8];
		y[7] = x[4];
		y[8] = x[5];
	end
endmodule

module rsdec_syn_m3 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;
	always @ (x)
	begin
		y[0] = x[5];
		y[1] = x[6];
		y[2] = x[7];
		y[3] = x[8];
		y[4] = x[0] ^ x[5];
		y[5] = x[1] ^ x[6];
		y[6] = x[2] ^ x[7];
		y[7] = x[3] ^ x[8];
		y[8] = x[4];
	end
endmodule

module rsdec_syn_m4 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;
	always @ (x)
	begin
		y[0] = x[4];
		y[1] = x[5];
		y[2] = x[6];
		y[3] = x[7];
		y[4] = x[4] ^ x[8];
		y[5] = x[0] ^ x[5];
		y[6] = x[1] ^ x[6];
		y[7] = x[2] ^ x[7];
		y[8] = x[3] ^ x[8];
	end
endmodule

module rsdec_syn_m5 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;
	always @ (x)
	begin
		y[0] = x[3] ^ x[8];
		y[1] = x[4];
		y[2] = x[5];
		y[3] = x[6];
		y[4] = x[3] ^ x[7] ^ x[8];
		y[5] = x[4] ^ x[8];
		y[6] = x[0] ^ x[5];
		y[7] = x[1] ^ x[6];
		y[8] = x[2] ^ x[7];
	end
endmodule

module rsdec_syn_m6 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;
	always @ (x)
	begin
		y[0] = x[2] ^ x[7];
		y[1] = x[3] ^ x[8];
		y[2] = x[4];
		y[3] = x[5];
		y[4] = x[2] ^ x[6] ^ x[7];
		y[5] = x[3] ^ x[7] ^ x[8];
		y[6] = x[4] ^ x[8];
		y[7] = x[0] ^ x[5];
		y[8] = x[1] ^ x[6];
	end
endmodule

module rsdec_syn_m7 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;
	always @ (x)
	begin
		y[0] = x[1] ^ x[6];
		y[1] = x[2] ^ x[7];
		y[2] = x[3] ^ x[8];
		y[3] = x[4];
		y[4] = x[1] ^ x[5] ^ x[6];
		y[5] = x[2] ^ x[6] ^ x[7];
		y[6] = x[3] ^ x[7] ^ x[8];
		y[7] = x[4] ^ x[8];
		y[8] = x[0] ^ x[5];
	end
endmodule

module rsdec_syn (y0, y1, y2, y3, y4, y5, y6, y7, u, enable, shift, init, clk, clrn);
	input [8:0] u;
	input clk, clrn, shift, init, enable;
	output [8:0] y0;
	output [8:0] y1;
	output [8:0] y2;
	output [8:0] y3;
	output [8:0] y4;
	output [8:0] y5;
	output [8:0] y6;
	output [8:0] y7;
	reg [8:0] y0;
	reg [8:0] y1;
	reg [8:0] y2;
	reg [8:0] y3;
	reg [8:0] y4;
	reg [8:0] y5;
	reg [8:0] y6;
	reg [8:0] y7;

	wire [8:0] scale0;
	wire [8:0] scale1;
	wire [8:0] scale2;
	wire [8:0] scale3;
	wire [8:0] scale4;
	wire [8:0] scale5;
	wire [8:0] scale6;
	wire [8:0] scale7;

	rsdec_syn_m0 m0 (scale0, y0);
	rsdec_syn_m1 m1 (scale1, y1);
	rsdec_syn_m2 m2 (scale2, y2);
	rsdec_syn_m3 m3 (scale3, y3);
	rsdec_syn_m4 m4 (scale4, y4);
	rsdec_syn_m5 m5 (scale5, y5);
	rsdec_syn_m6 m6 (scale6, y6);
	rsdec_syn_m7 m7 (scale7, y7);

	always @ (posedge clk)// or negedge clrn)
	begin
		if (~clrn)
		begin
			y0 <= 0;
			y1 <= 0;
			y2 <= 0;
			y3 <= 0;
			y4 <= 0;
			y5 <= 0;
			y6 <= 0;
			y7 <= 0;
		end
		else if (init)
		begin
			y0 <= u;
			y1 <= u;
			y2 <= u;
			y3 <= u;
			y4 <= u;
			y5 <= u;
			y6 <= u;
			y7 <= u;
		end
		else if (enable)
		begin
			y0 <= scale0 ^ u;
			y1 <= scale1 ^ u;
			y2 <= scale2 ^ u;
			y3 <= scale3 ^ u;
			y4 <= scale4 ^ u;
			y5 <= scale5 ^ u;
			y6 <= scale6 ^ u;
			y7 <= scale7 ^ u;
		end
		else if (shift)
		begin
			y0 <= y1;
			y1 <= y2;
			y2 <= y3;
			y3 <= y4;
			y4 <= y5;
			y5 <= y6;
			y6 <= y7;
			y7 <= y0;
		end
	end

endmodule

// -------------------------------------------------------------------------
//Chien-Forney search circuit for Reed-Solomon decoder
//Copyright (C) Wed Apr 28 11:17:57 2004
//by Ming-Han Lei(hendrik@humanistic.org)
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
// --------------------------------------------------------------------------

module rsdec_chien_scale0 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;

	always @ (x)
	begin
		y[0] = x[0];
		y[1] = x[1];
		y[2] = x[2];
		y[3] = x[3];
		y[4] = x[4];
		y[5] = x[5];
		y[6] = x[6];
		y[7] = x[7];
		y[8] = x[8];
	end
endmodule

module rsdec_chien_scale1 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;

	always @ (x)
	begin
		y[0] = x[8];
		y[1] = x[0];
		y[2] = x[1];
		y[3] = x[2];
		y[4] = x[3] ^ x[8];
		y[5] = x[4];
		y[6] = x[5];
		y[7] = x[6];
		y[8] = x[7];
	end
endmodule

module rsdec_chien_scale2 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;

	always @ (x)
	begin
		y[0] = x[7];
		y[1] = x[8];
		y[2] = x[0];
		y[3] = x[1];
		y[4] = x[2] ^ x[7];
		y[5] = x[3] ^ x[8];
		y[6] = x[4];
		y[7] = x[5];
		y[8] = x[6];
	end
endmodule

module rsdec_chien_scale3 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;

	always @ (x)
	begin
		y[0] = x[6];
		y[1] = x[7];
		y[2] = x[8];
		y[3] = x[0];
		y[4] = x[1] ^ x[6];
		y[5] = x[2] ^ x[7];
		y[6] = x[3] ^ x[8];
		y[7] = x[4];
		y[8] = x[5];
	end
endmodule

module rsdec_chien_scale4 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;

	always @ (x)
	begin
		y[0] = x[5];
		y[1] = x[6];
		y[2] = x[7];
		y[3] = x[8];
		y[4] = x[0] ^ x[5];
		y[5] = x[1] ^ x[6];
		y[6] = x[2] ^ x[7];
		y[7] = x[3] ^ x[8];
		y[8] = x[4];
	end
endmodule

module rsdec_chien_scale5 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;

	always @ (x)
	begin
		y[0] = x[4];
		y[1] = x[5];
		y[2] = x[6];
		y[3] = x[7];
		y[4] = x[4] ^ x[8];
		y[5] = x[0] ^ x[5];
		y[6] = x[1] ^ x[6];
		y[7] = x[2] ^ x[7];
		y[8] = x[3] ^ x[8];
	end
endmodule

module rsdec_chien_scale6 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;

	always @ (x)
	begin
		y[0] = x[3] ^ x[8];
		y[1] = x[4];
		y[2] = x[5];
		y[3] = x[6];
		y[4] = x[3] ^ x[7] ^ x[8];
		y[5] = x[4] ^ x[8];
		y[6] = x[0] ^ x[5];
		y[7] = x[1] ^ x[6];
		y[8] = x[2] ^ x[7];
	end
endmodule

module rsdec_chien_scale7 (y, x);
	input [8:0] x;
	output [8:0] y;
	reg [8:0] y;

	always @ (x)
	begin
		y[0] = x[2] ^ x[7];
		y[1] = x[3] ^ x[8];
		y[2] = x[4];
		y[3] = x[5];
		y[4] = x[2] ^ x[6] ^ x[7];
		y[5] = x[3] ^ x[7] ^ x[8];
		y[6] = x[4] ^ x[8];
		y[7] = x[0] ^ x[5];
		y[8] = x[1] ^ x[6];
	end
endmodule

module rsdec_chien (error, alpha, lambda, omega, even, D, search, load, shorten, clk, clrn);
	input clk, clrn, load, search, shorten;
	input [8:0] D;
	input [8:0] lambda;
	input [8:0] omega;
	output [8:0] even, error;
	output [8:0] alpha;
	reg [8:0] even, error;
	reg [8:0] alpha;

	wire [8:0] scale0;
	wire [8:0] scale1;
	wire [8:0] scale2;
	wire [8:0] scale3;
	wire [8:0] scale4;
	wire [8:0] scale5;
	wire [8:0] scale6;
	wire [8:0] scale7;
	wire [8:0] scale8;
	wire [8:0] scale9;
	wire [8:0] scale10;
	wire [8:0] scale11;
	wire [8:0] scale12;
	wire [8:0] scale13;
	wire [8:0] scale14;
	wire [8:0] scale15;
	reg [8:0] data0;
	reg [8:0] data1;
	reg [8:0] data2;
	reg [8:0] data3;
	reg [8:0] data4;
	reg [8:0] data5;
	reg [8:0] data6;
	reg [8:0] data7;
	reg [8:0] a0;
	reg [8:0] a1;
	reg [8:0] a2;
	reg [8:0] a3;
	reg [8:0] a4;
	reg [8:0] a5;
	reg [8:0] a6;
	reg [8:0] a7;
	reg [8:0] l0;
	reg [8:0] l1;
	reg [8:0] l2;
	reg [8:0] l3;
	reg [8:0] l4;
	reg [8:0] l5;
	reg [8:0] l6;
	reg [8:0] l7;
	reg [8:0] o0;
	reg [8:0] o1;
	reg [8:0] o2;
	reg [8:0] o3;
	reg [8:0] o4;
	reg [8:0] o5;
	reg [8:0] o6;
	reg [8:0] o7;
	reg [8:0] odd, numerator;
	wire [8:0] tmp;
	//integer j;

	rsdec_chien_scale0 x0 (scale0, data0);
	rsdec_chien_scale1 x1 (scale1, data1);
	rsdec_chien_scale2 x2 (scale2, data2);
	rsdec_chien_scale3 x3 (scale3, data3);
	rsdec_chien_scale4 x4 (scale4, data4);
	rsdec_chien_scale5 x5 (scale5, data5);
	rsdec_chien_scale6 x6 (scale6, data6);
	rsdec_chien_scale7 x7 (scale7, data7);
	rsdec_chien_scale0 x8 (scale8, o0);
	rsdec_chien_scale1 x9 (scale9, o1);
	rsdec_chien_scale2 x10 (scale10, o2);
	rsdec_chien_scale3 x11 (scale11, o3);
	rsdec_chien_scale4 x12 (scale12, o4);
	rsdec_chien_scale5 x13 (scale13, o5);
	rsdec_chien_scale6 x14 (scale14, o6);
	rsdec_chien_scale7 x15 (scale15, o7);

	always @ (shorten or a0 or l0)
		if (shorten) data0 = a0;
		else data0 = l0;

	always @ (shorten or a1 or l1)
		if (shorten) data1 = a1;
		else data1 = l1;

	always @ (shorten or a2 or l2)
		if (shorten) data2 = a2;
		else data2 = l2;

	always @ (shorten or a3 or l3)
		if (shorten) data3 = a3;
		else data3 = l3;

	always @ (shorten or a4 or l4)
		if (shorten) data4 = a4;
		else data4 = l4;

	always @ (shorten or a5 or l5)
		if (shorten) data5 = a5;
		else data5 = l5;

	always @ (shorten or a6 or l6)
		if (shorten) data6 = a6;
		else data6 = l6;

	always @ (shorten or a7 or l7)
		if (shorten) data7 = a7;
		else data7 = l7;

	always @ (posedge clk)// or negedge clrn)
	begin
		if (~clrn)
		begin
			l0 <= 0;
			l1 <= 0;
			l2 <= 0;
			l3 <= 0;
			l4 <= 0;
			l5 <= 0;
			l6 <= 0;
			l7 <= 0;
			o0 <= 0;
			o1 <= 0;
			o2 <= 0;
			o3 <= 0;
			o4 <= 0;
			o5 <= 0;
			o6 <= 0;
			o7 <= 0;
			a0 <= 1;
			a1 <= 1;
			a2 <= 1;
			a3 <= 1;
			a4 <= 1;
			a5 <= 1;
			a6 <= 1;
			a7 <= 1;
		end
		else if (shorten)
		begin
			a0 <= scale0;
			a1 <= scale1;
			a2 <= scale2;
			a3 <= scale3;
			a4 <= scale4;
			a5 <= scale5;
			a6 <= scale6;
			a7 <= scale7;
		end
		else if (search)
		begin
			l0 <= scale0;
			l1 <= scale1;
			l2 <= scale2;
			l3 <= scale3;
			l4 <= scale4;
			l5 <= scale5;
			l6 <= scale6;
			l7 <= scale7;
			o0 <= scale8;
			o1 <= scale9;
			o2 <= scale10;
			o3 <= scale11;
			o4 <= scale12;
			o5 <= scale13;
			o6 <= scale14;
			o7 <= scale15;
		end
		else if (load)
		begin
			l0 <= lambda;
			l1 <= l0;
			l2 <= l1;
			l3 <= l2;
			l4 <= l3;
			l5 <= l4;
			l6 <= l5;
			l7 <= l6;
			o0 <= omega;
			o1 <= o0;
			o2 <= o1;
			o3 <= o2;
			o4 <= o3;
			o5 <= o4;
			o6 <= o5;
			o7 <= o6;
			a0 <= a7;
			a1 <= a0;
			a2 <= a1;
			a3 <= a2;
			a4 <= a3;
			a5 <= a4;
			a6 <= a5;
			a7 <= a6;
		end
	end

	always @ (l0 or l2 or l4 or l6)
		even = l0 ^ l2 ^ l4 ^ l6;

	always @ (l1 or l3 or l5 or l7)
		odd = l1 ^ l3 ^ l5 ^ l7;

	always @ (o0 or o1 or o2 or o3 or o4 or o5 or o6 or o7)
		numerator = o0 ^ o1 ^ o2 ^ o3 ^ o4 ^ o5 ^ o6 ^ o7;

	multiply m0 (tmp, numerator, D);

	always @ (even or odd or tmp)
		if (even == odd) error = tmp;
		else error = 0;

	always @ (a7) alpha = a7;

endmodule


