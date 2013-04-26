//===========================================================================//
// Purpose: PCCTS grammar for the options store file.
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
//                                                                           //
// This program is free software; you can redistribute it and/or modify it   //
// under the terms of the GNU General Public License as published by the     //
// Free Software Foundation; version 3 of the License, or any later version. //
//                                                                           //
// This program is distributed in the hope that it will be useful, but       //
// WITHOUT ANY WARRANTY; without even an implied warranty of MERCHANTABILITY //
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License   //
// for more details.                                                         //
//                                                                           //
// You should have received a copy of the GNU General Public License along   //
// with this program; if not, see <http://www.gnu.org/licenses>.             //
//---------------------------------------------------------------------------//

//===========================================================================//
#header

<<
#include <cstdio>
using namespace std;

#include "stdpccts.h"
#include "GenericTokenBuffer.h"

#include "TIO_PrintHandler.h"

#include "TOS_OptionsStore.h"

#include "TOP_OptionsFile.h"
#include "TOP_OptionsScanner_c.h"
>>

//===========================================================================//
#lexclass QUOTED_VALUE

#token CLOSE_QUOTE     "\""                    << mode( START ); >>
#token STRING          "~[\"\n]+"
#token UNCLOSED_STRING "[\n]"

//===========================================================================//
#lexclass START

#token                 "[\ \t]+"      << skip( ); >>
#token CPP_COMMENT     "//~[\n]*[\n]" << skip( ); newline( ); >>
#token BLOCK_COMMENT   "#~[\n]*[\n]"  << skip( ); newline( ); >>
#token NEW_LINE        "[\n]"         << skip( ); newline( ); >>
#token END_OF_FILE     "@"
#token OPEN_QUOTE      "\""           << mode( QUOTED_VALUE ); >>
#token OPEN_BRACE      "\{"
#token CLOSE_BRACE     "\}"
#token EQUAL           "="

#token INPUT_FILE_XML          "[Ii][Nn][Pp][Uu][Tt][_][Ff][Ii][Ll][Ee][_][Xx][Mm][Ll]"
#token INPUT_FILE_BLIF         "[Ii][Nn][Pp][Uu][Tt][_][Ff][Ii][Ll][Ee][_][Bb][Ll][Ii][Ff]"
#token INPUT_FILE_ARCH         "[Ii][Nn][Pp][Uu][Tt][_][Ff][Ii][Ll][Ee][_][Aa][Rr][Cc][Hh]{[Ii][Tt][Ee][Cc][Tt][Uu][Rr][Ee]}{[_][Dd][Ee][Ss][Cc][Rr][Ii][Pp][Tt][Ii][Oo][Nn]}{[_][Ff][Ii][Ll][Ee]}"
#token INPUT_FILE_FABRIC       "[Ii][Nn][Pp][Uu][Tt][_][Ff][Ii][Ll][Ee][_][Ff][Aa][Bb][Rr][Ii][Cc]"
#token INPUT_FILE_CIRCUIT      "[Ii][Nn][Pp][Uu][Tt][_][Ff][Ii][Ll][Ee][_][Cc][Ii][Rr][Cc][Uu][Ii][Tt]{[_][Dd][Ee][Ss][Cc][Rr][Ii][Pp][Tt][Ii][Oo][Nn]}{[_][Ff][Ii][Ll][Ee]}"
#token INPUT_ENABLE_XML        "[Ii][Nn][Pp][Uu][Tt][_][Ee][Nn][Aa][Bb][Ll][Ee][_][Xx][Mm][Ll]"
#token INPUT_ENABLE_BLIF       "[Ii][Nn][Pp][Uu][Tt][_][Ee][Nn][Aa][Bb][Ll][Ee][_][Bb][Ll][Ii][Ff]"
#token INPUT_ENABLE_ARCH       "[Ii][Nn][Pp][Uu][Tt][_][Ee][Nn][Aa][Bb][Ll][Ee][_][Aa][Rr][Cc][Hh]{[Ii][Tt][Ee][Cc][Tt][Uu][Rr][Ee]}{[_][Dd][Ee][Ss][Cc][Rr][Ii][Pp][Tt][Ii][Oo][Nn]}{[_][Ff][Ii][Ll][Ee]}"
#token INPUT_ENABLE_FABRIC     "[Ii][Nn][Pp][Uu][Tt][_][Ee][Nn][Aa][Bb][Ll][Ee][_][Ff][Aa][Bb][Rr][Ii][Cc]"
#token INPUT_ENABLE_CIRCUIT    "[Ii][Nn][Pp][Uu][Tt][_][Ee][Nn][Aa][Bb][Ll][Ee][_][Cc][Ii][Rr][Cc][Uu][Ii][Tt]"

#token OUTPUT_FILE_LOG         "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ff][Ii][Ll][Ee][_][Ll][Oo][Gg]"
#token OUTPUT_FILE_OPTIONS     "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ff][Ii][Ll][Ee][_][Oo][Pp][Tt]{[Ii][Oo][Nn]}[Ss]"
#token OUTPUT_FILE_XML         "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ff][Ii][Ll][Ee][_][Xx][Mm][Ll]"
#token OUTPUT_FILE_BLIF        "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ff][Ii][Ll][Ee][_][Bb][Ll][Ii][Ff]"
#token OUTPUT_FILE_ARCH        "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ff][Ii][Ll][Ee][_][Aa][Rr][Cc][Hh]{[Ii][Tt][Ee][Cc][Tt][Uu][Rr][Ee]}{[_][Dd][Ee][Ss][Cc][Rr][Ii][Pp][Tt][Ii][Oo][Nn]}{[_][Ff][Ii][Ll][Ee]}"
#token OUTPUT_FILE_FABRIC      "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ff][Ii][Ll][Ee][_][Ff][Aa][Bb][Rr][Ii][Cc]"
#token OUTPUT_FILE_CIRCUIT     "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ff][Ii][Ll][Ee][_][Cc][Ii][Rr][Cc][Uu][Ii][Tt]"
#token OUTPUT_FILE_RC_DELAYS   "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ff][Ii][Ll][Ee][_]{[Rr][Cc][_]}[Dd][Ee][Ll][Aa][Yy][Ss]"
#token OUTPUT_FILE_LAFF        "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ff][Ii][Ll][Ee][_][Ll][Aa][Ff][Ff]"
#token OUTPUT_EMAIL_METRICS    "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ee][Mm][Aa][Ii][Ll][_]([Aa][Dd][Dd][Rr][Ee][Ss][Ss]|[Mm][Ee][Tt][Rr][Ii][Cc][Ss])"
#token OUTPUT_ENABLE_LOG       "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ee][Nn][Aa][Bb][Ll][Ee][_][Ll][Oo][Gg]"
#token OUTPUT_ENABLE_OPTIONS   "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ee][Nn][Aa][Bb][Ll][Ee][_][Oo][Pp][Tt]{[Ii][Oo][Nn]}[Ss]"
#token OUTPUT_ENABLE_XML       "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ee][Nn][Aa][Bb][Ll][Ee][_][Xx][Mm][Ll]"
#token OUTPUT_ENABLE_BLIF      "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ee][Nn][Aa][Bb][Ll][Ee][_][Bb][Ll][Ii][Ff]"
#token OUTPUT_ENABLE_ARCH      "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ee][Nn][Aa][Bb][Ll][Ee][_][Aa][Rr][Cc][Hh]{[Ii][Tt][Ee][Cc][Tt][Uu][Rr][Ee]}{[_][Dd][Ee][Ss][Cc][Rr][Ii][Pp][Tt][Ii][Oo][Nn]}{[_][Ff][Ii][Ll][Ee]}"
#token OUTPUT_ENABLE_FABRIC    "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ee][Nn][Aa][Bb][Ll][Ee][_][Ff][Aa][Bb][Rr][Ii][Cc]"
#token OUTPUT_ENABLE_CIRCUIT   "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ee][Nn][Aa][Bb][Ll][Ee][_][Cc][Ii][Rr][Cc][Uu][Ii][Tt]"
#token OUTPUT_ENABLE_RC_DELAYS "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ee][Nn][Aa][Bb][Ll][Ee][_]{[Rr][Cc][_]}[Dd][Ee][Ll][Aa][Yy][Ss]"
#token OUTPUT_ENABLE_LAFF      "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ee][Nn][Aa][Bb][Ll][Ee][_][Ll][Aa][Ff][Ff]"
#token OUTPUT_LAFF_MODE        "[Oo][Uu][Tt][Pp][Uu][Tt][_][Ll][Aa][Ff][Ff][_][Mm][Oo][Dd][Ee]"
#token OUTPUT_RC_DELAYS_MODE   "[Oo][Uu][Tt][Pp][Uu][Tt][_]{[Rr][Cc][_]}[Dd][Ee][Ll][Aa][Yy]{[Ss]}[_][Mm][Oo][Dd][Ee]"
#token OUTPUT_RC_DELAYS_SORT   "[Oo][Uu][Tt][Pp][Uu][Tt][_]{[Rr][Cc][_]}[Dd][Ee][Ll][Aa][Yy]{[Ss]}[_][Ss][Oo][Rr][Tt]"
#token OUTPUT_RC_DELAYS_NETS   "[Oo][Uu][Tt][Pp][Uu][Tt][_]{[Rr][Cc][_]}[Dd][Ee][Ll][Aa][Yy]{[Ss]}[_][Nn][Ee][Tt]{[Ss]}"
#token OUTPUT_RC_DELAYS_BLOCKS "[Oo][Uu][Tt][Pp][Uu][Tt][_]{[Rr][Cc][_]}[Dd][Ee][Ll][Aa][Yy]{[Ss]}[_]([Bb][Ll][Oo][Cc][Kk]|[Cc][Ll][Bb]){[Ss]}"
#token OUTPUT_RC_DELAYS_LENGTH "[Oo][Uu][Tt][Pp][Uu][Tt][_]{[Rr][Cc][_]}[Dd][Ee][Ll][Aa][Yy]{[Ss]}[_][Mm][Aa][Xx][_]{[Ww][Ii][Rr][Ee][_]}[Ll][Ee][Nn]{[Gg][Tt][Hh]}"

#token FORMAT_MIN_GRID         "[Ff][Oo][Rr][Mm][Aa][Tt][_][Mm][Ii][Nn][_][Gg][Rr][Ii][Dd]"
#token FORMAT_TIME_STAMPS      "[Ff][Oo][Rr][Mm][Aa][Tt][_][Tt][Ii][Mm][Ee][_][Ss][Tt][Aa][Mm][Pp][Ss]"
#token DISPLAY_INFO_ACCEPT     "{[Dd][Ii][Ss][Pp][Ll][Aa][Yy][_]}[Ii][Nn][Ff][Oo]{[_][Aa][Cc][Cc][Ee][Pp][Tt]}"
#token DISPLAY_INFO_REJECT     "{[Dd][Ii][Ss][Pp][Ll][Aa][Yy][_]}[Ii][Nn][Ff][Oo][_][Rr][Ee][Jj][Ee][Cc][Tt]"
#token DISPLAY_WARNING_ACCEPT  "{[Dd][Ii][Ss][Pp][Ll][Aa][Yy][_]}[Ww][Aa][Rr][Nn][Ii][Nn][Gg]{[Ss]}{[_][Aa][Cc][Cc][Ee][Pp][Tt]}"
#token DISPLAY_WARNING_REJECT  "{[Dd][Ii][Ss][Pp][Ll][Aa][Yy][_]}[Ww][Aa][Rr][Nn][Ii][Nn][Gg]{[Ss]}[_][Rr][Ee][Jj][Ee][Cc][Tt]"
#token DISPLAY_ERROR_ACCEPT    "{[Dd][Ii][Ss][Pp][Ll][Aa][Yy][_]}[Ee][Rr][Rr][Oo][Rr]{[Ss]}{[_][Aa][Cc][Cc][Ee][Pp][Tt]}"
#token DISPLAY_ERROR_REJECT    "{[Dd][Ii][Ss][Pp][Ll][Aa][Yy][_]}[Ee][Rr][Rr][Oo][Rr]{[Ss]}[_][Rr][Ee][Jj][Ee][Cc][Tt]"
#token DISPLAY_TRACE_ACCEPT    "{[Dd][Ii][Ss][Pp][Ll][Aa][Yy][_]}[Tt][Rr][Aa][Cc][Ee]{[_][Aa][Cc][Cc][Ee][Pp][Tt]}"
#token DISPLAY_TRACE_REJECT    "{[Dd][Ii][Ss][Pp][Ll][Aa][Yy][_]}[Tt][Rr][Aa][Cc][Ee][_][Rr][Ee][Jj][Ee][Cc][Tt]"

#token TRACE_READ_OPTIONS      "[Tt][Rr][Aa][Cc][Ee][_][Rr][Ee][Aa][Dd][_][Oo][Pp][Tt]{[Ii][Oo][Nn]}[Ss]"
#token TRACE_READ_XML          "[Tt][Rr][Aa][Cc][Ee][_][Rr][Ee][Aa][Dd][_][Xx][Mm][Ll]"
#token TRACE_READ_BLIF         "[Tt][Rr][Aa][Cc][Ee][_][Rr][Ee][Aa][Dd][_][Bb][Ll][Ii][Ff]"
#token TRACE_READ_ARCH         "[Tt][Rr][Aa][Cc][Ee][_][Rr][Ee][Aa][Dd][_][Aa][Rr][Cc][Hh]{[Ii][Tt][Ee][Cc][Tt][Uu][Rr][Ee]}"
#token TRACE_READ_FABRIC       "[Tt][Rr][Aa][Cc][Ee][_][Rr][Ee][Aa][Dd][_][Ff][Aa][Bb][Rr][Ii][Cc]"
#token TRACE_READ_CIRCUIT      "[Tt][Rr][Aa][Cc][Ee][_][Rr][Ee][Aa][Dd][_][Cc][Ii][Rr][Cc][Uu][Ii][Tt]"

#token TRACE_VPR_SHOW_SETUP    "[Tt][Rr][Aa][Cc][Ee][_][Vv][Pp][Rr][_][Ss][Hh][Oo][Ww][_][Ss][Ee][Tt][Uu][Pp]"
#token TRACE_VPR_ECHO_FILE     "[Tt][Rr][Aa][Cc][Ee][_][Vv][Pp][Rr][_][Ee][Cc][Hh][Oo][_][Ff][Ii][Ll][Ee]{[Ss]}"

#token HALT_MAX_WARNINGS       "[Hh][Aa][Ll][Tt][_][Mm][Aa][Xx][_][Ww][Aa][Rr][Nn][Ii][Nn][Gg]{[Ss]}"
#token HALT_MAX_ERRORS         "[Hh][Aa][Ll][Tt][_][Mm][Aa][Xx][_][Ee][Rr][Rr][Oo][Rr]{[Ss]}"
#token EXECUTE_MODE            "[Ee][Xx][Ee][Cc][Uu][Tt][Ee]{[_][Mm][Oo][Dd][Ee]{[Ss]}}"

#token FABRIC_BLOCK_ENABLE       "[Ff][Aa][Bb][Rr][Ii][Cc][_]([Bb][Ll][Oo][Cc][Kk]|[Cc][Ll][Bb]){[Ss]}[_]{[Oo][Vv][Ee][Rr][Rr][Ii][Dd][Ee][_]}[Ee][Nn][Aa][Bb][Ll][Ee]"
#token FABRIC_CHANNEL_ENABLE     "[Ff][Aa][Bb][Rr][Ii][Cc][_][Cc][Hh][Aa][Nn][Nn][Ee][Ll]{[Ss]}[_]{[Oo][Vv][Ee][Rr][Rr][Ii][Dd][Ee][_]}[Ee][Nn][Aa][Bb][Ll][Ee]"
#token FABRIC_SWITCHBOX_ENABLE   "[Ff][Aa][Bb][Rr][Ii][Cc][_][Ss]{[Ww][Ii][Tt][Cc][Hh]}[Bb]{[Oo][Xx]{[Ee][Ss]}}[_]{[Oo][Vv][Ee][Rr][Rr][Ii][Dd][Ee][_]}[Ee][Nn][Aa][Bb][Ll][Ee]"
#token FABRIC_CONNECTIONBLOCK_ENABLE
                                 "[Ff][Aa][Bb][Rr][Ii][Cc][_][Cc]{[Oo][Nn][Nn][Ee][Cc][Tt]{[Ii][Oo][Nn]}}[Bb]{[Ll][Oo][Cc][Kk]}{[Ss]}[_]{[Oo][Vv][Ee][Rr][Rr][Ii][Dd][Ee][_]}[Ee][Nn][Aa][Bb][Ll][Ee]"

#token PACK_ALGORITHM            "[Pp][Aa][Cc][Kk][_][Aa][Ll][Gg][Oo][Rr][Ii][Tt][Hh][Mm]"
#token PACK_CLUSTER_NETS         "[Pp][Aa][Cc][Kk][_]{[Aa][Aa][Pp][Aa][Cc][Kk][_]}[Cc][Ll][Uu][Ss][Tt][Ee][Rr][_][Nn][Ee][Tt][Ss]"
#token PACK_AREA_WEIGHT          "[Pp][Aa][Cc][Kk][_]{[Aa][Aa][Pp][Aa][Cc][Kk][_]}([Aa][Rr][Ee][Aa]|[Aa][Ll][Pp][Hh][Aa])[_][Ww][Ee][Ii][Gg][Hh][Tt]"
#token PACK_NETS_WEIGHT          "[Pp][Aa][Cc][Kk][_]{[Aa][Aa][Pp][Aa][Cc][Kk][_]}([Nn][Ee][Tt][Ss]|[Bb][Ee][Tt][Aa])[_][Ww][Ee][Ii][Gg][Hh][Tt]"
#token PACK_AFFINITY_MODE        "[Pp][Aa][Cc][Kk][_]{[Aa][Aa][Pp][Aa][Cc][Kk][_]}[Aa][Ff][Ff][Ii][Nn][Ii][Tt][Yy]"
#token PACK_BLOCK_SIZE           "[Pp][Aa][Cc][Kk][_](([Bb][Ll][Oo][Cc][Kk]|[Cc][Ll][Bb])[_][Ss][Ii][Zz][Ee]|[N])"
#token PACK_LUT_SIZE             "[Pp][Aa][Cc][Kk][_]([Ll][Uu][Tt][_][Ss][Ii][Zz][Ee]|[K])"
#token PACK_COST_MODE            "[Pp][Aa][Cc][Kk][_][Cc][Oo][Ss][Tt][_][Mm][Oo][Dd][Ee]"

#token PLACE_ALGORITHM           "[Pp][Ll][Aa][Cc][Ee][_][Aa][Ll][Gg][Oo][Rr][Ii][Tt][Hh][Mm]"
#token PLACE_CHANNEL_WIDTH       "[Pp][Ll][Aa][Cc][Ee][_][Cc][Hh][Aa][Nn][Nn][Ee][Ll][_][Ww][Ii][Dd][Tt][Hh]"
#token PLACE_RANDOM_SEED         "[Pp][Ll][Aa][Cc][Ee][_][Rr][Aa][Nn][Dd][Oo][MM][_][Ss][Ee][Ee][Dd]"
#token PLACE_TEMP_INIT           "[Pp][Ll][Aa][Cc][Ee][_][Tt][Ee][Mm][Pp]{[Ee][Rr][Aa][Tt][Uu][Rr][Ee]}[_][Ii][Nn][Ii][Tt]"
#token PLACE_TEMP_INIT_FACTOR    "[Pp][Ll][Aa][Cc][Ee][_][Tt][Ee][Mm][Pp]{[Ee][Rr][Aa][Tt][Uu][Rr][Ee]}[_][Ii][Nn][Ii][Tt][_][Ff][Aa][Cc][Tt][Oo][Rr]"
#token PLACE_TEMP_INIT_EPSILON   "[Pp][Ll][Aa][Cc][Ee][_][Tt][Ee][Mm][Pp]{[Ee][Rr][Aa][Tt][Uu][Rr][Ee]}[_][Ii][Nn][Ii][Tt][_][Ee][Pp][Ss][Ii][Ll][Oo][Nn]"
#token PLACE_TEMP_EXIT           "[Pp][Ll][Aa][Cc][Ee][_][Tt][Ee][Mm][Pp]{[Ee][Rr][Aa][Tt][Uu][Rr][Ee]}[_][Ee][Xx][Ii][Tt]"
#token PLACE_TEMP_EXIT_FACTOR    "[Pp][Ll][Aa][Cc][Ee][_][Tt][Ee][Mm][Pp]{[Ee][Rr][Aa][Tt][Uu][Rr][Ee]}[_][Ee][Xx][Ii][Tt][_][Ff][Aa][Cc][Tt][Oo][Rr]"
#token PLACE_TEMP_EXIT_EPSILON   "[Pp][Ll][Aa][Cc][Ee][_][Tt][Ee][Mm][Pp]{[Ee][Rr][Aa][Tt][Uu][Rr][Ee]}[_][Ee][Xx][Ii][Tt][_][Ee][Pp][Ss][Ii][Ll][Oo][Nn]"
#token PLACE_TEMP_REDUCE         "[Pp][Ll][Aa][Cc][Ee][_][Tt][Ee][Mm][Pp]{[Ee][Rr][Aa][Tt][Uu][Rr][Ee]}[_][Rr][Ee][Dd][Uu][Cc][Ee]"
#token PLACE_TEMP_REDUCE_FACTOR  "[Pp][Ll][Aa][Cc][Ee][_][Tt][Ee][Mm][Pp]{[Ee][Rr][Aa][Tt][Uu][Rr][Ee]}[_][Rr][Ee][Dd][Uu][Cc][Ee][_][Ff][Aa][Cc][Tt][Oo][Rr]"
#token PLACE_TEMP_INNER_NUM      "[Pp][Ll][Aa][Cc][Ee][_]{[Tt][Ee][Mm][Pp]{[Ee][Rr][Aa][Tt][Uu][Rr][Ee]}[_]}[Ii][Nn][Nn][Ee][Rr][_][Nn][Uu][Mm]"
#token PLACE_SEARCH_LIMIT        "[Pp][Ll][Aa][Cc][Ee][_][Ss][Ee][Aa][Rr][Cc][Hh][_][Ll][Ii][Mm][Ii][Tt]{[_][Aa][Ll][Pp][Hh][Aa]}"
#token PLACE_COST_MODE           "[Pp][Ll][Aa][Cc][Ee][_][Cc][Oo][Ss][Tt][_][Mm][Oo][Dd][Ee]"
#token PLACE_TIMING_COST_FACTOR  "[Pp][Ll][Aa][Cc][Ee][_][Tt][Ii][Mm][Ii][Nn][Gg][_][Cc][Oo][Ss][Tt][_][Ff][Aa][Cc][Tt][Oo][Rr]"
#token PLACE_TIMING_UPDATE_INT   "[Pp][Ll][Aa][Cc][Ee][_][Tt][Ii][Mm][Ii][Nn][Gg][_][Uu][Pp][Dd][Aa][Tt][Ee][_][Ii][Nn][Tt]{[Ee][Rr][Vv][Aa][Ll]}"
#token PLACE_TIMING_UPDATE_COUNT "[Pp][Ll][Aa][Cc][Ee][_][Tt][Ii][Mm][Ii][Nn][Gg][_][Uu][Pp][Dd][Aa][Tt][Ee][_][Cc][Oo][Uu][Nn][Tt]"
#token PLACE_SLACK_INIT_WEIGHT   "[Pp][Ll][Aa][Cc][Ee][_]{[Tt][Ii][Mm][Ii][Nn][Gg][_]}[Ss][Ll][Aa][Cc][Kk][_][Ii][Nn][Ii][Tt][_][Ww][Ee][Ii][Gg][Hh][Tt]"
#token PLACE_SLACK_FINAL_WEIGHT  "[Pp][Ll][Aa][Cc][Ee][_]{[Tt][Ii][Mm][Ii][Nn][Gg][_]}[Ss][Ll][Aa][Cc][Kk][_][Ff][Ii][Nn][Aa][Ll][_][Ww][Ee][Ii][Gg][Hh][Tt]"

#token PLACE_RELATIVE_ENABLE     "[Pp][Ll][Aa][Cc][Ee][_][Rr][Ee][Ll][Aa][Tt][Ii][Vv][Ee][_][Ee][Nn][Aa][Bb][Ll][Ee]"
#token PLACE_RELATIVE_ROTATE     "[Pp][Ll][Aa][Cc][Ee][_][Rr][Ee][Ll][Aa][Tt][Ii][Vv][Ee][_][Rr][Oo][Tt][Aa][Tt][Ee]{[_][Ee][Nn][Aa][Bb][Ll][Ee]}"
#token PLACE_RELATIVE_INIT_PLACE "[Pp][Ll][Aa][Cc][Ee][_][Rr][Ee][Ll][Aa][Tt][Ii][Vv][Ee][_][Ii][Nn][Ii][Tt]{[Ii][Aa][Ll]}[_][Pp][Ll][Aa][Cc][Ee]{[_][Rr][Ee][Tt][Rr][Yy]}"
#token PLACE_RELATIVE_INIT_MACRO "[Pp][Ll][Aa][Cc][Ee][_][Rr][Ee][Ll][Aa][Tt][Ii][Vv][Ee][_][Ii][Nn][Ii][Tt]{[Ii][Aa][Ll]}[_][Mm][Aa][Cc][Rr][Oo]{[_][Rr][Ee][Tt][Rr][Yy]}"

#token PLACE_PREPLACED_ENABLE    "[Pp][Ll][Aa][Cc][Ee][_][Pp][Rr][Ee]{[_]}[Pp][Ll][Aa][Cc][Ee]{[Dd]|[Ss]}[_][Ee][Nn][Aa][Bb][Ll][Ee]"

#token ROUTE_ALGORITHM           "[Rr][Oo][Uu][Tt][Ee][_][Aa][Ll][Gg][Oo][Rr][Ii][Tt][Hh][Mm]"
#token ROUTE_TYPE                "[Rr][Oo][Uu][Tt][Ee][_][Tt][Yy][Pp][Ee]"
#token ROUTE_WINDOW_SIZE         "[Rr][Oo][Uu][Tt][Ee][_][Ww][Ii][Nn][Dd][Oo][Ww][_][Ss][Ii][Zz][Ee]"
#token ROUTE_CHANNEL_WIDTH       "[Rr][Oo][Uu][Tt][Ee][_][Cc][Hh][Aa][Nn][Nn][Ee][Ll][_][Ww][Ii][Dd][Tt][Hh]"
#token ROUTE_MAX_ITERATIONS      "[Rr][Oo][Uu][Tt][Ee][_][Mm][Aa][Xx][_][Ii][Tt][Ee][Rr][Aa][Tt][Ii][Oo][Nn][Ss]"
#token ROUTE_CONG_HIST_FACTOR    "[Rr][Oo][Uu][Tt][Ee][_][Cc][Oo][Nn][Gg]{[Ee][Ss][Tt][Ii][Oo][Nn]}[_][Hh][Ii][Ss][Tt]{[Oo][Rr][Ii][Cc][Aa][Ll]}[_][Ff][Aa][Cc][Tt][Oo][Rr]"
#token ROUTE_CONG_INIT_FACTOR    "[Rr][Oo][Uu][Tt][Ee][_][Cc][Oo][Nn][Gg]{[Ee][Ss][Tt][Ii][Oo][Nn]}[_][Ii][Nn][Ii][Tt][_][Ff][Aa][Cc][Tt][Oo][Rr]{[Ss]}"
#token ROUTE_CONG_PRESENT_FACTOR "[Rr][Oo][Uu][Tt][Ee][_][Cc][Oo][Nn][Gg]{[Ee][Ss][Tt][Ii][Oo][Nn]}[_][Pp][Rr][Ee][Ss][Ee][Nn][Tt][_]{[Gg][Rr][Oo][Ww][Tt][Hh][_]}[Ff][Aa][Cc][Tt][Oo][Rr]"
#token ROUTE_BEND_COST_FACTOR    "[Rr][Oo][Uu][Tt][Ee][_][Bb][Ee][Nn][Dd][_][Cc][Oo][Ss][Tt][_][Ff][Aa][Cc][Tt][Oo][Rr]"
#token ROUTE_RESOURCE_MODE       "[Rr][Oo][Uu][Tt][Ee][_][Rr][Ee][Ss][Oo][Uu][Rr][Cc][Ee][_]{[Cc][Oo][Ss][Tt][_]}[Mm][Oo][Dd][Ee]"
#token ROUTE_COST_MODE           "[Rr][Oo][Uu][Tt][Ee][_][Cc][Oo][Ss][Tt][_][Mm][Oo][Dd][Ee]"
#token ROUTE_TIMING_ASTAR_FACTOR "[Rr][Oo][Uu][Tt][Ee][_][Tt][Ii][Mm][Ii][Nn][Gg][_][Aa][Ss][Tt][Aa][Rr][_][Ff][Aa][Cc][Tt][Oo][Rr]"
#token ROUTE_TIMING_MAX_CRIT     "[Rr][Oo][Uu][Tt][Ee][_][Tt][Ii][Mm][Ii][Nn][Gg][_][Mm][Aa][Xx][_][Cc][Rr][Ii][Tt]{[Ii][Cc][Aa][Ll][Ii][Tt][Yy]}"
#token ROUTE_TIMING_SLACK_CRIT   "[Rr][Oo][Uu][Tt][Ee][_]{[Tt][Ii][Mm][Ii][Nn][Gg][_]}[Ss][Ll][Aa][Cc][Kk][_][Cc][Rr][Ii][Tt]{[Ii][Cc][Aa][Ll][Ii][Tt][Yy]}"

#token ROUTE_PREROUTED_ENABLE    "[Rr][Oo][Uu][Tt][Ee][_][Pp][Rr][Ee]{[_]}[Rr][Oo][Uu][Tt][Ee]{[Dd]|[Ss]}[_][Ee][Nn][Aa][Bb][Ll][Ee]"
#token ROUTE_PREROUTED_ORDER     "[Rr][Oo][Uu][Tt][Ee][_][Pp][Rr][Ee]{[_]}[Rr][Oo][Uu][Tt][Ee]{[Dd]|[Ss]}[_][Oo][Rr][Dd][Ee][Rr]"

#token NONE               "[Nn][Oo][Nn][Ee]"
#token ANY                "[Aa][Nn][Yy]"
#token ALL                "[Aa][Ll][Ll]"

#token FABRIC_VIEW        "[Ff][Aa][Bb][Rr][Ii][Cc][_][Vv][Ii][Ee][Ww]"
#token BOUNDING_BOX       "{[Bb][Oo][Uu][Nn][Dd][Ii][Nn][Gg][_]}[Bb][Oo][Xx]"
#token INTERNAL_GRID      "{[Ii][Nn][Tt][Ee][Rr][Nn][Aa][Ll][_]}[Gg][Rr][Ii][Dd]"

#token BLOCKS             "[Bb][Ll][Oo][Cc][Kk]{[Ss]}"
#token IOS                "[Ii][Oo]{[Ss]}"

#token PACK               "[Pp][Aa][Cc][Kk]"
#token PLACE              "[Pp][Ll][Aa][Cc][Ee]"
#token ROUTE              "[Rr][Oo][Uu][Tt][Ee]"

#token ELMORE             "[Ee][Ll][Mm][Oo][Rr][Ee]"
#token D2M                "[Dd][2][Mm]"

#token BY_NETS            "{[Bb][Yy][_]}[Nn][Ee][Tt]{[Ss]}"
#token BY_DELAYS          "{[Bb][Yy][_]}[Dd][Ee][Ll][Aa][Yy]{[Ss]}"

#token AAPACK             "[Aa][Aa][Pp][Aa][Cc][Kk]"
#token ANNEALING          "{[Ss][Ii][Mm][Uu][Ll][Aa][Tt][Ee][Dd][_]}[Aa][Nn][Nn][Ee][Aa][Ll][Ii][Nn][Gg]"
#token PATHFINDER         "[Pp][Aa][Tt][Hh][Ff][Ii][Nn][Dd][Ee][Rr]"

#token GLOBAL             "[Gg][Ll][Oo][Bb][Aa][Ll]"
#token DETAILED           "[Dd][Ee][Tt][Aa][Ii][Ll][Ee][Dd]"

#token MAX_INPUTS         "[Mm][Aa][Xx][_][Ii][Nn][Pp][Uu][Tt][Ss]"
#token MIN_CONNECTIONS    "[Mm][Ii][Nn][_][Cc][Oo][Nn][Nn][Ee][Cc][Tt][Ii][Oo][Nn][Ss]"
#token MAX_CONNECTIONS    "[Mm][Aa][Xx][_][Cc][Oo][Nn][Nn][Ee][Cc][Tt][Ii][Oo][Nn][Ss]"

#token ROUTABILITY_DRIVEN "[Rr][Oo][Uu][Tt][Aa][Bb][Ii][Ll][Ii][Tt][Yy]{[_][Dd][Rr][Ii][Vv][Ee][Nn]}"
#token TIMING_DRIVEN      "[Tt][Ii][Mm][Ii][Nn][Gg]{[_][Dd][Rr][Ii][Vv][Ee][Nn]}"
#token PATH_TIMING_DRIVEN "[Pp][Aa][Tt][Hh][_][Tt][Ii][Mm][Ii][Nn][Gg]{[_][Dd][Rr][Ii][Vv][Ee][Nn]}"
#token NET_TIMING_DRIVEN  "[Nn][Ee][Tt][_][Tt][Ii][Mm][Ii][Nn][Gg]{[_][Dd][Rr][Ii][Vv][Ee][Nn]}"
#token BREADTH_FIRST      "[Bb][Rr][Ee][Aa][Dd][Tt][Hh][_][Ff][Ii][Rr][Ss][Tt]"
#token DEMAND_ONLY        "[Dd][Ee][Mm][Aa][Nn][Dd][_][Oo][Nn][Ll][Yy]"
#token DELAY_NORMALIZED   "[Dd][Ee][Ll][Aa][Yy][_][Nn][Oo][Rr][Mm][Aa][Ll][Ii][Zz][Ee][Dd]"

#token FIRST              "[Ff][Ii][Rr][Ss][Tt]"
#token AUTO               "[Aa][Uu][Tt][Oo]"

#token BOOL_TRUE          "[Tt][Rr][Uu][Ee]"
#token BOOL_FALSE         "[Ff][Aa][Ll][Ss][Ee]"
#token BOOL_YES           "[Yy][Ee][Ss]"
#token BOOL_NO            "[Nn][Oo]"
#token BOOL_ON            "[Oo][Nn]"
#token BOOL_OFF           "[Oo][Ff][Ff]"

#token NEG_INT            "[\-][0-9]+"
#token POS_INT            "[0-9]+"
#token FLOAT              "{\-}{[0-9]+}.[0-9]+"
#token CHAR               "[a-zA-Z0-9_/\\\|\*\+\-]"
#token STRING             "[a-zA-Z_/\|][a-zA-Z0-9_/\|\(\)\[\]\.\+\~]*"

#tokclass OUTPUT_LAFF_VAL  { NONE FABRIC_VIEW BOUNDING_BOX INTERNAL_GRID ANY ALL }
#tokclass EXECUTE_TOOL_VAL { NONE PACK PLACE ROUTE ANY ALL }

#tokclass BOOL             { BOOL_TRUE BOOL_FALSE BOOL_YES BOOL_NO BOOL_ON BOOL_OFF }

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//

class TOP_OptionsParser_c
{
<<
public:

   void syn( ANTLRAbstractToken* /* pToken */, 
             ANTLRChar*          pszGroup,
             SetWordType*        /* pWordType */,
             ANTLRTokenType      tokenType,
             int                 /* k */ );

   void SetScanner( TOP_OptionsScanner_c* pscanner );
   void SetFileName( const char* pszFileName );
   void SetOptionsFile( TOP_OptionsFile_c* poptionsFile );
   void SetOptionsStore( TOS_OptionsStore_c* poptionsStore );
>> 
<<
private:

   int FindOutputLaffMask_( ANTLRTokenType tokenType );
   int FindExecuteToolMask_( ANTLRTokenType tokenType );

   bool FindBool_( ANTLRTokenType tokenType );
>> 
<<
private:

   TOP_OptionsScanner_c* pscanner_;
   string                srFileName_;
   TOP_OptionsFile_c*    poptionsFile_;

   TOS_InputOptions_c*   pinputOptions_;
   TOS_OutputOptions_c*  poutputOptions_;
   TOS_MessageOptions_c* pmessageOptions_;
   TOS_ExecuteOptions_c* pexecuteOptions_;
   TOS_FabricOptions_c*  pfabricOptions_;
   TOS_PackOptions_c*    ppackOptions_;
   TOS_PlaceOptions_c*   pplaceOptions_;
   TOS_RouteOptions_c*   prouteOptions_;

   string srActiveCmd_;
>>

//===========================================================================//
start 
   : 
   << 
      this->srActiveCmd_ = "";
   >>
   (  inputOptions
   |  outputOptions
   |  messageOptions
   |  executeOptions
   |  fabricOptions
   |  packOptions
   |  placeOptions
   |  routeOptions
   )* END_OF_FILE
   ;

//===========================================================================//
inputOptions
   : 
   <<
      this->srActiveCmd_ = ( LT( 1 ) ? LT( 1 )->getText( ) : "" );
   >>
      INPUT_FILE_XML { EQUAL } stringText[ &pinputOptions_->srXmlFileName ]
      <<
         pinputOptions_->xmlFileEnable = true;
      >>
   |  INPUT_FILE_BLIF { EQUAL } stringText[ &pinputOptions_->srBlifFileName ]
      <<
         pinputOptions_->blifFileEnable = true;
      >>
   |  INPUT_FILE_ARCH { EQUAL } stringText[ &pinputOptions_->srArchitectureFileName ]
      <<
         pinputOptions_->architectureFileEnable = true;
      >>
   |  INPUT_FILE_FABRIC { EQUAL } stringText[ &pinputOptions_->srFabricFileName ]
      <<
         pinputOptions_->fabricFileEnable = true;
      >>
   |  INPUT_FILE_CIRCUIT { EQUAL } stringText[ &pinputOptions_->srCircuitFileName ]
      <<
         pinputOptions_->circuitFileEnable = true;
      >>

   |  INPUT_ENABLE_XML { EQUAL } boolType[ &pinputOptions_->xmlFileEnable ]
   |  INPUT_ENABLE_BLIF { EQUAL } boolType[ &pinputOptions_->blifFileEnable ]
   |  INPUT_ENABLE_ARCH { EQUAL } boolType[ &pinputOptions_->architectureFileEnable ]
   |  INPUT_ENABLE_FABRIC { EQUAL } boolType[ &pinputOptions_->fabricFileEnable ]
   |  INPUT_ENABLE_CIRCUIT { EQUAL } boolType[ &pinputOptions_->circuitFileEnable ]
   <<
      this->srActiveCmd_ = "";
   >>
   ;

//===========================================================================//
outputOptions
   : 
   <<
      this->srActiveCmd_ = ( LT( 1 ) ? LT( 1 )->getText( ) : "" );
   >>
      OUTPUT_FILE_LOG { EQUAL } stringText[ &poutputOptions_->srLogFileName ]
   |  OUTPUT_FILE_OPTIONS { EQUAL } stringText[ &poutputOptions_->srOptionsFileName ]
   |  OUTPUT_FILE_XML { EQUAL } stringText[ &poutputOptions_->srXmlFileName ]
   |  OUTPUT_FILE_BLIF { EQUAL } stringText[ &poutputOptions_->srBlifFileName ]
   |  OUTPUT_FILE_ARCH { EQUAL } stringText[ &poutputOptions_->srArchitectureFileName ]
   |  OUTPUT_FILE_FABRIC { EQUAL } stringText[ &poutputOptions_->srFabricFileName ]
   |  OUTPUT_FILE_CIRCUIT { EQUAL } stringText[ &poutputOptions_->srCircuitFileName ]
   |  OUTPUT_FILE_RC_DELAYS { EQUAL } stringText[ &poutputOptions_->srRcDelaysFileName ]
   |  OUTPUT_FILE_LAFF { EQUAL } stringText[ &poutputOptions_->srLaffFileName ]

   |  OUTPUT_EMAIL_METRICS { EQUAL } stringText[ &poutputOptions_->srMetricsEmailAddress ]

   |  OUTPUT_ENABLE_LOG { EQUAL } boolType[ &poutputOptions_->logFileEnable ]
   |  OUTPUT_ENABLE_OPTIONS { EQUAL } boolType[ &poutputOptions_->optionsFileEnable ]
   |  OUTPUT_ENABLE_XML { EQUAL } boolType[ &poutputOptions_->xmlFileEnable ]
   |  OUTPUT_ENABLE_BLIF { EQUAL } boolType[ &poutputOptions_->blifFileEnable ]
   |  OUTPUT_ENABLE_ARCH { EQUAL } boolType[ &poutputOptions_->architectureFileEnable ]
   |  OUTPUT_ENABLE_FABRIC { EQUAL } boolType[ &poutputOptions_->fabricFileEnable ]
   |  OUTPUT_ENABLE_CIRCUIT { EQUAL } boolType[ &poutputOptions_->circuitFileEnable ]
   |  OUTPUT_ENABLE_RC_DELAYS { EQUAL } boolType[ &poutputOptions_->rcDelaysFileEnable ]
   |  OUTPUT_ENABLE_LAFF { EQUAL } boolType[ &poutputOptions_->laffFileEnable ]

   |  OUTPUT_LAFF_MODE { EQUAL } outputLaffMask[ &poutputOptions_->laffMask ]

   |  OUTPUT_RC_DELAYS_MODE { EQUAL } rcDelaysExtractMode[ &poutputOptions_->rcDelaysExtractMode ]
   |  OUTPUT_RC_DELAYS_SORT { EQUAL } rcDelaysSortMode[ &poutputOptions_->rcDelaysSortMode ]
   |  OUTPUT_RC_DELAYS_NETS { EQUAL } rcDelaysNameList[ &poutputOptions_->rcDelaysNetNameList ]
   |  OUTPUT_RC_DELAYS_BLOCKS { EQUAL } rcDelaysNameList[ &poutputOptions_->rcDelaysBlockNameList ]
   |  OUTPUT_RC_DELAYS_LENGTH { EQUAL } floatNum[ &poutputOptions_->rcDelaysMaxWireLength ]
   <<
      this->srActiveCmd_ = "";
   >>
   ;

//===========================================================================//
messageOptions
   : 
   <<
      string srEchoName;
      string srFileName;

      this->srActiveCmd_ = ( LT( 1 ) ? LT( 1 )->getText( ) : "" );
   >>
      FORMAT_MIN_GRID { EQUAL } floatNum[ &pmessageOptions_->minGridPrecision ]
   |  FORMAT_TIME_STAMPS { EQUAL } boolType[ &pmessageOptions_->timeStampsEnable ]

   |  DISPLAY_INFO_ACCEPT { EQUAL } displayNameList[ &pmessageOptions_->info.acceptList ]
   |  DISPLAY_INFO_REJECT { EQUAL } displayNameList[ &pmessageOptions_->info.rejectList ]
   |  DISPLAY_WARNING_ACCEPT { EQUAL } displayNameList[ &pmessageOptions_->warning.acceptList ]
   |  DISPLAY_WARNING_REJECT { EQUAL } displayNameList[ &pmessageOptions_->warning.rejectList ]
   |  DISPLAY_ERROR_ACCEPT { EQUAL } displayNameList[ &pmessageOptions_->error.acceptList ]
   |  DISPLAY_ERROR_REJECT { EQUAL } displayNameList[ &pmessageOptions_->error.rejectList ]
   |  DISPLAY_TRACE_ACCEPT { EQUAL } displayNameList[ &pmessageOptions_->trace.acceptList ]
   |  DISPLAY_TRACE_REJECT { EQUAL } displayNameList[ &pmessageOptions_->trace.rejectList ]

   |  TRACE_READ_OPTIONS { EQUAL } boolType[ &pmessageOptions_->trace.read.options ]
   |  TRACE_READ_XML { EQUAL } boolType[ &pmessageOptions_->trace.read.xml ]
   |  TRACE_READ_BLIF { EQUAL } boolType[ &pmessageOptions_->trace.read.blif ]
   |  TRACE_READ_ARCH { EQUAL } boolType[ &pmessageOptions_->trace.read.architecture ]
   |  TRACE_READ_FABRIC { EQUAL } boolType[ &pmessageOptions_->trace.read.fabric ]
   |  TRACE_READ_CIRCUIT { EQUAL } boolType[ &pmessageOptions_->trace.read.circuit ]

   |  TRACE_VPR_SHOW_SETUP { EQUAL } boolType[ &pmessageOptions_->trace.vpr.showSetup ]
   |  TRACE_VPR_ECHO_FILE { EQUAL } 
      (  boolType[ &pmessageOptions_->trace.vpr.echoFile ]
      |  stringText[ &srEchoName ] { stringText[ &srFileName ] }
         <<
            TC_NameFile_c echoFileName( srEchoName, srFileName );
            pmessageOptions_->trace.vpr.echoFileNameList.Add( echoFileName );
         >>
      )
   <<
      this->srActiveCmd_ = "";
   >>
   ;

//===========================================================================//
executeOptions
   : 
   <<
      this->srActiveCmd_ = "";
   >>
      HALT_MAX_WARNINGS { EQUAL } ulongNum[ &pexecuteOptions_->maxWarningCount  ]
   |  HALT_MAX_ERRORS { EQUAL } ulongNum[ &pexecuteOptions_->maxErrorCount ]

   |  EXECUTE_MODE { EQUAL } executeToolMask[ &pexecuteOptions_->toolMask ]
   <<
      this->srActiveCmd_ = ( LT( 1 ) ? LT( 1 )->getText( ) : "" );
   >>
   ;

//===========================================================================//
fabricOptions
   : 
   <<
      this->srActiveCmd_ = "";
   >>
      FABRIC_BLOCK_ENABLE { EQUAL } boolType[ &pfabricOptions_->blocks.override ]
   |  FABRIC_SWITCHBOX_ENABLE { EQUAL } boolType[ &pfabricOptions_->switchBoxes.override ]
   |  FABRIC_CONNECTIONBLOCK_ENABLE { EQUAL } boolType[ &pfabricOptions_->connectionBlocks.override ]
   |  FABRIC_CHANNEL_ENABLE { EQUAL } boolType[ &pfabricOptions_->channels.override ]
   <<
      this->srActiveCmd_ = ( LT( 1 ) ? LT( 1 )->getText( ) : "" );
   >>
   ;

//===========================================================================//
packOptions
   : 
   <<
      this->srActiveCmd_ = "";
   >>
      PACK_ALGORITHM { EQUAL } packAlgorithmMode[ &ppackOptions_->algorithmMode ]

   |  PACK_CLUSTER_NETS { EQUAL } packClusterNetsMode[ &ppackOptions_->clusterNetsMode ]
   |  PACK_AREA_WEIGHT { EQUAL } floatNum[ &ppackOptions_->areaWeight ]
   |  PACK_NETS_WEIGHT { EQUAL } floatNum[ &ppackOptions_->netsWeight ]
   |  PACK_AFFINITY_MODE { EQUAL } packAffinityMode[ &ppackOptions_->affinityMode ]
   |  PACK_BLOCK_SIZE { EQUAL } uintNum[ &ppackOptions_->blockSize ]
   |  PACK_LUT_SIZE { EQUAL } uintNum[ &ppackOptions_->lutSize ]

   |  PACK_COST_MODE { EQUAL } packCostMode[ &ppackOptions_->costMode ]
   <<
      this->srActiveCmd_ = ( LT( 1 ) ? LT( 1 )->getText( ) : "" );
   >>
   ;

//===========================================================================//
placeOptions
   : 
   <<
      this->srActiveCmd_ = "";
   >>
      PLACE_ALGORITHM { EQUAL } placeAlgorithmMode[ &pplaceOptions_->algorithmMode ]

   |  PLACE_CHANNEL_WIDTH { EQUAL } uintNum[ &pplaceOptions_->channelWidth ]

   |  PLACE_RANDOM_SEED { EQUAL } uintNum[ &pplaceOptions_->randomSeed ]
   |  PLACE_TEMP_INIT { EQUAL } floatNum[ &pplaceOptions_->initTemp ]
   |  PLACE_TEMP_INIT_FACTOR { EQUAL } floatNum[ &pplaceOptions_->initTempFactor ]
   |  PLACE_TEMP_INIT_EPSILON { EQUAL } floatNum[ &pplaceOptions_->initTempEpsilon ]
   |  PLACE_TEMP_EXIT { EQUAL } floatNum[ &pplaceOptions_->exitTemp ]
   |  PLACE_TEMP_EXIT_FACTOR { EQUAL } floatNum[ &pplaceOptions_->exitTempFactor ]
   |  PLACE_TEMP_EXIT_EPSILON { EQUAL } floatNum[ &pplaceOptions_->exitTempEpsilon ]
   |  PLACE_TEMP_REDUCE { EQUAL } floatNum[ &pplaceOptions_->reduceTemp ]
   |  PLACE_TEMP_REDUCE_FACTOR { EQUAL } floatNum[ &pplaceOptions_->reduceTempFactor ]
   |  PLACE_TEMP_INNER_NUM { EQUAL } floatNum[ &pplaceOptions_->innerNum ]
   |  PLACE_SEARCH_LIMIT { EQUAL } floatNum[ &pplaceOptions_->searchLimit ]

   |  PLACE_COST_MODE { EQUAL } placeCostMode[ &pplaceOptions_->costMode ]

   |  PLACE_TIMING_COST_FACTOR { EQUAL } floatNum[ &pplaceOptions_->timingCostFactor ]
   |  PLACE_TIMING_UPDATE_INT { EQUAL } uintNum[ &pplaceOptions_->timingUpdateInt ]
   |  PLACE_TIMING_UPDATE_COUNT { EQUAL } uintNum[ &pplaceOptions_->timingUpdateCount ]
   |  PLACE_SLACK_INIT_WEIGHT { EQUAL } floatNum[ &pplaceOptions_->slackInitWeight ]
   |  PLACE_SLACK_FINAL_WEIGHT { EQUAL } floatNum[ &pplaceOptions_->slackFinalWeight ]

   |  PLACE_RELATIVE_ENABLE { EQUAL } boolType[ &pplaceOptions_->relativePlace.enable ]
   |  PLACE_RELATIVE_ROTATE { EQUAL } boolType[ &pplaceOptions_->relativePlace.rotateEnable ]
   |  PLACE_RELATIVE_INIT_PLACE { EQUAL } uintNum[ &pplaceOptions_->relativePlace.maxPlaceRetryCt ]
   |  PLACE_RELATIVE_INIT_MACRO { EQUAL } uintNum[ &pplaceOptions_->relativePlace.maxMacroRetryCt ]

   |  PLACE_PREPLACED_ENABLE { EQUAL } boolType[ &pplaceOptions_->prePlaced.enable ]
   <<
      this->srActiveCmd_ = ( LT( 1 ) ? LT( 1 )->getText( ) : "" );
   >>
   ;

//===========================================================================//
routeOptions
   : 
   <<
      this->srActiveCmd_ = "";
   >>
      ROUTE_ALGORITHM { EQUAL } routeAlgorithmMode[ &prouteOptions_->algorithmMode ]

   |  ROUTE_TYPE { EQUAL } routeAbstractMode[ &prouteOptions_->abstractMode ]
   |  ROUTE_WINDOW_SIZE { EQUAL } uintNum[ &prouteOptions_->windowSize ]
   |  ROUTE_CHANNEL_WIDTH { EQUAL } uintNum[ &prouteOptions_->channelWidth ]
   |  ROUTE_MAX_ITERATIONS { EQUAL } uintNum[ &prouteOptions_->maxIterations ]

   |  ROUTE_CONG_HIST_FACTOR { EQUAL } floatNum[ &prouteOptions_->histCongestionFactor ]
   |  ROUTE_CONG_INIT_FACTOR { EQUAL } floatNum[ &prouteOptions_->initCongestionFactor ]
   |  ROUTE_CONG_PRESENT_FACTOR { EQUAL } floatNum[ &prouteOptions_->presentCongestionFactor ]
   |  ROUTE_BEND_COST_FACTOR { EQUAL } floatNum[ &prouteOptions_->bendCostFactor ]

   |  ROUTE_RESOURCE_MODE { EQUAL } routeResourceMode[ &prouteOptions_->resourceMode ]
   |  ROUTE_COST_MODE { EQUAL } routeCostMode[ &prouteOptions_->costMode ]

   |  ROUTE_TIMING_ASTAR_FACTOR { EQUAL } floatNum[ &prouteOptions_->timingAStarFactor ]
   |  ROUTE_TIMING_MAX_CRIT { EQUAL } floatNum[ &prouteOptions_->timingMaxCriticality ]
   |  ROUTE_TIMING_SLACK_CRIT { EQUAL } floatNum[ &prouteOptions_->slackCriticality ]

   |  ROUTE_PREROUTED_ENABLE { EQUAL } boolType[ &prouteOptions_->preRouted.enable ]
   |  ROUTE_PREROUTED_ORDER { EQUAL } routeOrderMode[ &prouteOptions_->preRouted.orderMode ]
   <<
      this->srActiveCmd_ = ( LT( 1 ) ? LT( 1 )->getText( ) : "" );
   >>
   ;

//===========================================================================//
outputLaffMask[ int* pmask ]
   : 
   <<
      *pmask = TOS_OUTPUT_LAFF_NONE;
   >>
   OPEN_BRACE
   (  maskListVal:OUTPUT_LAFF_VAL
      << 
         *pmask |= this->FindOutputLaffMask_( maskListVal->getType( ));
      >>
   )+
   CLOSE_BRACE
   |  maskVal:OUTPUT_LAFF_VAL
      << 
         *pmask = this->FindOutputLaffMask_( maskVal->getType( ));
      >>
   ;

//===========================================================================//
rcDelaysExtractMode[ TOS_RcDelaysExtractMode_t* pmode ]
   :
      ELMORE
      <<
         *pmode = TOS_RC_DELAYS_EXTRACT_ELMORE;
      >>
   |  D2M
      <<
         *pmode = TOS_RC_DELAYS_EXTRACT_D2M;
      >>
   ;

//===========================================================================//
rcDelaysSortMode[ TOS_RcDelaysSortMode_t* pmode ]
   :
      BY_NETS
      <<
         *pmode = TOS_RC_DELAYS_SORT_BY_NETS;
      >>
   |  BY_DELAYS
      <<
         *pmode = TOS_RC_DELAYS_SORT_BY_DELAYS;
      >>
   ;

//===========================================================================//
rcDelaysNameList[ TOS_RcDelaysNameList_t* pnameList ]
   : 
   <<
      string srString;
   >>
   OPEN_BRACE
   (  stringText[ &srString ]
      <<
         pnameList->Add( srString );
      >>
   )*
   CLOSE_BRACE
   |  <<
         string srString_;
      >>
      stringText[ &srString_ ]
      <<
         pnameList->Add( srString_ );
      >>
   ;

//===========================================================================//
displayNameList[ TOS_DisplayNameList_t* pnameList ]
   : 
   <<
      string srString;
   >>
   OPEN_BRACE
   (  stringText[ &srString ] 
      <<
         TC_Name_c name( srString );
         pnameList->Add( name );
      >>
   )*
   CLOSE_BRACE
   |  <<
         string srString_;
      >>
      stringText[ &srString_ ] 
      <<
         TC_Name_c name( srString_ );
         pnameList->Add( name );
      >>
   ;

//===========================================================================//
executeToolMask[ int* pmask ]
   : 
   <<
      *pmask = TOS_EXECUTE_TOOL_NONE;
   >>
   OPEN_BRACE
   (  maskListVal:EXECUTE_TOOL_VAL
      << 
         *pmask |= this->FindExecuteToolMask_( maskListVal->getType( ));
      >>
   )+
   CLOSE_BRACE
   |  maskVal:EXECUTE_TOOL_VAL
      << 
         *pmask = this->FindExecuteToolMask_( maskVal->getType( ));
      >>
   ;

//===========================================================================//
packAlgorithmMode[ TOS_PackAlgorithmMode_t* pmode ]
   :
      AAPACK
      <<
         *pmode = TOS_PACK_ALGORITHM_AAPACK;
      >>
   ;

//===========================================================================//
packClusterNetsMode[ TOS_PackClusterNetsMode_t* pmode ]
   :
      MIN_CONNECTIONS
      <<
         *pmode = TOS_PACK_CLUSTER_NETS_MIN_CONNECTIONS;
      >>
   |  MAX_CONNECTIONS
      <<
         *pmode = TOS_PACK_CLUSTER_NETS_MAX_CONNECTIONS;
      >>
   ;

//===========================================================================//
packAffinityMode[ TOS_PackAffinityMode_t* pmode ]
   :
      NONE
      <<
         *pmode = TOS_PACK_AFFINITY_NONE;
      >>
   |  ANY
      <<
         *pmode = TOS_PACK_AFFINITY_ANY;
      >>
   ;

//===========================================================================//
packCostMode[ TOS_PackCostMode_t* pmode ]
   :
      ROUTABILITY_DRIVEN
      <<
         *pmode = TOS_PACK_COST_ROUTABILITY_DRIVEN;
      >>
   |  TIMING_DRIVEN
      <<
         *pmode = TOS_PACK_COST_TIMING_DRIVEN;
      >>
   ;

//===========================================================================//
placeAlgorithmMode[ TOS_PlaceAlgorithmMode_t* pmode ]
   :
      ANNEALING
      <<
         *pmode = TOS_PLACE_ALGORITHM_ANNEALING;
      >>
   ;

//===========================================================================//
placeCostMode[ TOS_PlaceCostMode_t* pmode ]
   :
      ROUTABILITY_DRIVEN
      <<
         *pmode = TOS_PLACE_COST_ROUTABILITY_DRIVEN;
      >>
   |  TIMING_DRIVEN
      <<
         *pmode = TOS_PLACE_COST_PATH_TIMING_DRIVEN;
      >>
   |  PATH_TIMING_DRIVEN
      <<
         *pmode = TOS_PLACE_COST_PATH_TIMING_DRIVEN;
      >>
   |  NET_TIMING_DRIVEN
      <<
         *pmode = TOS_PLACE_COST_NET_TIMING_DRIVEN;
      >>
   ;

//===========================================================================//
routeAlgorithmMode[ TOS_RouteAlgorithmMode_t* pmode ]
   :
      PATHFINDER
      <<
         *pmode = TOS_ROUTE_ALGORITHM_PATHFINDER;
      >>
   ;

//===========================================================================//
routeAbstractMode[ TOS_RouteAbstractMode_t* pmode ]
   :
      GLOBAL
      <<
         *pmode = TOS_ROUTE_ABSTRACT_GLOBAL;
      >>
   |  DETAILED
      <<
         *pmode = TOS_ROUTE_ABSTRACT_DETAILED;
      >>
   ;

//===========================================================================//
routeResourceMode[ TOS_RouteResourceMode_t* pmode ]
   :
      DEMAND_ONLY
      <<
         *pmode = TOS_ROUTE_RESOURCE_DEMAND_ONLY;
      >>
   |  DELAY_NORMALIZED
      <<
         *pmode = TOS_ROUTE_RESOURCE_DELAY_NORMALIZED;
      >>
   ;

//===========================================================================//
routeCostMode[ TOS_RouteCostMode_t* pmode ]
   :
      BREADTH_FIRST
      <<
         *pmode = TOS_ROUTE_COST_BREADTH_FIRST;
      >>
   |  TIMING_DRIVEN
      <<
         *pmode = TOS_ROUTE_COST_TIMING_DRIVEN;
      >>
   ;

//===========================================================================//
routeOrderMode[ TOS_RouteOrderMode_t* pmode ]
   :
      FIRST
      <<
         *pmode = TOS_ROUTE_ORDER_FIRST;
      >>
   |  AUTO
      <<
         *pmode = TOS_ROUTE_ORDER_AUTO;
      >>
   ;

//===========================================================================//
name[ TC_Name_c& srName ]
   : 
   <<
      srName = "";
   >>
      OPEN_QUOTE
      {  qnameVal:STRING 
         <<
            srName = qnameVal->getText( );
         >>
      } 
      CLOSE_QUOTE
   |  nameVal:STRING
      <<
         srName = nameVal->getText( );
      >>
   ;

//===========================================================================//
stringText[ string* psrString ]
   : 
   <<
      *psrString = "";
   >>
      OPEN_QUOTE
      {  qstringVal:STRING 
         <<
            *psrString = qstringVal->getText( );
         >>
      } 
      CLOSE_QUOTE
   |  stringVal:STRING
      <<
         *psrString = stringVal->getText( );
      >>
   ;

//===========================================================================//
floatNum[ double* pdouble ]
   : 
      fVal:FLOAT
      <<
         *pdouble = atof( fVal->getText( ));
      >>
   |  posIntVal:POS_INT
      <<
         *pdouble = atof( posIntVal->getText( ));
      >>
   |  negIntVal:NEG_INT
      <<
         *pdouble = atof( negIntVal->getText( ));
      >>
   ;

//===========================================================================//
intNum[ int* pint ]
   : 
      posIntVal:POS_INT
      <<
         *pint = atoi( posIntVal->getText( ));
      >>
   |  negIntVal:NEG_INT
      <<
         *pint = atoi( negIntVal->getText( ));
      >>
   ;

//===========================================================================//
uintNum[ unsigned int* puint ]
   : 
      uintVal:POS_INT
      <<
         *puint = static_cast< unsigned int >( atol( uintVal->getText( )));
      >>
   ;

//===========================================================================//
ulongNum[ unsigned long* plong ]
   : 
      ulongVal:POS_INT
      <<
         *plong = atol( ulongVal->getText( ));
      >>
   |  NEG_INT
      <<
         *plong = ULONG_MAX;
      >>
   ;

//===========================================================================//
boolType[ bool* pbool ]
   : 
      boolVal:BOOL
      <<
         *pbool = this->FindBool_( boolVal->getType( ));
      >>
   ;

//===========================================================================//

}

<<
#include "TOP_OptionsGrammar.h"
>>
