#-------------------------------------------------
#
# Project created by QtCreator 2011-07-14T14:45:31
#
#-------------------------------------------------
QT +=\
    core \
    gui \
    widgets

TARGET =\
    blifExplorer

TEMPLATE =\
    app

SOURCES +=\
    main.cpp\
    mainwindow.cpp \
    logicunit.cpp \
    wire.cpp \
    explorerscene.cpp \
    diagramtextitem.cpp \
    container.cpp \
    odininterface.cpp \
    clockconfig.cpp

HEADERS +=\
    mainwindow.h \
    logicunit.h \
    wire.h \
    explorerscene.h \
    diagramtextitem.h \
    container.h \
    odininterface.h \
    clockconfig.h

FORMS +=\
    clockconfig.ui

RESOURCES +=\
    explorerres.qrc

INCLUDEPATH +=\
    ../../libs/libarchfpga/src \
    ../../ODIN_II/SRC/include \
    ../../libs/libvtrutil/src \
    ../../libs/EXTERNAL/libargparse/src

LIBS +=\
    -L../../libarchfpga \
    -L../../ODIN_II/OBJ \
    ../../liblog/obj/log.o \
    -lm -ldl\
