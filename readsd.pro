QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

DEFINES += SOURCE_PATH=$$PWD
DEFINES += TARGI=$$TARGET

#No debug output in release mode
CONFIG(release, debug|release): DEFINES += QT_NO_DEBUG_OUTPUT

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        helpers/coreappworker.cpp \
        helpers/logger.cpp \
        helpers/mounthelper.cpp \
        helpers/processhelper.cpp \
        helpers/signalhelper.cpp \
        helpers/textfilehelper.cpp \
        helpers/userhelper.cpp \
        main.cpp \
        work1.cpp

HEADERS += \
    helpers/coreappworker.h \
    helpers/logger.h \
    helpers/mounthelper.h \
    helpers/processhelper.h \
    helpers/signalhelper.h \
    helpers/stringify.h \
    helpers/textfilehelper.h \
    helpers/userhelper.h \
    typekey.h \
    work1.h



contains(QMAKESPEC,.*linux-rasp-pi\d*-.*){
    message(rpi detected)
    CONFIG += rpi
    DEFINES += RPI
}

unix:rpi:
{
    LIBS += -L/home/zoli/pi4_bullseye/sysroot/usr/lib/arm-linux-gnueabihf
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:rpi: target.path = /home/pi/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


