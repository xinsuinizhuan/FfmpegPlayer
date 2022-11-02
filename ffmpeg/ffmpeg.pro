include(../libs.pri)
include(../3rdparty/3rdparty.pri)
include(videooutput/videooutput.pri)

QT += widgets multimedia openglwidgets

DEFINES += FFMPEG_LIBRARY
TARGET = $$replaceLibName(ffmpeg)

LIBS += -l$$replaceLibName(utils)

SOURCES += \
    audiodecoder.cpp \
    avaudio.cpp \
    avcontextinfo.cpp \
    averror.cpp \
    codeccontext.cpp \
    decoderaudioframe.cpp \
    decodervideoframe.cpp \
    formatcontext.cpp \
    frameconverter.cc \
    hardwaredecode.cc \
    packet.cpp \
    player.cpp \
    playframe.cpp \
    subtitle.cpp \
    subtitledecoder.cpp \
    videodecoder.cpp

HEADERS += \
    audiodecoder.h \
    avaudio.h \
    avcontextinfo.h \
    averror.h \
    codeccontext.h \
    decoder.h \
    decoderaudioframe.h \
    decodervideoframe.h \
    ffmepg_global.h \
    formatcontext.h \
    frameconverter.hpp \
    hardwaredecode.hpp \
    packet.h \
    player.h \
    playframe.h \
    subtitle.h \
    subtitledecoder.h \
    videodecoder.h
