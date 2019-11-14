TEMPLATE = app
CONFIG += c++11
CONFIG -= console
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += $$PWD/../Redopera/Redlib

CONFIG(debug, debug|release){
    DESTDIR = $$PWD/bin_debug
} else {
    DEFINES += R_NO_DEBUG
    DESTDIR = bin_release
}

unix{
unix:!macx:
LIBS += -lglfw \
    -ldl \
}

SOURCES += \
        Redlib/Extern/glad.c \
        Redlib/Extern/stb_image_implementatio.cpp \
        Redlib/RController.cpp \
        Redlib/RDebug.cpp \
        Redlib/RInputEvent.cpp \
        Redlib/RPoint.cpp \
        Redlib/RResource/RImage.cpp \
        Redlib/RResource/RResource.cpp \
        Redlib/RWindowCtrl.cpp \
        Testctr.cpp \
        main.cpp

HEADERS += \
    Redlib/Extern/sigslot.h \
    Redlib/Extern/stb_image.h \
    Redlib/RController.h \
    Redlib/RDebug.h \
    Redlib/RInputEvent.h \
    Redlib/RNotifyEvent.h \
    Redlib/ROpenGL.h \
    Redlib/RPoint.h \
    Redlib/RResource/RImage.h \
    Redlib/RResource/RResource.h \
    Redlib/RWindowCtrl.h \
    Testctr.h

DISTFILES += \
    Resource/StackList.txt
