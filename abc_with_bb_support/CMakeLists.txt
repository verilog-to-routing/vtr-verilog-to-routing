cmake_minimum_required(VERSION 2.8.12)

project("abc_with_bb_support")

#Collect the source files
# ABC contains code (e.g. .c files) which are not part of the regular build process,
# and fail to compile. As a result we can't just collect all the .c files to compile,
# but must list them directly
set(EXEC_SOURCES src/base/main/main.c)

set(LIB_SOURCES 
    src/base/abc/abcAig.c src/base/abc/abcBlifMv.c src/base/abc/abcCheck.c src/base/abc/abcDfs.c src/base/abc/abcFanio.c 
    src/base/abc/abcFunc.c src/base/abc/abcHie.c src/base/abc/abcLatch.c src/base/abc/abcLib.c src/base/abc/abcMinBase.c 
    src/base/abc/abcNames.c src/base/abc/abcNetlist.c src/base/abc/abcNtk.c src/base/abc/abcObj.c src/base/abc/abcRefs.c 
    src/base/abc/abcShow.c src/base/abc/abcSop.c src/base/abc/abcUtil.c src/base/abci/abc.c src/base/abci/abcAttach.c 
    src/base/abci/abcAuto.c src/base/abci/abcBalance.c src/base/abci/abcBmc.c src/base/abci/abcCas.c src/base/abci/abcClpBdd.c 
    src/base/abci/abcClpSop.c src/base/abci/abcCut.c src/base/abci/abcDar.c src/base/abci/abcDebug.c src/base/abci/abcDress.c 
    src/base/abci/abcDsd.c src/base/abci/abcEspresso.c src/base/abci/abcExtract.c src/base/abci/abcFpga.c src/base/abci/abcFpgaFast.c 
    src/base/abci/abcFraig.c src/base/abci/abcFxu.c src/base/abci/abcGen.c src/base/abci/abcHaig.c src/base/abci/abcIf.c 
    src/base/abci/abcIvy.c src/base/abci/abcLut.c src/base/abci/abcMap.c src/base/abci/abcMini.c src/base/abci/abcMiter.c 
    src/base/abci/abcMulti.c src/base/abci/abcNtbdd.c src/base/abci/abcOdc.c src/base/abci/abcOrder.c src/base/abci/abcPart.c 
    src/base/abci/abcPlace.c src/base/abci/abcPrint.c src/base/abci/abcProve.c src/base/abci/abcQbf.c src/base/abci/abcQuant.c 
    src/base/abci/abcRec.c src/base/abci/abcReconv.c src/base/abci/abcRefactor.c src/base/abci/abcRenode.c src/base/abci/abcReorder.c 
    src/base/abci/abcRestruct.c src/base/abci/abcResub.c src/base/abci/abcRewrite.c src/base/abci/abcRr.c src/base/abci/abcSat.c 
    src/base/abci/abcStrash.c src/base/abci/abcSweep.c src/base/abci/abcSymm.c src/base/abci/abcTiming.c src/base/abci/abcUnate.c 
    src/base/abci/abcUnreach.c src/base/abci/abcVerify.c src/base/abci/abcXsim.c  src/base/cmd/cmd.c src/base/cmd/cmdAlias.c 
    src/base/cmd/cmdApi.c src/base/cmd/cmdFlag.c src/base/cmd/cmdHist.c src/base/cmd/cmdUtils.c src/base/io/io.c src/base/io/ioReadAiger.c 
    src/base/io/ioReadBaf.c src/base/io/ioReadBench.c src/base/io/ioReadBlif.c src/base/io/ioReadBlifAig.c src/base/io/ioReadBlifMv.c 
    src/base/io/ioReadDsd.c src/base/io/ioReadEdif.c src/base/io/ioReadEqn.c src/base/io/ioReadPla.c src/base/io/ioReadVerilog.c 
    src/base/io/ioUtil.c src/base/io/ioWriteAiger.c src/base/io/ioWriteBaf.c src/base/io/ioWriteBench.c src/base/io/ioWriteBlif.c 
    src/base/io/ioWriteBlifMv.c src/base/io/ioWriteCnf.c src/base/io/ioWriteDot.c src/base/io/ioWriteEqn.c src/base/io/ioWriteGml.c 
    src/base/io/ioWriteList.c src/base/io/ioWritePla.c src/base/io/ioWriteVerilog.c src/base/main/mainFrame.c 
    src/base/main/mainInit.c src/base/main/libSupport.c src/base/main/mainUtils.c src/base/ver/verCore.c src/base/ver/verFormula.c 
    src/base/ver/verParse.c src/base/ver/verStream.c  src/aig/ivy/ivyBalance.c src/aig/ivy/ivyCanon.c src/aig/ivy/ivyCheck.c 
    src/aig/ivy/ivyCut.c src/aig/ivy/ivyCutTrav.c src/aig/ivy/ivyDfs.c src/aig/ivy/ivyDsd.c src/aig/ivy/ivyFanout.c src/aig/ivy/ivyFastMap.c 
    src/aig/ivy/ivyFraig.c src/aig/ivy/ivyHaig.c src/aig/ivy/ivyMan.c src/aig/ivy/ivyMem.c src/aig/ivy/ivyMulti.c src/aig/ivy/ivyObj.c 
    src/aig/ivy/ivyOper.c src/aig/ivy/ivyResyn.c src/aig/ivy/ivyRwr.c src/aig/ivy/ivySeq.c src/aig/ivy/ivyShow.c src/aig/ivy/ivyTable.c 
    src/aig/ivy/ivyUtil.c src/aig/hop/hopBalance.c src/aig/hop/hopCheck.c src/aig/hop/hopDfs.c src/aig/hop/hopMan.c src/aig/hop/hopMem.c 
    src/aig/hop/hopObj.c src/aig/hop/hopOper.c src/aig/hop/hopTable.c src/aig/hop/hopUtil.c  src/aig/rwt/rwtDec.c src/aig/rwt/rwtMan.c 
    src/aig/rwt/rwtUtil.c  src/aig/mem/mem.c  src/aig/dar/darBalance.c src/aig/dar/darCore.c src/aig/dar/darCut.c src/aig/dar/darData.c 
    src/aig/dar/darLib.c src/aig/dar/darMan.c src/aig/dar/darRefact.c src/aig/dar/darResub.c src/aig/dar/darScript.c src/aig/dar/darTruth.c 
    src/aig/fra/fraCec.c src/aig/fra/fraClass.c src/aig/fra/fraCnf.c src/aig/fra/fraCore.c src/aig/fra/fraInd.c src/aig/fra/fraMan.c 
    src/aig/fra/fraSat.c src/aig/fra/fraSec.c src/aig/fra/fraSim.c src/aig/cnf/cnfCore.c src/aig/cnf/cnfCut.c src/aig/cnf/cnfData.c 
    src/aig/cnf/cnfMan.c src/aig/cnf/cnfMap.c src/aig/cnf/cnfPost.c src/aig/cnf/cnfUtil.c src/aig/cnf/cnfWrite.c  src/aig/csw/cswCore.c 
    src/aig/csw/cswCut.c src/aig/csw/cswMan.c src/aig/csw/cswTable.c   src/aig/aig/aigCheck.c src/aig/aig/aigDfs.c src/aig/aig/aigFanout.c 
    src/aig/aig/aigMan.c src/aig/aig/aigMem.c src/aig/aig/aigMffc.c src/aig/aig/aigObj.c src/aig/aig/aigOper.c src/aig/aig/aigOrder.c 
    src/aig/aig/aigPart.c src/aig/aig/aigRepr.c src/aig/aig/aigSeq.c src/aig/aig/aigTable.c src/aig/aig/aigTiming.c src/aig/aig/aigTruth.c 
    src/aig/aig/aigUtil.c src/aig/aig/aigWin.c src/aig/kit/kitBdd.c src/aig/kit/kitDsd.c src/aig/kit/kitFactor.c src/aig/kit/kitGraph.c 
    src/aig/kit/kitHop.c src/aig/kit/kitIsop.c src/aig/kit/kitSop.c src/aig/kit/kitTruth.c src/aig/bdc/bdcCore.c src/aig/bdc/bdcDec.c 
    src/aig/bdc/bdcTable.c src/bdd/cudd/cuddAPI.c src/bdd/cudd/cuddAddAbs.c src/bdd/cudd/cuddAddApply.c src/bdd/cudd/cuddAddFind.c 
    src/bdd/cudd/cuddAddInv.c src/bdd/cudd/cuddAddIte.c src/bdd/cudd/cuddAddNeg.c src/bdd/cudd/cuddAddWalsh.c src/bdd/cudd/cuddAndAbs.c 
    src/bdd/cudd/cuddAnneal.c src/bdd/cudd/cuddApa.c src/bdd/cudd/cuddApprox.c src/bdd/cudd/cuddBddAbs.c src/bdd/cudd/cuddBddCorr.c 
    src/bdd/cudd/cuddBddIte.c src/bdd/cudd/cuddBridge.c src/bdd/cudd/cuddCache.c src/bdd/cudd/cuddCheck.c src/bdd/cudd/cuddClip.c 
    src/bdd/cudd/cuddCof.c src/bdd/cudd/cuddCompose.c src/bdd/cudd/cuddDecomp.c src/bdd/cudd/cuddEssent.c src/bdd/cudd/cuddExact.c 
    src/bdd/cudd/cuddExport.c src/bdd/cudd/cuddGenCof.c src/bdd/cudd/cuddGenetic.c src/bdd/cudd/cuddGroup.c src/bdd/cudd/cuddHarwell.c 
    src/bdd/cudd/cuddInit.c src/bdd/cudd/cuddInteract.c src/bdd/cudd/cuddLCache.c src/bdd/cudd/cuddLevelQ.c src/bdd/cudd/cuddLinear.c 
    src/bdd/cudd/cuddLiteral.c src/bdd/cudd/cuddMatMult.c src/bdd/cudd/cuddPriority.c src/bdd/cudd/cuddRead.c src/bdd/cudd/cuddRef.c 
    src/bdd/cudd/cuddReorder.c src/bdd/cudd/cuddSat.c src/bdd/cudd/cuddSign.c src/bdd/cudd/cuddSolve.c src/bdd/cudd/cuddSplit.c 
    src/bdd/cudd/cuddSubsetHB.c src/bdd/cudd/cuddSubsetSP.c src/bdd/cudd/cuddSymmetry.c src/bdd/cudd/cuddTable.c src/bdd/cudd/cuddUtil.c 
    src/bdd/cudd/cuddWindow.c src/bdd/cudd/cuddZddCount.c src/bdd/cudd/cuddZddFuncs.c src/bdd/cudd/cuddZddGroup.c src/bdd/cudd/cuddZddIsop.c 
    src/bdd/cudd/cuddZddLin.c src/bdd/cudd/cuddZddMisc.c src/bdd/cudd/cuddZddPort.c src/bdd/cudd/cuddZddReord.c src/bdd/cudd/cuddZddSetop.c 
    src/bdd/cudd/cuddZddSymm.c src/bdd/cudd/cuddZddUtil.c  src/bdd/dsd/dsdApi.c src/bdd/dsd/dsdCheck.c src/bdd/dsd/dsdLocal.c src/bdd/dsd/dsdMan.c 
    src/bdd/dsd/dsdProc.c src/bdd/dsd/dsdTree.c src/bdd/epd/epd.c src/bdd/mtr/mtrBasic.c src/bdd/mtr/mtrGroup.c src/bdd/parse/parseCore.c 
    src/bdd/parse/parseEqn.c src/bdd/parse/parseStack.c src/bdd/reo/reoApi.c src/bdd/reo/reoCore.c src/bdd/reo/reoProfile.c src/bdd/reo/reoSift.c 
    src/bdd/reo/reoSwap.c src/bdd/reo/reoTransfer.c src/bdd/reo/reoUnits.c src/bdd/cas/casCore.c src/bdd/cas/casDec.c src/map/fpga/fpga.c 
    src/map/fpga/fpgaCore.c src/map/fpga/fpgaCreate.c src/map/fpga/fpgaCut.c src/map/fpga/fpgaCutUtils.c src/map/fpga/fpgaFanout.c 
    src/map/fpga/fpgaLib.c src/map/fpga/fpgaMatch.c src/map/fpga/fpgaSwitch.c src/map/fpga/fpgaTime.c src/map/fpga/fpgaTruth.c 
    src/map/fpga/fpgaUtils.c src/map/fpga/fpgaVec.c src/map/mapper/mapper.c src/map/mapper/mapperCanon.c src/map/mapper/mapperCore.c 
    src/map/mapper/mapperCreate.c src/map/mapper/mapperCut.c src/map/mapper/mapperCutUtils.c src/map/mapper/mapperFanout.c src/map/mapper/mapperLib.c
    src/map/mapper/mapperMatch.c src/map/mapper/mapperRefs.c src/map/mapper/mapperSuper.c src/map/mapper/mapperSwitch.c src/map/mapper/mapperTable.c
    src/map/mapper/mapperTime.c src/map/mapper/mapperTree.c src/map/mapper/mapperTruth.c src/map/mapper/mapperUtils.c src/map/mapper/mapperVec.c 
    src/map/mio/mio.c src/map/mio/mioApi.c src/map/mio/mioFunc.c src/map/mio/mioRead.c src/map/mio/mioUtils.c src/map/super/super.c 
    src/map/super/superAnd.c src/map/super/superGate.c src/map/super/superWrite.c src/map/if/ifCore.c src/map/if/ifCut.c src/map/if/ifMan.c 
    src/map/if/ifMap.c src/map/if/ifReduce.c src/map/if/ifSeq.c src/map/if/ifTime.c src/map/if/ifTruth.c src/map/if/ifUtil.c  
    src/misc/extra/extraBddAuto.c src/misc/extra/extraBddCas.c src/misc/extra/extraBddKmap.c src/misc/extra/extraBddMisc.c src/misc/extra/extraBddSymm.c
    src/misc/extra/extraBddUnate.c src/misc/extra/extraUtilBitMatrix.c src/misc/extra/extraUtilCanon.c src/misc/extra/extraUtilFile.c 
    src/misc/extra/extraUtilMemory.c src/misc/extra/extraUtilMisc.c src/misc/extra/extraUtilProgress.c src/misc/extra/extraUtilReader.c
    src/misc/extra/extraUtilTruth.c src/misc/extra/extraUtilUtil.c src/misc/mvc/mvc.c src/misc/mvc/mvcApi.c src/misc/mvc/mvcCompare.c 
    src/misc/mvc/mvcContain.c src/misc/mvc/mvcCover.c src/misc/mvc/mvcCube.c src/misc/mvc/mvcDivide.c src/misc/mvc/mvcDivisor.c src/misc/mvc/mvcList.c
    src/misc/mvc/mvcLits.c src/misc/mvc/mvcMan.c src/misc/mvc/mvcOpAlg.c src/misc/mvc/mvcOpBool.c src/misc/mvc/mvcPrint.c src/misc/mvc/mvcSort.c 
    src/misc/mvc/mvcUtils.c src/misc/st/st.c src/misc/st/stmm.c  src/misc/espresso/cofactor.c src/misc/espresso/cols.c src/misc/espresso/compl.c 
    src/misc/espresso/contain.c src/misc/espresso/cubehack.c src/misc/espresso/cubestr.c src/misc/espresso/cvrin.c src/misc/espresso/cvrm.c 
    src/misc/espresso/cvrmisc.c src/misc/espresso/cvrout.c src/misc/espresso/dominate.c src/misc/espresso/equiv.c src/misc/espresso/espresso.c 
    src/misc/espresso/essen.c src/misc/espresso/exact.c src/misc/espresso/expand.c src/misc/espresso/gasp.c src/misc/espresso/gimpel.c 
    src/misc/espresso/globals.c src/misc/espresso/hack.c src/misc/espresso/indep.c src/misc/espresso/irred.c src/misc/espresso/map.c 
    src/misc/espresso/matrix.c src/misc/espresso/mincov.c src/misc/espresso/opo.c src/misc/espresso/pair.c src/misc/espresso/part.c 
    src/misc/espresso/primes.c src/misc/espresso/reduce.c src/misc/espresso/rows.c src/misc/espresso/set.c src/misc/espresso/setc.c 
    src/misc/espresso/sharp.c src/misc/espresso/sminterf.c src/misc/espresso/solution.c src/misc/espresso/sparse.c src/misc/espresso/unate.c
    src/misc/espresso/verify.c src/misc/nm/nmApi.c src/misc/nm/nmTable.c   src/opt/cut/cutApi.c src/opt/cut/cutCut.c src/opt/cut/cutMan.c
    src/opt/cut/cutMerge.c src/opt/cut/cutNode.c src/opt/cut/cutOracle.c src/opt/cut/cutPre22.c src/opt/cut/cutSeq.c src/opt/cut/cutTruth.c 
    src/opt/dec/decAbc.c src/opt/dec/decFactor.c src/opt/dec/decMan.c src/opt/dec/decPrint.c src/opt/dec/decUtil.c  src/opt/fxu/fxu.c 
    src/opt/fxu/fxuCreate.c src/opt/fxu/fxuHeapD.c src/opt/fxu/fxuHeapS.c src/opt/fxu/fxuList.c src/opt/fxu/fxuMatrix.c src/opt/fxu/fxuPair.c
    src/opt/fxu/fxuPrint.c src/opt/fxu/fxuReduce.c src/opt/fxu/fxuSelect.c src/opt/fxu/fxuSingle.c src/opt/fxu/fxuUpdate.c src/opt/rwr/rwrDec.c
    src/opt/rwr/rwrEva.c src/opt/rwr/rwrExp.c src/opt/rwr/rwrLib.c src/opt/rwr/rwrMan.c src/opt/rwr/rwrPrint.c src/opt/rwr/rwrUtil.c 
    src/opt/sim/simMan.c src/opt/sim/simSat.c src/opt/sim/simSeq.c src/opt/sim/simSupp.c src/opt/sim/simSwitch.c src/opt/sim/simSym.c 
    src/opt/sim/simSymSat.c src/opt/sim/simSymSim.c src/opt/sim/simSymStr.c src/opt/sim/simUtils.c src/opt/ret/retArea.c src/opt/ret/retCore.c
    src/opt/ret/retDelay.c src/opt/ret/retFlow.c src/opt/ret/retIncrem.c src/opt/ret/retInit.c src/opt/ret/retLvalue.c src/opt/res/resCore.c 
    src/opt/res/resDivs.c src/opt/res/resFilter.c src/opt/res/resSat.c src/opt/res/resSim.c src/opt/res/resStrash.c src/opt/res/resWin.c 
    src/opt/lpk/lpkCore.c src/opt/lpk/lpkCut.c src/opt/lpk/lpkMan.c src/opt/lpk/lpkMap.c src/opt/lpk/lpkMulti.c src/opt/lpk/lpkMux.c 
    src/opt/lpk/lpkSets.c  src/sat/bsat/satMem.c src/sat/bsat/satInter.c src/sat/bsat/satSolver.c src/sat/bsat/satStore.c src/sat/bsat/satTrace.c
    src/sat/bsat/satUtil.c src/sat/csat/csat_apis.c src/sat/msat/msatActivity.c src/sat/msat/msatClause.c src/sat/msat/msatClauseVec.c 
    src/sat/msat/msatMem.c src/sat/msat/msatOrderJ.c src/sat/msat/msatQueue.c src/sat/msat/msatRead.c src/sat/msat/msatSolverApi.c 
    src/sat/msat/msatSolverCore.c src/sat/msat/msatSolverIo.c src/sat/msat/msatSolverSearch.c src/sat/msat/msatSort.c src/sat/msat/msatVec.c
    src/sat/fraig/fraigApi.c src/sat/fraig/fraigCanon.c src/sat/fraig/fraigFanout.c src/sat/fraig/fraigFeed.c src/sat/fraig/fraigMan.c 
    src/sat/fraig/fraigMem.c src/sat/fraig/fraigNode.c src/sat/fraig/fraigPrime.c src/sat/fraig/fraigSat.c src/sat/fraig/fraigTable.c 
    src/sat/fraig/fraigUtil.c src/sat/fraig/fraigVec.c src/phys/place/place_base.c src/phys/place/place_bin.c src/phys/place/place_genqp.c
    src/phys/place/place_gordian.c src/phys/place/place_legalize.c src/phys/place/place_pads.c src/phys/place/place_partition.c 
    src/phys/place/place_qpsolver.c src/phys/place/place_io.c src/phys/place/place_inc.c)

headers_to_include_dirs(LIB_SOURCES LIB_INCLUDE_DIRS)

#Add the directories with required headers, but no compiled sources
list(APPEND LIB_INCLUDE_DIRS 
    src/misc/vec 
    src/misc/util 
    src/aig/deco)

#Create the library
add_library(libabc STATIC
             ${LIB_SOURCES})
target_include_directories(libabc PUBLIC ${LIB_INCLUDE_DIRS})
target_compile_definitions(libabc PUBLIC -DSIZEOF_VOID_P=${CMAKE_SIZEOF_VOID_P})
set_target_properties(libabc PROPERTIES PREFIX "") #Avoid extra 'lib' prefix



#Specify link-time dependancies
target_link_libraries(libabc ${CMAKE_DL_LIBS})

if(NOT MSVC)
    #MSVC includes all math functionality in the regular standard library,
    #all other *nix-like systems require explicit linkage to libm
    target_link_libraries(libabc m)
endif()

#Create the executable
add_executable(abc ${EXEC_SOURCES})
target_link_libraries(abc 
                        libabc)

install(TARGETS abc libabc DESTINATION bin)


#Disable some problematic errors/warnings in ABC
set(ABC_SUPPRESS_FLAGS -w)
target_compile_options(abc PRIVATE ${ABC_SUPPRESS_FLAGS})
target_compile_options(libabc PRIVATE ${ABC_SUPPRESS_FLAGS})
