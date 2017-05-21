/* 
 * Copyright (C) 2002-2004 XimpleWare, info@ximpleware.com
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
to do list
1. Added atTerminal variable and make corresponding changes to various navigation functions (done)
2. Added special variable to Autopilot (done)
3. Port various selectElement_P, selectElementNS_P, selectElement_F, selectElementNS_F, and iterate
and compare the implementation along the way to see if there is any discrepancy (done)
4. Port selectAttr IterateAttr (done)
5. Start working on flex and bison, needs to define wchar_t!!!, may have to use lex instead (done)
6. Define corresponding data types need to construct an XPath expression (done)
7. Define various exception condition for XPath exception (done)
8. Port the core function that does evalNodeSet for LocationPathExpr (done)
9. Study interface emulation in C using func pointers (done)
10. 



1. add localname and prefix support
2. add teh rest of features in autopilot
3. probing possible memory leaks in yyparse
4. debugging the whole thing
 
