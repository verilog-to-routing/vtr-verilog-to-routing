#-------------------------------------------------
#
# Project created by QtCreator 2011-07-14T14:45:31
#
#-------------------------------------------------

QT       += core gui

TARGET = blifExplorer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    logicunit.cpp \
    wire.cpp \
    explorerscene.cpp \
    diagramtextitem.cpp \
    container.cpp \
    odininterface.cpp \
    clockconfig.cpp

HEADERS  += mainwindow.h \
    logicunit.h \
    wire.h \
    explorerscene.h \
    diagramtextitem.h \
    container.h \
    odininterface.h \
    ../../libarchfpga/include/util.h \
    ../../libarchfpga/include/read_xml_util.h \
    ../../libarchfpga/include/read_xml_arch_file.h \
    ../../libarchfpga/include/ReadLine.h \
    ../../libarchfpga/include/physical_types.h \
    ../../libarchfpga/include/logic_types.h \
    ../../libarchfpga/include/ezxml.h \
    ../../libarchfpga/include/arch_types.h \
    ../../ODIN_II/SRC/output_graphcrunch_format.h \
    ../../ODIN_II/SRC/output_blif.h \
    ../../ODIN_II/SRC/odin_util.h \
    ../../ODIN_II/SRC/node_creation_library.h \
    ../../ODIN_II/SRC/netlist_visualizer.h \
    ../../ODIN_II/SRC/netlist_utils.h \
    ../../ODIN_II/SRC/netlist_stats.h \
    ../../ODIN_II/SRC/netlist_optimizations.h \
    ../../ODIN_II/SRC/netlist_create_from_ast.h \
    ../../ODIN_II/SRC/netlist_check.h \
    ../../ODIN_II/SRC/multipliers.h \
    ../../ODIN_II/SRC/memories.h \
    ../../ODIN_II/SRC/high_level_data.h \
    ../../ODIN_II/SRC/hashtable.h \
    ../../ODIN_II/SRC/hard_blocks.h \
    ../../ODIN_II/SRC/globals.h \
    ../../ODIN_II/SRC/errors.h \
    ../../ODIN_II/SRC/ast_util.h \
    ../../ODIN_II/SRC/ast_optimizations.h \
    ../../ODIN_II/SRC/activity_estimation.h \
    ../../ODIN_II/SRC/odin_ii_func.h \
    ../../ODIN_II/SRC/adders.h \
    ../../ODIN_II/SRC/implicit_memory.h \
    ../../ODIN_II/SRC/subtractions.h \
    ../../printhandler/SRC/TIO_InputOutputHandlers/TIO_PrintHandlerExtern.h \
    ../../printhandler/SRC/TIO_InputOutputHandlers/TIO_SkinHandler.h \
    ../../printhandler/SRC/TIO_InputOutputHandlers/TIO_PrintHandler.h \
    ../../printhandler/SRC/TIO_InputOutputHandlers/TIO_FileOutput.h \
    ../../printhandler/SRC/TIO_InputOutputHandlers/TIO_CustomOutput.h \
    ../../printhandler/SRC/TIO_InputOutputHandlers/TIO_FileHandler.h \
    ../../printhandler/SRC/TC_Common/TC_Name.h \
    ../../printhandler/SRC/TC_Common/TC_StringUtils.h \
    ../../printhandler/SRC/TC_Common/RegExp.h \
    clockconfig.h


FORMS    += \
    clockconfig.ui

RESOURCES += \
    explorerres.qrc

INCLUDEPATH += ../../libarchfpga/include ../../libarchfpga ../../ODIN_II/SRC ../../printhandler/SRC/TIO_InputOutputHandlers/ ../../printhandler

LIBS += -L../../libarchfpga \
-L../../ODIN_II/OBJ \
-lm -ldl\
\
../../ODIN_II/OBJ/odin_ii_func.o \
../../ODIN_II/OBJ/netlist_visualizer.o \
../../ODIN_II/OBJ/multipliers.o \
../../ODIN_II/OBJ/partial_map.o \
../../ODIN_II/OBJ/errors.o \
../../ODIN_II/OBJ/node_creation_library.o \
../../ODIN_II/OBJ/netlist_utils.o \
../../ODIN_II/OBJ/netlist_stats.o \
../../ODIN_II/OBJ/odin_util.o \
../../ODIN_II/OBJ/string_cache.o \
../../ODIN_II/OBJ/hard_blocks.o \
../../ODIN_II/OBJ/memories.o \
../../ODIN_II/OBJ/queue.o \
../../ODIN_II/OBJ/hashtable.o \
../../ODIN_II/OBJ/simulate_blif.o\
../../ODIN_II/OBJ/print_netlist.o \
\
../../ODIN_II/OBJ/read_xml_config_file.o \
../../ODIN_II/OBJ/outputs.o \
../../ODIN_II/OBJ/parse_making_ast.o \
../../ODIN_II/OBJ/ast_util.o \
../../ODIN_II/OBJ/high_level_data.o \
../../ODIN_II/OBJ/ast_optimizations.o \
../../ODIN_II/OBJ/netlist_create_from_ast.o \
../../ODIN_II/OBJ/netlist_optimizations.o \
../../ODIN_II/OBJ/output_blif.o \
../../ODIN_II/OBJ/netlist_check.o \
../../ODIN_II/OBJ/activity_estimation.o \
../../ODIN_II/OBJ/read_netlist.o \
../../ODIN_II/OBJ/read_blif.o \
../../ODIN_II/OBJ/output_graphcrunch_format.o \
../../ODIN_II/OBJ/verilog_preprocessor.o \
../../ODIN_II/OBJ/verilog_bison.o \
../../ODIN_II/OBJ/verilog_flex.o \
../../ODIN_II/OBJ/adders.o \
../../ODIN_II/OBJ/implicit_memory.o \
../../ODIN_II/OBJ/subtractions.o \
\
/usr/lib/x86_64-linux-gnu/libdl.so \
../../libarchfpga/ezxml.o \
../../libarchfpga/read_xml_arch_file.o \
../../libarchfpga/read_xml_util.o \
../../libarchfpga/util.o\
../../libarchfpga/ReadLine.o \
../../printhandler/OBJ/TIO_InputOutputHandlers/TIO_PrintHandlerExtern.o \
../../printhandler/OBJ/TIO_InputOutputHandlers/TIO_SkinHandler.o \
../../printhandler/OBJ/TIO_InputOutputHandlers/TIO_PrintHandler.o \
../../printhandler/OBJ/TIO_InputOutputHandlers/TIO_FileOutput.o \
../../printhandler/OBJ/TIO_InputOutputHandlers/TIO_CustomOutput.o \
../../printhandler/OBJ/TIO_InputOutputHandlers/TIO_FileHandler.o \
../../printhandler/OBJ/TC_Common/TC_Name.o \
../../printhandler/OBJ/TC_Common/TC_StringUtils.o \
../../printhandler/OBJ/TC_Common/RegExp.o \
