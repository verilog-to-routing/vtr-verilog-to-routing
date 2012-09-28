#include "cmdo.r"

resource 'cmdo' (128, "Antlr") {
	{
		/* [1] */
		295,
		"ANTLR -- Purdue Compiler Construction Tool Set (PCCTS) LL(k) parser generator.",
		{	
			/* [1] */
			NotDependent { }, MultiFiles {
				"Grammar File(s)…",
				"Choose the grammar specification files you wish to have ANTLR process.",
				{25, 24, 44, 154},
				"Grammar specification:",
				"",
				MultiInputFiles {
					{	/* array MultiTypesArray: 1 elements */
						/* [1] */
						text
					},
					".g",
					"Files ending in .g",
					"All text files"
				}
			},
			/* [2] */
			NotDependent { }, Files {
				DirOnly,
				OptionalFile {
					{56, 25, 72, 155},
					{77, 25, 96, 155},
					"Output Directory",
					":",
					"-o",
					"",
					"Choose the directory where ANTLR will put its output.",
					dim,
					"Output Directory…",
					"",
					""
				},
				NoMore {

				}
			},
			/* [3] */
			NotDependent { }, Redirection {
				StandardOutput,
				{126, 27}
			},
			/* [4] */
			NotDependent { }, Redirection {
				DiagnosticOutput,
				{126, 178}
			},
			/* [5] */
			NotDependent { }, TextBox {
				gray,
				{117, 20, 167, 300},
				"Redirection"
			},
			/* [6] */
			NotDependent { }, NestedDialog {
				2,
				{20, 324, 40, 460},
				"Options…",
				"Various command line options may be set "
				"with this button."
			},
			/* [7] */
			NotDependent { }, NestedDialog {
				3,
				{48, 324, 68, 460},
				"More Options…",
				"Antlr has ALOT of options. There are even more to be found with this button."
			},
			/* [8] */
			NotDependent { }, NestedDialog {
				4,
				{76, 324, 96, 460},
				"Rename Options…",
				"Options for renaming output files may be set with this button."
			},
			/* [9] */
			NotDependent { }, VersionDialog {
				VersionString {
					"1.33"
				},
				"PCCTS was written by Terence Parr, Russell Quong, Will Cohen, and Hank Dietz: 1989-1995. "
				"MPW port by Scott Haney.",
				noDialog
			}
		},
		/* [2] */
		295,
		"Use this dialog to specify command line options.",
		{
			/* [1] */
			NotDependent { }, CheckOption {
				NotSet,
				{18, 25, 33, 225},
				"Generate C++ code",
				"-CC",
				"Generate C++ output from both ANTLR and DLG."
			},
			/* [2] */
			NotDependent { }, CheckOption {
				NotSet,
				{38, 25, 53, 225},
				"Generate ASTs",
				"-gt",
				"Generate code for Abstract-Syntax-Trees (ASTs)."
			},
			/* [3] */
			NotDependent { }, CheckOption {
				NotSet,
				{18, 235, 33, 435},
				"Support parse traces",
				"-gd",
				"If this option is checked, ANTLR inserts code in each parsing "
				"function to provide for user-defined handling of a detailed parse trace. "
				"The code consists of calls to zzTRACEIN and zzTRACEOUT."
			},
			/* [4] */
			NotDependent { }, CheckOption {
				NotSet,
				{58, 25, 73, 225},
				"Generate line info",
				"-gl",
				"If this option is checked, ANTLR will generate line info about grammar"
				"actions, thereby making debugging easier since "
				"compile errors will point to the grammar file."
			},
			/* [5] */
			NotDependent { }, CheckOption {
				NotSet,
				{38, 235, 53, 435},
				"Generate cross-references",
				"-cr",
				"If this option is checked, ANTLR will generate a cross reference for all "
				"rules. For each rule it will print a list of all other rules that refrence it."
			},
			/* [6] */
			NotDependent { }, CheckOption {
				NotSet,
				{78, 25, 93, 225},
				"Generate error classes",
				"-ge",
				"If this option is checked, ANTLR will generate an error class for"
				"each non-terminal."
			},
			/* [7] */
			NotDependent { }, CheckOption {
				NotSet,
				{58, 235, 73, 435},
				"Hoist predicate context",
				"-prc on",
				"If this option is checked, ANTLR will turn on the computation and hoisting of "
				"predicate context."
			},
			/* [8] */
			NotDependent { }, CheckOption {
				NotSet,
				{98, 25, 113, 225},
				"Don't generate Code",
				"-gc",
				"If this option is checked, ANTLR will generate no code, i.e. "
				"it will only perform analysis on the grammar."
			},
			/* [9] */
			NotDependent { }, CheckOption {
				NotSet,
				{78, 235, 93, 435},
				"Don't create Lexer files",
				"-gx",
				"If this option is checked, ANTLR will not generate DLG-related output files. "
				"This option should be used if one wants a custom lexical analyzer or if one "
				"has made changes to the grammar not affecting the lexical structure."
			},
			/* [10] */
			NotDependent { }, CheckOption {
				NotSet,
				{118, 25, 133, 225},
				"Delay lookahead fetches",
				"-gk",
				"If this option is checked, ANTLR will generate a parser that delays lookahead "
				"fetches until needed."
			},
			/* [11] */
			NotDependent { }, CheckOption {
				NotSet,
				{98, 235, 113, 460},
				"Don't generate token expr sets",
				"-gs",
				"If this option is checked, ANTLR will not generate sets for token expression "
				"sets; instead, it will generate a || separated sequence of LA(1)==token #. "
			},
			/* [12] */
			NotDependent { }, RegularEntry {
				"Lookahead:",
				{140, 25, 155, 150},
				{160, 25, 176, 150},
				"1",
				keepCase,
				"-k",
				"This entry specifies the number of tokens of lookahead."
			},
			/* [13] */
			NotDependent { }, RegularEntry {
				"Compr lookahead:",
				{140, 165, 155, 290},
				{160, 165, 176, 290},
				"",
				keepCase,
				"-ck",
				"This entry specifies the number of tokens of lookahead when using compressed "
				"(linear approximation) lookahead. In general, the compressed lookahead is much "
				"deeper than the full lookahead."
			},
			/* [14] */
			NotDependent { }, RegularEntry {
				"Max tree nodes:",
				{140, 310, 155, 435},
				{160, 305, 176, 435},
				"",
				keepCase,
				"-rl",
				"This entry specifies the maximum number of tokens of tree nodes used by the grammar "
				"analysis."
			}
		},
		/* [3] */
		295,
		"Use this dialog to specify still more command line options.",
		{
			/* [1] */
			NotDependent { }, RadioButtons {
				{	/* array radioArray: 3 elements */
					/* [1] */
					{38, 25, 53, 105}, "None", "", Set, "When this option is selected, ANTLR "
					"will not print the grammar to stdout.",
					/* [2] */
					{38, 115, 53, 195}, "Yes", "-p", NotSet, "When this option is selected, ANTLR "
					"will print the grammar, stripped of all actions and comments, to stdout.",
					/* [3] */
					{38, 210, 53, 300}, "More", "-pa", NotSet, "When this option is selected, ANTLR "
					"will print the grammar, stripped of all actions and comments, to stdout. "
					"It will also annotate the output with the first sets determined from grammar "
					"analysis."
				}
			},
			/* [2] */
			NotDependent { }, TextBox {
				gray,
				{ 28, 15, 60, 310 },
				"Grammar Printing"
			},
			/* [3] */
			NotDependent { }, RadioButtons {
				{	/* array radioArray: 3 elements */
					/* [1] */
					{88, 25, 103, 105}, "Low", "", Set, "When this option is selected, ANTLR "
					"will show ambiguities/errors in low detail.",
					/* [2] */
					{88, 115, 103, 195}, "Medium", "-e2", NotSet, "When this option is selected, ANTLR "
					"will show ambiguities/errors in more detail.",
					/* [3] */
					{88, 210, 103, 300}, "High", "-e3", NotSet, "When this option is selected, ANTLR "
					"will show ambiguities/errors in excruciating detail."
				}
			},
			/* [4] */
			NotDependent { }, TextBox {
				gray,
				{ 78, 15, 110, 310 },
				"Error reporting"
			},
			/* [5] */
			NotDependent { }, CheckOption {
				NotSet,
				{128, 25, 143, 225},
				"More warnings",
				"-w2",
				"If this option is checked, ANTLR will warn if semantic predicates and/or "
				"(…)? blocks are assumed to cover ambiguous alternatives."
			},

		},
		/* [4] */
		295,
		"Use this dialog to specify command line options relating to renaming output files.",
		{
			/* [1] */
			NotDependent { }, RegularEntry {
				"Errors file name:",
				{35, 25, 50, 205},
				{35, 205, 51, 300},
				"err.c",
				keepCase,
				"-fe",
				"This entry specifies the name ANTLR uses for "
				"the errors file."
			},
			/* [2] */
			NotDependent { }, RegularEntry {
				"Lexical output name:",
				{60, 25, 75, 205},
				{60, 205, 76, 300},
				"parser.dlg",
				keepCase,
				"-fl",
				"This entry specifies the name ANTLR uses for "
				"the lexical output file."
			},
			/* [3] */
			NotDependent { }, RegularEntry {
				"Lexical modes name:",
				{85, 25, 100, 205},
				{85, 205, 101, 300},
				"mode.h",
				keepCase,
				"-fl",
				"This entry specifies the name ANTLR uses for "
				"the lexical mode definitions file."
			},
			/* [4] */
			NotDependent { }, RegularEntry {
				"Remap file name:",
				{110, 25, 125, 205},
				{110, 205, 126, 300},
				"remap.h",
				keepCase,
				"-fl",
				"This entry specifies the name ANTLR uses for "
				"the file that remaps globally visible symbols."
			},
			/* [5] */
			NotDependent { }, RegularEntry {
				"Tokens file name:",
				{135, 25, 150, 205},
				{135, 205, 151, 300},
				"tokens.h",
				keepCase,
				"-fl",
				"This entry specifies the name ANTLR uses for "
				"the tokens file."
			},
			/* [6] */
			NotDependent{ }, CheckOption {
				NotSet,
				{160, 25, 175, 175},
				"Create std header",
				"-gh",
				"If this option is checked, ANTLR will create a standard header file named, "
				"by default 'stdpccts.h'. This name can be altered using the entry right next door."
			},
			/* [7] */
			Or { {6} }, RegularEntry {
				"Std header file name:",
				{160, 175, 175, 355},
				{160, 355, 176, 450},
				"stdpccts.h",
				keepCase,
				"-fh",
				"This entry specifies the name ANTLR uses for "
				"the standard header file."
			}
		}
	}
};

