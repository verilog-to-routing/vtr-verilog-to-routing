%{
/*
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "odin_error.h"
#include "odin_types.h"
#include "odin_util.h"
#include "odin_globals.h"
#include "verilog_bison.h"
#include "vtr_util.h"
#include "scope_util.h"

#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>
#include <string.h>

#define RECURSIVE_LIMIT 256

#define YY_USER_ACTION {                                           \
    my_location.col = current_yycolumn; current_yycolumn = yyleng; \
    }

#define TOP_STATE() top_flex_state("")

#define POP_STATE() {                    \
    flex_state.pop_back();               \
    BEGIN(top_flex_state("Popped to ")); \
    }

#define PUSH_STATE(state)    {            \
    flex_state.push_back(state);         \
    BEGIN(top_flex_state("Pushed to ")); \
    }

#define CHANGE_STATE(state) {              \
    flex_state.pop_back();                 \
    flex_state.push_back(state);           \
    BEGIN(top_flex_state("Switched to ")); \
    }

struct defines_t 
{
    std::string name;
    bool use_va_args;
    std::vector<std::string> args;
    std::string body;
};

extern loc_t my_location;

int current_yycolumn = 1;
std::unordered_map<std::string, defines_t*> defines_map;
defines_t *current_define = NULL;
std::vector<int> current_include_stack;
std::vector<std::string> current_args;
std::string current_define_body;

std::vector<int> flex_state = {};
std::vector<int> ieee_state = {};

void MP();

int top_flex_state(const char *str);
int top_ieee_state(const char *str);

void push_include(const char *file_name);
bool pop_include();
void pop_buffer_state();
void lex_string(const char * str);

bool ifdef(const char* id);
void new_define_t(const char* id);
void free_define_t(const char* id);
void free_define_map();
void add_args_to_define(const char* str);
void append_to_define_body(char c);
void finalize_define();

std::string get_simple_define(const char *str);
std::string get_complex_define();
void next_define_arg();
void define_arg_push_back(char c);
void load_define(const char *str);
void initialize_defaults();

int ieee_filter(int return_type);
%}

/* %x INITIAL  == 0 ** bison create initial by default and starts with it */
%x BRACKET_MATCH
%x COMMENT
%x MULTI_LINE_COMMENT
%x DEFINE_BODY
%x SKIP
%x ELIFCONDITION
%x NEG_CONDITION
%x CONDITION
%x DEFINE_REMOVAL
%x VAR_PARSE
%x PARSE_DEFINE
%x DEFINE_ARGS
%x INCLUDE 
%x READ_IEEE

%option noyywrap
%option nounput
%option noinput
%option never-interactive
%option nounistd
%option nodefault
%option case-sensitive


vBIN [Bb][_]*[ZzXx0-1][ZzXx0-1_]*
vOCT [Oo][_]*[ZzXx0-7][ZzXx0-7_]*
vDEC [Dd][_]*[[:digit:]][[:digit:]_]*
vHEX [Hh][_]*[ZzXx[:xdigit:]][ZzXx[:xdigit:]_]*
vINT [[:digit:]][[:digit:]_]*
vWORD [[:alpha:]_][[:alnum:]_$]*
defineWORD [[:alnum:]_$]+
vSTR ["]([^\\\"]|\\.)*["]  
vPUNCT [\?\:\|\^\&\<\>\-\*\/\%\(\)\{\}\[\]\~\!\;\#\,\.\@\=\+]

%%

    /*********************************
     * Preprocessor directives
     **/
<VAR_PARSE>{vWORD}                  { MP(); add_args_to_define(yytext); }
<VAR_PARSE>"..."                    { MP(); add_args_to_define(yytext); }
<VAR_PARSE>[\,]                     {  }
<VAR_PARSE>[\)]                     { CHANGE_STATE(DEFINE_BODY); }
<PARSE_DEFINE>{defineWORD}[\(]      { MP(); new_define_t(yytext); CHANGE_STATE(VAR_PARSE); }
<PARSE_DEFINE>{defineWORD}          { MP(); new_define_t(yytext); CHANGE_STATE(DEFINE_BODY); }
<INITIAL>"`define"                  { MP(); PUSH_STATE(PARSE_DEFINE); }

<DEFINE_REMOVAL>{vWORD}             { MP(); free_define_t(yytext); POP_STATE(); }
<INITIAL>"`undef"                   { MP(); PUSH_STATE(DEFINE_REMOVAL); }

<CONDITION>{vWORD}                  { MP(); CHANGE_STATE( ((ifdef(yytext))?  INITIAL: SKIP) ); }
<NEG_CONDITION>{vWORD}              { MP(); CHANGE_STATE( ((ifdef(yytext))?  SKIP: INITIAL) ); }

    /* since the condition did not hold true, we evaluate these statement */
<SKIP>"`elsif"                      { MP(); CHANGE_STATE(CONDITION); }
<SKIP>"`else"                       { MP(); CHANGE_STATE(INITIAL); }

    /* since the condition held true, we need to skip these */
<INITIAL>"`elsif"                   { MP(); CHANGE_STATE(SKIP); }
<INITIAL>"`else"                    { MP(); CHANGE_STATE(SKIP); }

    /* entry point */
<INITIAL>"`ifdef"                   { MP(); PUSH_STATE(CONDITION); }
<INITIAL>"`ifndef"                  { MP(); PUSH_STATE(NEG_CONDITION); }

    /* exit point */
<INITIAL,SKIP>"`endif"              { MP(); POP_STATE(); }

<INITIAL>"`include"                 { MP(); PUSH_STATE(INCLUDE); }
<INCLUDE>{vSTR}                     { MP(); push_include(yytext); POP_STATE(); }

<INITIAL>"`default_nettype"         { MP(); return preDEFAULT_NETTYPE;}
<INITIAL>"`resetall"                { MP(); initialize_defaults(); }

    /* unsupported commands, we skip the rest of the line */
<INITIAL>"`timescale"               { MP(); PUSH_STATE(COMMENT); }
<INITIAL>"`pragma"                  { MP(); PUSH_STATE(COMMENT); }
<INITIAL>"`line"                    { MP(); PUSH_STATE(COMMENT); }
<INITIAL>"`celldefine"              { MP(); PUSH_STATE(COMMENT); }
<INITIAL>"`endcelldefine"           { MP(); PUSH_STATE(COMMENT); }
<INITIAL>"`nounconnected_drive"     { MP(); PUSH_STATE(COMMENT); }
<INITIAL>"`unconnected_drive"       { MP(); PUSH_STATE(COMMENT); }


<INITIAL>"‘begin_keywords"          { MP(); PUSH_STATE(READ_IEEE); }
<READ_IEEE>"\"1364-2005\""          { MP(); ieee_state.push_back(ieee_2005);            POP_STATE(); }
<READ_IEEE>"\"1364-2001-noconfig\"" { MP(); ieee_state.push_back(ieee_2001_noconfig);    POP_STATE(); }
<READ_IEEE>"\"1364-2001\""          { MP(); ieee_state.push_back(ieee_2001);            POP_STATE(); }
<READ_IEEE>"\"1364-1995\""          { MP(); ieee_state.push_back(ieee_1995);            POP_STATE(); }
<INITIAL>"`end_keywords"            { MP(); ieee_state.pop_back(); }

<DEFINE_ARGS>[\)]                   { MP(); POP_STATE(); lex_string(get_complex_define().c_str()); }
<DEFINE_ARGS>[\,]                   { MP(); next_define_arg(); }
<BRACKET_MATCH>[\)]                 { MP(); POP_STATE(); define_arg_push_back(yytext[0]); }
<DEFINE_ARGS,BRACKET_MATCH>[\(]     { MP(); PUSH_STATE(BRACKET_MATCH); define_arg_push_back(yytext[0]); }
<INITIAL>[\`]{vWORD}[\(]            { MP(); load_define(yytext); PUSH_STATE(DEFINE_ARGS); }
<INITIAL>[\`]{vWORD}                { MP(); lex_string(get_simple_define(yytext).c_str()); }

    /**********************************
     * Verilog Keywords
     */

    /*    unsupported Keywords        */
<INITIAL>"cell"                      { MP(); return ieee_filter(vCELL); }
<INITIAL>"config"                    { MP(); return ieee_filter(vCONFIG); }
<INITIAL>"design"                    { MP(); return ieee_filter(vDESIGN); }
<INITIAL>"endconfig"                 { MP(); return ieee_filter(vENDCONFIG); }
<INITIAL>"incdir"                    { MP(); return ieee_filter(vINCDIR); }
<INITIAL>"include"                   { MP(); return ieee_filter(vINCLUDE); }
<INITIAL>"instance"                  { MP(); return ieee_filter(vINSTANCE); }
<INITIAL>"liblist"                   { MP(); return ieee_filter(vLIBLIST); }
<INITIAL>"library"                   { MP(); return ieee_filter(vLIBRARY); }
<INITIAL>"use"                       { MP(); return ieee_filter(vUSE); }
<INITIAL>"noshowcancelled"           { MP(); return ieee_filter(vNOSHOWCANCELLED); }
<INITIAL>"pulsestyle_ondetect"       { MP(); return ieee_filter(vPULSESTYLE_ONDETECT); }
<INITIAL>"pulsestyle_onevent"        { MP(); return ieee_filter(vPULSESTYLE_ONEVENT); }
<INITIAL>"showcancelled"             { MP(); return ieee_filter(vSHOWCANCELLED); }
<INITIAL>"casex"                     { MP(); return ieee_filter(vCASEX); }
<INITIAL>"casez"                     { MP(); return ieee_filter(vCASEZ); }
<INITIAL>"disable"                   { MP(); return ieee_filter(vDISABLE); }
<INITIAL>"edge"                      { MP(); return ieee_filter(vEDGE); }
<INITIAL>"scalared"                  { MP(); return ieee_filter(vSCALARED); }
<INITIAL>"bufif0"                    { MP(); return ieee_filter(vBUFIF0); }
<INITIAL>"bufif1"                    { MP(); return ieee_filter(vBUFIF1); }
<INITIAL>"cmos"                      { MP(); return ieee_filter(vCMOS); }
<INITIAL>"deassign"                  { MP(); return ieee_filter(vDEASSIGN); }
<INITIAL>"endprimitive"              { MP(); return ieee_filter(vENDPRIMITIVE); }
<INITIAL>"endtable"                  { MP(); return ieee_filter(vENDTABLE); }
<INITIAL>"event"                     { MP(); return ieee_filter(vEVENT); }
<INITIAL>"force"                     { MP(); return ieee_filter(vFORCE); }
<INITIAL>"forever"                   { MP(); return ieee_filter(vFOREVER); }
<INITIAL>"fork"                      { MP(); return ieee_filter(vFORK); }
<INITIAL>"highz0"                    { MP(); return ieee_filter(vHIGHZ0); }
<INITIAL>"highz1"                    { MP(); return ieee_filter(vHIGHZ1); }
<INITIAL>"join"                      { MP(); return ieee_filter(vJOIN); }
<INITIAL>"large"                     { MP(); return ieee_filter(vLARGE); }
<INITIAL>"medium"                    { MP(); return ieee_filter(vMEDIUM); }
<INITIAL>"nmos"                      { MP(); return ieee_filter(vNMOS); }
<INITIAL>"notif0"                    { MP(); return ieee_filter(vNOTIF0); }
<INITIAL>"notif1"                    { MP(); return ieee_filter(vNOTIF1); }
<INITIAL>"pmos"                      { MP(); return ieee_filter(vPMOS); }
<INITIAL>"primitive"                 { MP(); return ieee_filter(vPRIMITIVE); }
<INITIAL>"pull0"                     { MP(); return ieee_filter(vPULL0); }
<INITIAL>"pull1"                     { MP(); return ieee_filter(vPULL1); }
<INITIAL>"pulldown"                  { MP(); return ieee_filter(vPULLDOWN); }
<INITIAL>"pullup"                    { MP(); return ieee_filter(vPULLUP); }
<INITIAL>"rcmos"                     { MP(); return ieee_filter(vRCMOS); }
<INITIAL>"release"                   { MP(); return ieee_filter(vRELEASE); }
<INITIAL>"repeat"                    { MP(); return ieee_filter(vREPEAT); }
<INITIAL>"rnmos"                     { MP(); return ieee_filter(vRNMOS); }
<INITIAL>"rpmos"                     { MP(); return ieee_filter(vRPMOS); }
<INITIAL>"rtran"                     { MP(); return ieee_filter(vRTRAN); }
<INITIAL>"rtranif0"                  { MP(); return ieee_filter(vRTRANIF0); }
<INITIAL>"rtranif1"                  { MP(); return ieee_filter(vRTRANIF1); }
<INITIAL>"small"                     { MP(); return ieee_filter(vSMALL); }
<INITIAL>"strong0"                   { MP(); return ieee_filter(vSTRONG0); }
<INITIAL>"strong1"                   { MP(); return ieee_filter(vSTRONG1); }
<INITIAL>"supply0"                   { MP(); return ieee_filter(vSUPPLY0); }
<INITIAL>"supply1"                   { MP(); return ieee_filter(vSUPPLY1); }
<INITIAL>"table"                     { MP(); return ieee_filter(vTABLE); }
<INITIAL>"time"                      { MP(); return ieee_filter(vTIME); }
<INITIAL>"tran"                      { MP(); return ieee_filter(vTRAN); }
<INITIAL>"tranif0"                   { MP(); return ieee_filter(vTRANIF0); }
<INITIAL>"tranif1"                   { MP(); return ieee_filter(vTRANIF1); }
<INITIAL>"vectored"                  { MP(); return ieee_filter(vVECTORED); }
<INITIAL>"wait"                      { MP(); return ieee_filter(vWAIT); }
<INITIAL>"weak0"                     { MP(); return ieee_filter(vWEAK0); }
<INITIAL>"weak1"                     { MP(); return ieee_filter(vWEAK1); }

    /* Begin Scoped items */
<INITIAL>"begin"                     { MP(); push_scope(); return ieee_filter(vBEGIN); }
<INITIAL>"function"                  { MP(); push_scope(); return ieee_filter(vFUNCTION); }
<INITIAL>"module"                    { MP(); push_scope(); return ieee_filter(vMODULE); }
<INITIAL>"macromodule"               { MP(); push_scope(); return ieee_filter(vMODULE); }
<INITIAL>"task"                      { MP(); push_scope(); return ieee_filter(vTASK); }

    /* End Scoped items */
<INITIAL>"end"                       { MP(); return ieee_filter(vEND); }
<INITIAL>"endfunction"               { MP(); return ieee_filter(vENDFUNCTION); }
<INITIAL>"endmodule"                 { MP(); return ieee_filter(vENDMODULE); }
<INITIAL>"endtask"                   { MP(); return ieee_filter(vENDTASK); }

    /*    Keywords    */
<INITIAL>"always"                    { MP(); return ieee_filter(vALWAYS); }
<INITIAL>"and"                       { MP(); return ieee_filter(vAND); }
<INITIAL>"assign"                    { MP(); return ieee_filter(vASSIGN); }
<INITIAL>"automatic"                 { MP(); return ieee_filter(vAUTOMATIC); }
<INITIAL>"case"                      { MP(); return ieee_filter(vCASE); }
<INITIAL>"default"                   { MP(); return ieee_filter(vDEFAULT); }
<INITIAL>"defparam"                  { MP(); return ieee_filter(vDEFPARAM); }
<INITIAL>"else"                      { MP(); return ieee_filter(vELSE); }
<INITIAL>"endcase"                   { MP(); return ieee_filter(vENDCASE); }
<INITIAL>"endspecify"                { MP(); return ieee_filter(vENDSPECIFY); }
<INITIAL>"endgenerate"               { MP(); return ieee_filter(vENDGENERATE); }
<INITIAL>"for"                       { MP(); return ieee_filter(vFOR); }
<INITIAL>"if"                        { MP(); return ieee_filter(vIF); }
<INITIAL>"initial"                   { MP(); return ieee_filter(vINITIAL); }
<INITIAL>"inout"                     { MP(); return ieee_filter(vINOUT); }
<INITIAL>"input"                     { MP(); return ieee_filter(vINPUT); }
<INITIAL>"integer"                   { MP(); return ieee_filter(vINTEGER); }
<INITIAL>"generate"                  { MP(); return ieee_filter(vGENERATE); }
<INITIAL>"genvar"                    { MP(); return ieee_filter(vGENVAR); }
<INITIAL>"nand"                      { MP(); return ieee_filter(vNAND); }
<INITIAL>"negedge"                   { MP(); return ieee_filter(vNEGEDGE); }
<INITIAL>"nor"                       { MP(); return ieee_filter(vNOR); }
<INITIAL>"not"                       { MP(); return ieee_filter(vNOT); }
<INITIAL>"or"                        { MP(); return ieee_filter(vOR); }
<INITIAL>"output"                    { MP(); return ieee_filter(vOUTPUT); }
<INITIAL>"parameter"                 { MP(); return ieee_filter(vPARAMETER); }
<INITIAL>"localparam"                { MP(); return ieee_filter(vLOCALPARAM); }
<INITIAL>"posedge"                   { MP(); return ieee_filter(vPOSEDGE); }
<INITIAL>"signed"                    { MP(); return ieee_filter(vSIGNED); }
<INITIAL>"unsigned"                  { MP(); return ieee_filter(vUNSIGNED); }
<INITIAL>"specify"                   { MP(); return ieee_filter(vSPECIFY); }
<INITIAL>"while"                     { MP(); return ieee_filter(vWHILE); }
<INITIAL>"xnor"                      { MP(); return ieee_filter(vXNOR); }
<INITIAL>"xor"                       { MP(); return ieee_filter(vXOR); }
<INITIAL>"specparam"                 { MP(); return ieee_filter(vSPECPARAM); }
<INITIAL>"buf"                       { MP(); return ieee_filter(vBUF); }

    /* Net types */
<INITIAL>"wire"                      { MP(); return ieee_filter(vWIRE); }
<INITIAL>"tri"                       { MP(); return ieee_filter(vTRI); }
<INITIAL>"tri0"                      { MP(); return ieee_filter(vTRI0); }
<INITIAL>"tri1"                      { MP(); return ieee_filter(vTRI1); }
<INITIAL>"wand"                      { MP(); return ieee_filter(vWAND); }
<INITIAL>"wor"                       { MP(); return ieee_filter(vWOR); }
<INITIAL>"triand"                    { MP(); return ieee_filter(vTRIAND); }
<INITIAL>"trior"                     { MP(); return ieee_filter(vTRIOR); }
<INITIAL>"trireg"                    { MP(); return ieee_filter(vTRIREG); }
<INITIAL>"uwire"                     { MP(); return ieee_filter(vUWIRE); }
<INITIAL>"none"                      { MP(); return ieee_filter(vNONE); }
<INITIAL>"reg"                       { MP(); return ieee_filter(vREG); }

    /**********************************
     * Verilog Operators (vo)
     */
<INITIAL>"&&&"                       { MP(); return ieee_filter(voANDANDAND); }
<INITIAL>"**"                        { MP(); return ieee_filter(voPOWER); }
<INITIAL>"&&"                        { MP(); return ieee_filter(voANDAND); }
<INITIAL>"||"                        { MP(); return ieee_filter(voOROR); }
<INITIAL>"<="                        { MP(); return ieee_filter(voLTE); }
<INITIAL>"=>"                        { MP(); return ieee_filter(voEGT); }
<INITIAL>">="                        { MP(); return ieee_filter(voGTE); }
<INITIAL>"<<"                        { MP(); return ieee_filter(voSLEFT); }
<INITIAL>"<<<"                       { MP(); return ieee_filter(voASLEFT); }
<INITIAL>">>"                        { MP(); return ieee_filter(voSRIGHT); }
<INITIAL>">>>"                       { MP(); return ieee_filter(voASRIGHT); }
<INITIAL>"=="                        { MP(); return ieee_filter(voEQUAL); }
<INITIAL>"!="                        { MP(); return ieee_filter(voNOTEQUAL); }
<INITIAL>"==="                       { MP(); return ieee_filter(voCASEEQUAL); }
<INITIAL>"!=="                       { MP(); return ieee_filter(voCASENOTEQUAL); }
<INITIAL>"^~"                        { MP(); return ieee_filter(voXNOR); }
<INITIAL>"~^"                        { MP(); return ieee_filter(voXNOR); }
<INITIAL>"~&"                        { MP(); return ieee_filter(voNAND); }
<INITIAL>"~|"                        { MP(); return ieee_filter(voNOR); }
<INITIAL>"+:"                        { MP(); return ieee_filter(voPLUSCOLON); }
<INITIAL>"-:"                        { MP(); return ieee_filter(voMINUSCOLON); }

    /**********************************
     * Verilog System (vs) Functions
     */
<INITIAL>"$clog2"                    { MP(); return ieee_filter(vsCLOG2); }
<INITIAL>"$unsigned"                 { MP(); return ieee_filter(vsUNSIGNED); }
<INITIAL>"$signed"                   { MP(); return ieee_filter(vsSIGNED); }
<INITIAL>"$finish"                   { MP(); return ieee_filter(vsFINISH); }
<INITIAL>"$display"                  { MP(); return ieee_filter(vsDISPLAY); }
    /* catch all C functions */
<INITIAL>[\$]{vWORD}                 { MP(); return ieee_filter(vsFUNCTION); }

    /* Integers */
<INITIAL>{vINT}                      { MP(); yylval.num_value = vtr::strdup(yytext); return vINT_NUMBER; }
    /* Strings */
<INITIAL>{vSTR}                      { MP(); yylval.num_value = vtr::strdup(yytext); return vSTRING; }
    /* Numbers */
<INITIAL>[[:digit:]]*'[sS]?{vBIN}    { MP(); yylval.num_value = vtr::strdup(yytext); return vNUMBER; }
<INITIAL>[[:digit:]]*'[sS]?{vHEX}    { MP(); yylval.num_value = vtr::strdup(yytext); return vNUMBER; }
<INITIAL>[[:digit:]]*'[sS]?{vOCT}    { MP(); yylval.num_value = vtr::strdup(yytext); return vNUMBER; }
<INITIAL>[[:digit:]]*'[sS]?{vDEC}    { MP(); yylval.num_value = vtr::strdup(yytext); return vNUMBER; }

    /*    operands    */
<INITIAL>{vWORD}(\.{vWORD})*         { MP(); yylval.id_name = vtr::strdup(yytext); return vSYMBOL_ID; }

    /* return operators */
<INITIAL>{vPUNCT}                    { MP(); return yytext[0]; }

    /************
     * general stuff 
     **/

    /* single line comment */
<*>[\/][\/]                         { 
                                        int state = TOP_STATE();
                                        if (state == DEFINE_BODY)
                                        {
                                            /**
                                            * single line comments will automaticaly continue on if we
                                            * escape the new line, to prevent issues, we stop processing the macro body
                                            */
                                            finalize_define();
                                            POP_STATE();
                                        }
                                        if(state != COMMENT
                                        && state != MULTI_LINE_COMMENT)
                                        {
                                            PUSH_STATE(COMMENT);
                                        }
                                    }
    /* multi line comment */
<*>[\/][\*]                         { 
                                        int state = TOP_STATE();
                                        if(state != COMMENT
                                        && state != MULTI_LINE_COMMENT)
                                        {
                                            PUSH_STATE(MULTI_LINE_COMMENT);
                                        }
                                    }

<MULTI_LINE_COMMENT>[\*][\/]        { POP_STATE(); }

<*>[[:blank:]]+                     {    
                                        int state = TOP_STATE();
                                        if(state == DEFINE_BODY)
                                        {
                                            append_to_define_body(' ');
                                        }
                                        else if(state == DEFINE_ARGS
                                        ||        state == BRACKET_MATCH)
                                        {
                                            define_arg_push_back(yytext[0]);
                                        } 
                                    }

<*><<EOF>>                            { if ( ! pop_include() ){ free_define_map(); yyterminate(); } }

    /* skip escapped newline */
<*>\\\r?\n                            { my_location.line++; my_location.col = 1; }

    /* deal with new lines */
<*>\r?\n                            { 
                                        bool done = false;
                                        do{
                                            int state = TOP_STATE();

                                            if(state == DEFINE_BODY)
                                            {
                                                finalize_define();
                                            } 

                                            done = ( state != DEFINE_BODY && state != COMMENT );
                                            if(!done)
                                            {
                                                POP_STATE();
                                            }

                                        }while(!done);

                                        my_location.line++;
                                        my_location.col = 1;
                                    }

    /* catch all */
<*>.                                { 
                                        MP(); 
                                        int state = TOP_STATE();
                                        if(state == DEFINE_BODY)
                                        {
                                            append_to_define_body(yytext[0]);
                                        }
                                        else if(state == DEFINE_ARGS
                                        ||        state == BRACKET_MATCH)
                                        {
                                            define_arg_push_back(yytext[0]);
                                        } 
                                        else if(state == READ_IEEE)
                                        {
                                            /* catch stuck parser */
                                            POP_STATE();
                                        }
                                    }                

%%

void MP()        
{ 
    if (configuration.print_parse_tokens) 
    {
        printf("%d %s\n", my_location.line, yytext);
    } 
}

int top_flex_state(const char *str)        
{ 
    if(flex_state.empty())
    {
        flex_state.push_back(INITIAL);
    } 
    int state = flex_state.back(); 
    if (configuration.print_parse_tokens && strlen(str)) 
    {
        printf("%s state: %s\n", str,
            (state == INCLUDE)?            "INCLUDE":
            (state == COMMENT)?            "COMMENT":
            (state == MULTI_LINE_COMMENT)?    "MULTI_LINE_COMMENT":
            (state == DEFINE_BODY)?        "DEFINE_BODY":
            (state == SKIP)?                "SKIP":
            (state == ELIFCONDITION)?        "ELIFCONDITION":
            (state == NEG_CONDITION)?        "NEG_CONDITION":
            (state == CONDITION)?            "CONDITION":
            (state == DEFINE_REMOVAL)?        "DEFINE_REMOVAL":
            (state == VAR_PARSE)?            "VAR_PARSE":
            (state == PARSE_DEFINE)?        "PARSE_DEFINE":
            (state == DEFINE_ARGS)?        "DEFINE_ARGS":
                                            "INITIAL"
        );
    } 
    return state;
}

static bool has_current_parse_file()
{
    return (
        my_location.file < include_file_names.size()
        && my_location.file >= 0
    );
}

void lex_string(const char * str)
{

    if (configuration.print_parse_tokens) 
    {
        printf("Processing define %s\n", str);
    } 

    if(has_current_parse_file() 
    && current_include_stack.back() == my_location.file)
    {
        include_file_names[my_location.file].second = my_location.line;
    }

    /* check current depth, prevent too much macro recursion */
    loc_t posible_error_location = {include_file_names[my_location.file].second, my_location.file, -1};
    if(current_include_stack.size() > RECURSIVE_LIMIT)
    {
        error_message(PARSER, posible_error_location,
            "Reached upper macro recursion limit of %d", 
            RECURSIVE_LIMIT);
    }
    else if(current_include_stack.size() > (RECURSIVE_LIMIT/2))
    {
        warning_message(PARSER, posible_error_location,
            "Reached halfway to upper macro recursion limit of %d", 
            RECURSIVE_LIMIT);
    }


    current_include_stack.push_back(-1);
    my_location.line = 0;

    YY_BUFFER_STATE cur = YY_CURRENT_BUFFER;
    YY_BUFFER_STATE yybuff = yy_scan_string(str);
    yy_switch_to_buffer(cur);
    yypush_buffer_state(yybuff);

}

void push_include(const char *file_name)
{

    printf("Adding file %s to parse list\n", file_name);

    std::string tmp(file_name);

    if(tmp[0] == '"')
    {
        tmp.erase(0,1);
    }

    if(tmp.back() == '"')
    {
        tmp.pop_back();
    }

    std::string current_file = "";
    if(has_current_parse_file())
    {
        current_file = include_file_names[my_location.file].first;
        if(current_include_stack.back() == my_location.file)
        {
            include_file_names[my_location.file].second = my_location.line;
        }
    }

    /* we add the path from the current file */
    size_t loc = current_file.find_last_of("/");
    if(loc == std::string::npos)
    {
        current_file = tmp;
    }
    else
    {
        current_file = current_file.substr(0, loc + 1) + tmp;
    }

    yyin = fopen(current_file.c_str(), "r");
    if(yyin == NULL)
    {
        printf("Unable to open %s, trying %s\n", current_file.c_str(), tmp.c_str());
        current_file = tmp;
        yyin = open_file(current_file.c_str(), "r");
    }
    
    my_location.line = 0;
    current_include_stack.push_back(include_file_names.size());
    include_file_names.push_back({current_file,my_location.line});

    my_location.file = current_include_stack.back();
    assert_supported_file_extension(include_file_names.back().first.c_str() , my_location); 

    YY_BUFFER_STATE yybuff = yy_create_buffer( yyin, YY_BUF_SIZE );
    yypush_buffer_state(yybuff);

}

bool pop_include()
{
    if(has_current_parse_file())
    {
        if(configuration.print_parse_tokens)
        {
            printf("Poping file %s from parse list\n", include_file_names[my_location.file].first.c_str());
        }
        
        if(yyin)
        {
            fflush(yyin);
            fclose(yyin);
            yyin = NULL;
        }
    }

    if(!current_include_stack.empty())
    {
        current_include_stack.pop_back();
    }

    if(!current_include_stack.empty())
    {
        if(current_include_stack.back() != -1)
        {
            my_location.file = current_include_stack.back();
        }
    }
    else
    {
        my_location.file = -1;
    }

    if(has_current_parse_file() && my_location.file >= 0)
    {
        my_location.line = include_file_names[my_location.file].second;

        if(configuration.print_parse_tokens)
        {
            printf("Reading file %s from line %d\n", include_file_names[my_location.file].first.c_str(), my_location.line);
        }
    }
    else
    {
        my_location.line = -1;
    }
    
    yypop_buffer_state(); 
    return ( YY_CURRENT_BUFFER );
}

void initialize_defaults()
{
    default_net_type = WIRE;
    ieee_state.clear();
    current_define = NULL;
    free_define_map();
}

void new_define_t(const char* id)
{
    std::string tmp(id);
    if(tmp.back() == '(')
    {
        tmp.pop_back();
    }

    defines_t *new_define = new defines_t();
    new_define->name = tmp;
    new_define->use_va_args = false;
    new_define->args = std::vector<std::string>();
    new_define->body = "";
    
    if(defines_map.find(tmp) != defines_map.end())
    {
        warning_message(PARSER, my_location, "%s is redefined, overwritting its value", id);
        delete defines_map[tmp];    
    }

    defines_map[tmp] = new_define;
    current_define = new_define;
}

void free_define_t(const char *id)
{
    std::string tmp(id);
    if(defines_map.find(tmp) != defines_map.end())
    {
        delete defines_map[tmp];
        defines_map.erase(tmp);
    }
}

void free_define_map()
{
    for(auto kv: defines_map)
    {
        delete kv.second;
    }
}

std::string get_simple_define(const char *str)
{
    load_define(str);
    return get_complex_define();
}

std::string get_complex_define()
{
    if(current_define)
    {
        std::string va_args_replacement = "";

        if( current_args.size() < current_define->args.size())
        {
            error_message(PARSER, my_location, 
                "define `%s is being used with too few arguments, Expected %ld, got %ld", 
                    current_define->name.c_str(), current_define->args.size(),current_args.size());
        }
        else if( current_args.size() > current_define->args.size())
        {
            if(! current_define->use_va_args) {
                error_message(PARSER, my_location, 
                    "define `%s is being used with too many arguments, Expected %ld, got %ld", 
                        current_define->name.c_str(), current_define->args.size(),current_args.size());
            } else {
                while(current_define->args.size() != current_args.size())
                {
                    if(va_args_replacement != "")
                    {
                        va_args_replacement = ", " + va_args_replacement;
                    }
                    va_args_replacement = current_args.back() + va_args_replacement;
                    current_args.pop_back();
                }

            }
        }

        if (current_define->use_va_args)
        {
            current_define->args.push_back("__VA_ARGS__");
            current_args.push_back(va_args_replacement);
        }


        for(int i=0; i<current_define->args.size(); i++)
        {
            std::string original_arg = current_define->args[i];
            std::string replacement_arg = current_args[i];

            size_t pos = current_define_body.find( original_arg, 0 );
            while ( pos != std::string::npos )
            {
                current_define_body.erase(pos, original_arg.size());
                current_define_body.insert( pos, replacement_arg );
                pos += replacement_arg.size();
                pos = current_define_body.find( original_arg, pos );
            }
        }

        if(configuration.print_parse_tokens )
        {
            printf("DEFINE = %s\n",  current_define_body.c_str());
        }
    }
    return current_define_body;
}

void next_define_arg()
{
    current_args.push_back("");
}

void define_arg_push_back(char c)
{
    if(current_args.empty())
    {
        next_define_arg();
    }

    current_args.back().push_back(c);
}

void load_define(const char *str)
{
    std::string tmp(str);

    current_define = NULL;
    current_define_body = "";
    current_args = std::vector<std::string>();

    // compiler specific macros
    if(tmp == "`__LINE__") 
    {
        current_define_body = std::to_string(my_location.line + 1 /* 0 indexed */);
    }
    else if (tmp == "`__FILE__")
    {
        current_define_body = "\"" + std::string(configuration.list_of_file_names[my_location.file]) + "\"";
    }
    else
    {
        if(tmp[0] == '`')
        {
            tmp.erase(0,1);
        }

        if(tmp.back() == '(')
        {
            tmp.pop_back();
        }

        auto itter = defines_map.find(tmp);
        if(itter == defines_map.end())
        {
            if(tmp == "elif")
            {
                warning_message(PARSER, my_location, 
                    "%s", "using `elif, when you probably meant `elsif");
            }
            else
            {
                warning_message(PARSER, my_location, 
                    "%s define cannot be found, replacing with empty string and continuing synthesis", 
                    tmp.c_str());
            }
        }
        else
        {
            current_define = itter->second;
            current_define_body = current_define->body;
        }
    }
}

void append_to_define_body(char c)
{
    current_define->body.push_back(c);
}

void add_args_to_define(const char* str)
{
    std::string tmp(str);
    if(current_define->use_va_args)
    {
        error_message(PARSER, my_location, 
                "%s","... must be used as the last argument of a `define");
    }
    else if(tmp == "...")
    {
        current_define->use_va_args = true;
    }
    else
    {
        for(int i=0; i < current_define->args.size(); i++)
        {
            if(tmp == current_define->args[i])
            {
                error_message(PARSER, my_location, 
                    "%s","define has two argument with same name");
            }
        }

        current_define->args.push_back(std::string(str));
    }
}

void finalize_define()
{
    current_define = NULL;
}

bool ifdef(const char* id)
{
    return ( defines_map.find(std::string(id)) != defines_map.end() );
}

int ieee_filter(int return_type)
{
    extern int ieee_filter(int std_version, int ret);
    
    if(ieee_state.empty())
    {
        ieee_state.push_back(ieee_2005);
    } 
    int std_version = ieee_state.back(); 
    if (configuration.print_parse_tokens) 
    {
        printf("ieee version used: %s\n", ieee_std_STR[std_version]);
    } 
    
    return ieee_filter(std_version, return_type);
}
