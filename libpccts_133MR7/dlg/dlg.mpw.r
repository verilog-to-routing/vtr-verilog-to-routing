#include "cmdo.r"
resource 'cmdo' (128, "Dlg") {
	{
		295,
		"DLG -- Purdue Compiler Construction Tool Set (PCCTS)"
		" lexical analyzer generator.",
		{
			/* [1] */
			NotDependent { }, CheckOption {
				NotSet,
				{35, 175, 50, 225},
				"On",
				"-CC",
				"When this control is checked, DLG generates"
				" a scanner using C++ classes rather"
				" than C functions."
			},
			/* [2] */
			Or{{1}}, RegularEntry {
				"Lexer Class Name:",
				{35, 225, 50, 355},
				{35, 355, 51, 450},
				"DLGLexer",
				keepCase,
				"-cl",
				"This entry specifies the name DLG uses for "
				"the C++ lexer class."
			},
			/* [3] */
			NotDependent { }, TextBox {
				gray,
				{ 25, 165, 60, 460 },
				"C++ Code Generation"
			},
			/* [4] */
			NotDependent { }, Files {
				InputFile,
				RequiredFile {
					{40, 25, 59, 135},
					"Input FileI",
					"",
					"Choose the lexical description file for DLG to process."
				},
				Additional {
					"",
					"",
					"",
					"",
					{	/* array TypesArray: 1 elements */
						/* [1] */
						text
					}
				}
			},
			/* [5] */
			Or {{-1}}, Files {
				OutputFile,
				RequiredFile {
					{83, 25, 102, 135},
					"Output FileI",
					"",
					"Choose the name of the file that will hold the DLG-produced scanner."
				},
				NoMore { }
			},
			/* [6] */
			Or { {1,5} }, Dummy { },
			/* [7] */
			NotDependent { }, Redirection {
				DiagnosticOutput,
				{ 115, 25 }
			},
			/* [8] */
			NotDependent { }, TextBox {
				gray,
				{ 25, 20, 156, 145 },
				"Files"
			},
			/* [9] */
			NotDependent { }, Files {
				DirOnly,
				OptionalFile {
					{68, 175, 84, 305},
					{88, 175, 107, 305},
					"Output Directory",
					":",
					"-o",
					"",
					"Choose the directory where DLG will put "
					"its output.",
					dim,
					"Output DirectoryI",
					"",
					""
				},
				NoMore { }

			},
			/* [10] */
			NotDependent { }, RegularEntry {
				"Mode File Name:",
				{68, 315, 83, 450},
				{88, 315, 104, 450},
				"mode.h",
				keepCase,
				"-m",
				"This entry specifies the name DLG uses for "
				"its lexical mode output file."
			},
			/* [11] */
			NotDependent { }, RadioButtons {
				{	/* array radioArray: 3 elements */
					/* [1] */
					{134, 175, 149, 255}, "None", "", Set, "When this option is selected, DLG "
					"will not compress its tables.",
					/* [2] */
					{134, 265, 149, 345}, "Level 1", "-C1", NotSet, "When this option is selected, DLG "
					"will remove all unused characters from the transition-from table.",
					/* [3] */
					{134, 360, 149, 450}, "Level 2", "-C2", NotSet, "When this option is selected, DLG "
					"will perform level 1 compression plus it will map "
                                        "equivalent characters into the same character classes."
				}
			},
			/* [12] */
			NotDependent { }, TextBox {
				gray,
				{ 124, 165, 156, 460 },
				"Table Compression"
			},
			/* [13] */
			NotDependent { }, CheckOption {
				Set,
				{165, 20, 180, 145},
				"Case Sensitive",
				"-ci",
				"When this control is checked, the DLG automaton will "
				"treat upper and lower case characters identically."
			},
			/* [14] */
			NotDependent { }, CheckOption {
				NotSet,
				{165, 150, 180, 300},
				"Interactive Scanner",
				"-i",
				"When this control is checked, DLG will "
				"generate as interactive a scanner as possible."
			},
			/* [15] */
			NotDependent { }, CheckOption {
				NotSet,
				{165, 310, 180, 460},
				"Ambiguity Warnings",
				"-Wambiguity",
				"When this control is checked, DLG warns if more "
				"than one regular expression could match the same character sequence."
			},
			/* [16] */
			NotDependent { }, VersionDialog	{
				VersionString { "1.33MR1" },
				"PCCTS was written by Terence Parr, Russell Quong, Will Cohen, and Hank Dietz: 1989-1995."
				" MPW port by Scott Haney.",
				0
			},
			/* [17] */
			And { {4,6} }, DoItButton { }
		}
	}
};

