# Microsoft Developer Studio Project File - Name="abc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=abc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "abc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "abc.mak" CFG="abc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "abc - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "abc - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "abc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "src\base\abc" /I "src\base\abci" /I "src\base\abcs" /I "src\base\seq" /I "src\base\cmd" /I "src\base\io" /I "src\base\main" /I "src\bdd\cudd" /I "src\bdd\epd" /I "src\bdd\mtr" /I "src\bdd\parse" /I "src\bdd\dsd" /I "src\bdd\reo" /I "src\sop\ft" /I "src\sat\asat" /I "src\sat\bsat" /I "src\sat\msat" /I "src\sat\fraig" /I "src\opt\cut" /I "src\opt\dec" /I "src\opt\fxu" /I "src\opt\sim" /I "src\opt\rwr" /I "src\opt\res" /I "src\opt\lpk" /I "src\map\fpga" /I "src\map\if" /I "src\map\mapper" /I "src\map\mio" /I "src\map\super" /I "src\misc\extra" /I "src\misc\st" /I "src\misc\mvc" /I "src\misc\util" /I "src\misc\npn" /I "src\misc\vec" /I "src\misc\espresso" /I "src\misc\nm" /I "src\misc\hash" /I "src\aig\ivy" /I "src\aig\hop" /I "src\aig\rwt" /I "src\aig\deco" /I "src\aig\mem" /I "src\aig\aig" /I "src\aig\dar" /I "src\aig\cnf" /I "src\aig\fra" /I "src\aig\kit" /I "src\aig\bdc" /I "src\temp\esop" /I "src\phys\place" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "__STDC__" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /profile /machine:I386 /out:"_TEST/abc.exe"

!ELSEIF  "$(CFG)" == "abc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "src\base\abc" /I "src\base\abci" /I "src\base\abcs" /I "src\base\seq" /I "src\base\cmd" /I "src\base\io" /I "src\base\main" /I "src\bdd\cudd" /I "src\bdd\epd" /I "src\bdd\mtr" /I "src\bdd\parse" /I "src\bdd\dsd" /I "src\bdd\reo" /I "src\sop\ft" /I "src\sat\asat" /I "src\sat\bsat" /I "src\sat\msat" /I "src\sat\fraig" /I "src\opt\cut" /I "src\opt\dec" /I "src\opt\fxu" /I "src\opt\sim" /I "src\opt\rwr" /I "src\opt\res" /I "src\opt\lpk" /I "src\map\fpga" /I "src\map\if" /I "src\map\mapper" /I "src\map\mio" /I "src\map\super" /I "src\misc\extra" /I "src\misc\st" /I "src\misc\mvc" /I "src\misc\util" /I "src\misc\npn" /I "src\misc\vec" /I "src\misc\espresso" /I "src\misc\nm" /I "src\misc\hash" /I "src\aig\ivy" /I "src\aig\hop" /I "src\aig\rwt" /I "src\aig\deco" /I "src\aig\mem" /I "src\aig\aig" /I "src\aig\dar" /I "src\aig\cnf" /I "src\aig\fra" /I "src\aig\kit" /I "src\aig\bdc" /I "src\temp\esop" /I "src\phys\place" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "__STDC__" /FR /YX /FD /GZ /c
# SUBTRACT CPP /X
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"_TEST/abc.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "abc - Win32 Release"
# Name "abc - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "base"

# PROP Default_Filter ""
# Begin Group "abc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\base\abc\abc.h
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcAig.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcBlifMv.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcCheck.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcDfs.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcFanio.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcFunc.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcHie.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcInt.h
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcLatch.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcLib.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcMinBase.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcNames.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcNetlist.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcNtk.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcObj.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcRefs.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcShow.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcSop.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abc\abcUtil.c
# End Source File
# End Group
# Begin Group "abci"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\base\abci\abc.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcAttach.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcAuto.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcBalance.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcBmc.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcCas.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcClpBdd.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcClpSop.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcCut.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcDar.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcDebug.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcDress.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcDsd.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcEspresso.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcExtract.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcFpga.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcFpgaFast.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcFraig.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcFxu.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcGen.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcHaig.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcIf.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcIvy.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcLut.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcMap.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcMeasure.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcMini.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcMiter.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcMulti.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcMv.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcNtbdd.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcOdc.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcOrder.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcPart.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcPlace.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcPrint.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcProve.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcQbf.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcQuant.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcRec.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcReconv.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcRefactor.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcRenode.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcReorder.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcRestruct.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcResub.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcRewrite.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcRr.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcSat.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcStrash.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcSweep.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcSymm.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcTiming.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcUnate.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcUnreach.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcVerify.c
# End Source File
# Begin Source File

SOURCE=.\src\base\abci\abcXsim.c
# End Source File
# End Group
# Begin Group "cmd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\base\cmd\cmd.c
# End Source File
# Begin Source File

SOURCE=.\src\base\cmd\cmd.h
# End Source File
# Begin Source File

SOURCE=.\src\base\cmd\cmdAlias.c
# End Source File
# Begin Source File

SOURCE=.\src\base\cmd\cmdApi.c
# End Source File
# Begin Source File

SOURCE=.\src\base\cmd\cmdFlag.c
# End Source File
# Begin Source File

SOURCE=.\src\base\cmd\cmdHist.c
# End Source File
# Begin Source File

SOURCE=.\src\base\cmd\cmdInt.h
# End Source File
# Begin Source File

SOURCE=.\src\base\cmd\cmdUtils.c
# End Source File
# End Group
# Begin Group "io"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\base\io\io.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\io.h
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioInt.h
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioReadAiger.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioReadBaf.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioReadBench.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioReadBlif.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioReadBlifAig.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioReadBlifMv.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioReadDsd.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioReadEdif.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioReadEqn.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioReadPla.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioReadVerilog.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioUtil.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioWriteAiger.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioWriteBaf.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioWriteBench.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioWriteBlif.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioWriteBlifMv.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioWriteCnf.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioWriteDot.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioWriteEqn.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioWriteGml.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioWriteList.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioWritePla.c
# End Source File
# Begin Source File

SOURCE=.\src\base\io\ioWriteVerilog.c
# End Source File
# End Group
# Begin Group "main"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\base\main\libSupport.c
# End Source File
# Begin Source File

SOURCE=.\src\base\main\main.c
# End Source File
# Begin Source File

SOURCE=.\src\base\main\main.h
# End Source File
# Begin Source File

SOURCE=.\src\base\main\mainFrame.c
# End Source File
# Begin Source File

SOURCE=.\src\base\main\mainInit.c
# End Source File
# Begin Source File

SOURCE=.\src\base\main\mainInt.h
# End Source File
# Begin Source File

SOURCE=.\src\base\main\mainUtils.c
# End Source File
# End Group
# Begin Group "ver"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\base\ver\ver.h
# End Source File
# Begin Source File

SOURCE=.\src\base\ver\verCore.c
# End Source File
# Begin Source File

SOURCE=.\src\base\ver\verFormula.c
# End Source File
# Begin Source File

SOURCE=.\src\base\ver\verParse.c
# End Source File
# Begin Source File

SOURCE=.\src\base\ver\verStream.c
# End Source File
# End Group
# Begin Group "func"

# PROP Default_Filter ""
# End Group
# End Group
# Begin Group "bdd"

# PROP Default_Filter ""
# Begin Group "cudd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\bdd\cudd\cudd.h
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddAddAbs.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddAddApply.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddAddFind.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddAddInv.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddAddIte.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddAddNeg.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddAddWalsh.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddAndAbs.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddAnneal.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddApa.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddAPI.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddApprox.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddBddAbs.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddBddCorr.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddBddIte.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddBridge.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddCache.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddCheck.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddClip.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddCof.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddCompose.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddDecomp.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddEssent.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddExact.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddExport.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddGenCof.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddGenetic.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddGroup.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddHarwell.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddInit.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddInt.h
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddInteract.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddLCache.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddLevelQ.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddLinear.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddLiteral.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddMatMult.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddPriority.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddRead.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddRef.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddReorder.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddSat.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddSign.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddSolve.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddSplit.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddSubsetHB.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddSubsetSP.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddSymmetry.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddTable.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddUtil.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddWindow.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddZddCount.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddZddFuncs.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddZddGroup.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddZddIsop.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddZddLin.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddZddMisc.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddZddPort.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddZddReord.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddZddSetop.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddZddSymm.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cudd\cuddZddUtil.c
# End Source File
# End Group
# Begin Group "epd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\bdd\epd\epd.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\epd\epd.h
# End Source File
# End Group
# Begin Group "mtr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\bdd\mtr\mtr.h
# End Source File
# Begin Source File

SOURCE=.\src\bdd\mtr\mtrBasic.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\mtr\mtrGroup.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\mtr\mtrInt.h
# End Source File
# End Group
# Begin Group "parse"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\bdd\parse\parse.h
# End Source File
# Begin Source File

SOURCE=.\src\bdd\parse\parseCore.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\parse\parseEqn.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\parse\parseInt.h
# End Source File
# Begin Source File

SOURCE=.\src\bdd\parse\parseStack.c
# End Source File
# End Group
# Begin Group "dsd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\bdd\dsd\dsd.h
# End Source File
# Begin Source File

SOURCE=.\src\bdd\dsd\dsdApi.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\dsd\dsdCheck.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\dsd\dsdInt.h
# End Source File
# Begin Source File

SOURCE=.\src\bdd\dsd\dsdLocal.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\dsd\dsdMan.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\dsd\dsdProc.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\dsd\dsdTree.c
# End Source File
# End Group
# Begin Group "reo"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\bdd\reo\reo.h
# End Source File
# Begin Source File

SOURCE=.\src\bdd\reo\reoApi.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\reo\reoCore.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\reo\reoProfile.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\reo\reoSift.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\reo\reoSwap.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\reo\reoTest.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\reo\reoTransfer.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\reo\reoUnits.c
# End Source File
# End Group
# Begin Group "cas"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\bdd\cas\cas.h
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cas\casCore.c
# End Source File
# Begin Source File

SOURCE=.\src\bdd\cas\casDec.c
# End Source File
# End Group
# End Group
# Begin Group "sat"

# PROP Default_Filter ""
# Begin Group "msat"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\sat\msat\msat.h
# End Source File
# Begin Source File

SOURCE=.\src\sat\msat\msatActivity.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\msat\msatClause.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\msat\msatClauseVec.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\msat\msatInt.h
# End Source File
# Begin Source File

SOURCE=.\src\sat\msat\msatMem.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\msat\msatOrderH.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\msat\msatQueue.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\msat\msatRead.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\msat\msatSolverApi.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\msat\msatSolverCore.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\msat\msatSolverIo.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\msat\msatSolverSearch.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\msat\msatSort.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\msat\msatVec.c
# End Source File
# End Group
# Begin Group "fraig"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\sat\fraig\fraig.h
# End Source File
# Begin Source File

SOURCE=.\src\sat\fraig\fraigApi.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\fraig\fraigCanon.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\fraig\fraigChoice.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\fraig\fraigFanout.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\fraig\fraigFeed.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\fraig\fraigInt.h
# End Source File
# Begin Source File

SOURCE=.\src\sat\fraig\fraigMan.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\fraig\fraigMem.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\fraig\fraigNode.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\fraig\fraigPrime.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\fraig\fraigSat.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\fraig\fraigTable.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\fraig\fraigUtil.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\fraig\fraigVec.c
# End Source File
# End Group
# Begin Group "csat"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\sat\csat\csat_apis.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\csat\csat_apis.h
# End Source File
# End Group
# Begin Group "bsat"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\sat\bsat\satInter.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\bsat\satMem.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\bsat\satMem.h
# End Source File
# Begin Source File

SOURCE=.\src\sat\bsat\satSolver.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\bsat\satSolver.h
# End Source File
# Begin Source File

SOURCE=.\src\sat\bsat\satStore.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\bsat\satStore.h
# End Source File
# Begin Source File

SOURCE=.\src\sat\bsat\satTrace.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\bsat\satUtil.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\bsat\satVec.h
# End Source File
# End Group
# Begin Group "proof"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\sat\proof\pr.c
# End Source File
# Begin Source File

SOURCE=.\src\sat\proof\pr.h
# End Source File
# End Group
# End Group
# Begin Group "opt"

# PROP Default_Filter ""
# Begin Group "fxu"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\opt\fxu\fxu.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\fxu\fxu.h
# End Source File
# Begin Source File

SOURCE=.\src\opt\fxu\fxuCreate.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\fxu\fxuHeapD.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\fxu\fxuHeapS.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\fxu\fxuInt.h
# End Source File
# Begin Source File

SOURCE=.\src\opt\fxu\fxuList.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\fxu\fxuMatrix.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\fxu\fxuPair.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\fxu\fxuPrint.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\fxu\fxuReduce.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\fxu\fxuSelect.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\fxu\fxuSingle.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\fxu\fxuUpdate.c
# End Source File
# End Group
# Begin Group "rwr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\opt\rwr\rwr.h
# End Source File
# Begin Source File

SOURCE=.\src\opt\rwr\rwrDec.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\rwr\rwrEva.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\rwr\rwrExp.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\rwr\rwrLib.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\rwr\rwrMan.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\rwr\rwrPrint.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\rwr\rwrTemp.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\rwr\rwrUtil.c
# End Source File
# End Group
# Begin Group "cut"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\opt\cut\cut.h
# End Source File
# Begin Source File

SOURCE=.\src\opt\cut\cutApi.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\cut\cutCut.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\cut\cutExpand.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\cut\cutInt.h
# End Source File
# Begin Source File

SOURCE=.\src\opt\cut\cutList.h
# End Source File
# Begin Source File

SOURCE=.\src\opt\cut\cutMan.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\cut\cutMerge.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\cut\cutNode.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\cut\cutOracle.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\cut\cutPre22.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\cut\cutSeq.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\cut\cutTruth.c
# End Source File
# End Group
# Begin Group "dec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\opt\dec\dec.h
# End Source File
# Begin Source File

SOURCE=.\src\opt\dec\decAbc.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\dec\decFactor.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\dec\decMan.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\dec\decPrint.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\dec\decUtil.c
# End Source File
# End Group
# Begin Group "sim"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\opt\sim\sim.h
# End Source File
# Begin Source File

SOURCE=.\src\opt\sim\simMan.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\sim\simSat.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\sim\simSeq.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\sim\simSupp.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\sim\simSwitch.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\sim\simSym.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\sim\simSymSat.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\sim\simSymSim.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\sim\simSymStr.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\sim\simUtils.c
# End Source File
# End Group
# Begin Group "ret"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\opt\ret\retArea.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\ret\retCore.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\ret\retDelay.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\ret\retFlow.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\ret\retIncrem.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\ret\retInit.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\ret\retInt.h
# End Source File
# Begin Source File

SOURCE=.\src\opt\ret\retLvalue.c
# End Source File
# End Group
# Begin Group "res"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\opt\res\res.h
# End Source File
# Begin Source File

SOURCE=.\src\opt\res\resCore.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\res\resDivs.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\res\resFilter.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\res\resInt.h
# End Source File
# Begin Source File

SOURCE=.\src\opt\res\resSat.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\res\resSim.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\res\resStrash.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\res\resWin.c
# End Source File
# End Group
# Begin Group "lpk"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\opt\lpk\lpk.h
# End Source File
# Begin Source File

SOURCE=.\src\opt\lpk\lpkCore.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\lpk\lpkCut.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\lpk\lpkInt.h
# End Source File
# Begin Source File

SOURCE=.\src\opt\lpk\lpkMan.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\lpk\lpkMap.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\lpk\lpkMulti.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\lpk\lpkMux.c
# End Source File
# Begin Source File

SOURCE=.\src\opt\lpk\lpkSets.c
# End Source File
# End Group
# End Group
# Begin Group "map"

# PROP Default_Filter ""
# Begin Group "fpga"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\map\fpga\fpga.c
# End Source File
# Begin Source File

SOURCE=.\src\map\fpga\fpga.h
# End Source File
# Begin Source File

SOURCE=.\src\map\fpga\fpgaCore.c
# End Source File
# Begin Source File

SOURCE=.\src\map\fpga\fpgaCreate.c
# End Source File
# Begin Source File

SOURCE=.\src\map\fpga\fpgaCut.c
# End Source File
# Begin Source File

SOURCE=.\src\map\fpga\fpgaCutUtils.c
# End Source File
# Begin Source File

SOURCE=.\src\map\fpga\fpgaFanout.c
# End Source File
# Begin Source File

SOURCE=.\src\map\fpga\fpgaInt.h
# End Source File
# Begin Source File

SOURCE=.\src\map\fpga\fpgaLib.c
# End Source File
# Begin Source File

SOURCE=.\src\map\fpga\fpgaMatch.c
# End Source File
# Begin Source File

SOURCE=.\src\map\fpga\fpgaSwitch.c
# End Source File
# Begin Source File

SOURCE=.\src\map\fpga\fpgaTime.c
# End Source File
# Begin Source File

SOURCE=.\src\map\fpga\fpgaTruth.c
# End Source File
# Begin Source File

SOURCE=.\src\map\fpga\fpgaUtils.c
# End Source File
# Begin Source File

SOURCE=.\src\map\fpga\fpgaVec.c
# End Source File
# End Group
# Begin Group "mapper"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\map\mapper\mapper.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapper.h
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperCanon.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperCore.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperCreate.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperCut.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperCutUtils.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperFanout.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperInt.h
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperLib.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperMatch.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperRefs.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperSuper.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperSwitch.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperTable.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperTime.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperTree.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperTruth.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperUtils.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mapper\mapperVec.c
# End Source File
# End Group
# Begin Group "mio"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\map\mio\mio.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mio\mio.h
# End Source File
# Begin Source File

SOURCE=.\src\map\mio\mioApi.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mio\mioFunc.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mio\mioInt.h
# End Source File
# Begin Source File

SOURCE=.\src\map\mio\mioRead.c
# End Source File
# Begin Source File

SOURCE=.\src\map\mio\mioUtils.c
# End Source File
# End Group
# Begin Group "super"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\map\super\super.c
# End Source File
# Begin Source File

SOURCE=.\src\map\super\super.h
# End Source File
# Begin Source File

SOURCE=.\src\map\super\superAnd.c
# End Source File
# Begin Source File

SOURCE=.\src\map\super\superGate.c
# End Source File
# Begin Source File

SOURCE=.\src\map\super\superInt.h
# End Source File
# Begin Source File

SOURCE=.\src\map\super\superWrite.c
# End Source File
# End Group
# Begin Group "if"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\map\if\if.h
# End Source File
# Begin Source File

SOURCE=.\src\map\if\ifCore.c
# End Source File
# Begin Source File

SOURCE=.\src\map\if\ifCut.c
# End Source File
# Begin Source File

SOURCE=.\src\map\if\ifMan.c
# End Source File
# Begin Source File

SOURCE=.\src\map\if\ifMap.c
# End Source File
# Begin Source File

SOURCE=.\src\map\if\ifReduce.c
# End Source File
# Begin Source File

SOURCE=.\src\map\if\ifSeq.c
# End Source File
# Begin Source File

SOURCE=.\src\map\if\ifTime.c
# End Source File
# Begin Source File

SOURCE=.\src\map\if\ifTruth.c
# End Source File
# Begin Source File

SOURCE=.\src\map\if\ifUtil.c
# End Source File
# End Group
# End Group
# Begin Group "misc"

# PROP Default_Filter ""
# Begin Group "extra"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\misc\extra\extra.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\extra\extraBddAuto.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\extra\extraBddCas.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\extra\extraBddKmap.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\extra\extraBddMisc.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\extra\extraBddSymm.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\extra\extraBddUnate.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\extra\extraUtilBitMatrix.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\extra\extraUtilCanon.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\extra\extraUtilFile.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\extra\extraUtilMemory.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\extra\extraUtilMisc.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\extra\extraUtilProgress.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\extra\extraUtilReader.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\extra\extraUtilTruth.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\extra\extraUtilUtil.c
# End Source File
# End Group
# Begin Group "st"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\misc\st\st.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\st\st.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\st\stmm.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\st\stmm.h
# End Source File
# End Group
# Begin Group "mvc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\misc\mvc\mvc.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvc.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvcApi.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvcCompare.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvcContain.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvcCover.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvcCube.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvcDivide.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvcDivisor.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvcList.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvcLits.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvcMan.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvcOpAlg.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvcOpBool.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvcPrint.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvcSort.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\mvc\mvcUtils.c
# End Source File
# End Group
# Begin Group "vec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\misc\vec\vec.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\vec\vecAtt.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\vec\vecFlt.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\vec\vecInt.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\vec\vecPtr.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\vec\vecStr.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\vec\vecVec.h
# End Source File
# End Group
# Begin Group "espresso"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\misc\espresso\cofactor.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\cols.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\compl.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\contain.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\cubehack.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\cubestr.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\cvrin.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\cvrm.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\cvrmisc.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\cvrout.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\dominate.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\equiv.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\espresso.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\espresso.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\essen.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\exact.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\expand.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\gasp.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\gimpel.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\globals.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\hack.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\indep.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\irred.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\map.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\matrix.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\mincov.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\mincov.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\mincov_int.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\opo.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\pair.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\part.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\primes.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\reduce.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\rows.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\set.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\setc.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\sharp.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\sminterf.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\solution.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\sparse.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\sparse.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\sparse_int.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\unate.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\espresso\verify.c
# End Source File
# End Group
# Begin Group "util"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\misc\util\util_hack.h
# End Source File
# End Group
# Begin Group "nm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\misc\nm\nm.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\nm\nmApi.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\nm\nmInt.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\nm\nmTable.c
# End Source File
# End Group
# Begin Group "hash"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\misc\hash\hash.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\hash\hashFlt.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\hash\hashInt.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\hash\hashPtr.h
# End Source File
# End Group
# End Group
# Begin Group "phys"

# PROP Default_Filter ""
# Begin Group "place"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\phys\place\libhmetis.h
# End Source File
# Begin Source File

SOURCE=.\src\phys\place\place_base.c
# End Source File
# Begin Source File

SOURCE=.\src\phys\place\place_base.h
# End Source File
# Begin Source File

SOURCE=.\src\phys\place\place_bin.c
# End Source File
# Begin Source File

SOURCE=.\src\phys\place\place_genqp.c
# End Source File
# Begin Source File

SOURCE=.\src\phys\place\place_gordian.c
# End Source File
# Begin Source File

SOURCE=.\src\phys\place\place_gordian.h
# End Source File
# Begin Source File

SOURCE=.\src\phys\place\place_legalize.c
# End Source File
# Begin Source File

SOURCE=.\src\phys\place\place_pads.c
# End Source File
# Begin Source File

SOURCE=.\src\phys\place\place_partition.c
# End Source File
# Begin Source File

SOURCE=.\src\phys\place\place_qpsolver.c
# End Source File
# Begin Source File

SOURCE=.\src\phys\place\place_qpsolver.h
# End Source File
# End Group
# End Group
# Begin Group "ai"

# PROP Default_Filter ""
# Begin Group "hop"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\aig\hop\hop.h
# End Source File
# Begin Source File

SOURCE=.\src\aig\hop\hopBalance.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\hop\hopCheck.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\hop\hopDfs.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\hop\hopMan.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\hop\hopMem.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\hop\hopObj.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\hop\hopOper.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\hop\hopTable.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\hop\hopUtil.c
# End Source File
# End Group
# Begin Group "ivy"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\aig\ivy\ivy.h
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyBalance.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyCanon.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyCheck.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyCut.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyCutTrav.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyDfs.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyDsd.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyFanout.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyFastMap.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyFraig.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyHaig.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyMan.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyMem.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyMulti.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyObj.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyOper.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyResyn.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyRwr.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivySeq.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyShow.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyTable.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\ivy\ivyUtil.c
# End Source File
# End Group
# Begin Group "rwt"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\aig\rwt\rwt.h
# End Source File
# Begin Source File

SOURCE=.\src\aig\rwt\rwtDec.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\rwt\rwtMan.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\rwt\rwtUtil.c
# End Source File
# End Group
# Begin Group "deco"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\aig\deco\deco.h
# End Source File
# End Group
# Begin Group "mem"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\aig\mem\mem.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\mem\mem.h
# End Source File
# End Group
# Begin Group "ec"

# PROP Default_Filter ""
# End Group
# Begin Group "dar"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\aig\dar\dar.h
# End Source File
# Begin Source File

SOURCE=.\src\aig\dar\darBalance.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\dar\darCore.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\dar\darCut.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\dar\darData.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\dar\darInt.h
# End Source File
# Begin Source File

SOURCE=.\src\aig\dar\darLib.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\dar\darMan.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\dar\darRefact.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\dar\darResub.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\dar\darScript.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\dar\darTruth.c
# End Source File
# End Group
# Begin Group "fra"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\aig\fra\fra.h
# End Source File
# Begin Source File

SOURCE=.\src\aig\fra\fraCec.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\fra\fraClass.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\fra\fraCnf.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\fra\fraCore.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\fra\fraInd.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\fra\fraMan.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\fra\fraSat.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\fra\fraSec.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\fra\fraSim.c
# End Source File
# End Group
# Begin Group "cnf"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\aig\cnf\cnf.h
# End Source File
# Begin Source File

SOURCE=.\src\aig\cnf\cnfCore.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\cnf\cnfCut.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\cnf\cnfData.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\cnf\cnfMan.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\cnf\cnfMap.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\cnf\cnfPost.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\cnf\cnfUtil.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\cnf\cnfWrite.c
# End Source File
# End Group
# Begin Group "csw"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\aig\csw\csw.h
# End Source File
# Begin Source File

SOURCE=.\src\aig\csw\cswCore.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\csw\cswCut.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\csw\cswInt.h
# End Source File
# Begin Source File

SOURCE=.\src\aig\csw\cswMan.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\csw\cswTable.c
# End Source File
# End Group
# Begin Group "kit"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\aig\kit\kit.h
# End Source File
# Begin Source File

SOURCE=.\src\aig\kit\kitBdd.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\kit\kitDsd.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\kit\kitFactor.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\kit\kitGraph.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\kit\kitHop.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\kit\kitIsop.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\kit\kitSop.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\kit\kitTruth.c
# End Source File
# End Group
# Begin Group "bdc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\aig\bdc\bdc.h
# End Source File
# Begin Source File

SOURCE=.\src\aig\bdc\bdcCore.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\bdc\bdcDec.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\bdc\bdcInt.h
# End Source File
# Begin Source File

SOURCE=.\src\aig\bdc\bdcTable.c
# End Source File
# End Group
# Begin Group "aig"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\aig\aig\aig.h
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigCheck.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigDfs.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigFanout.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigMan.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigMem.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigMffc.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigObj.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigOper.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigOrder.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigPart.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigRepr.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigSeq.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigShow.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigTable.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigTiming.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigTruth.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigUtil.c
# End Source File
# Begin Source File

SOURCE=.\src\aig\aig\aigWin.c
# End Source File
# End Group
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
