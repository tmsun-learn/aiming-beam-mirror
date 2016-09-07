#-------------------------------------------------
#
# Project created by QtCreator 2016-06-28T17:16:28
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AimingBeam
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    tcp_ip.cpp \
    guiupdater.cpp \
    camera.cpp \
    videopointgrey.cpp \
    imageacquisition.cpp \
    threadwrapper.cpp \
    overlay.cpp \
    segmentation.cpp \
    errorhandler.cpp \
    laguerredeconvolution.cpp \
    deconvolveprocess.cpp


INCLUDEPATH += "C:\Opt\Boost\include\boost-1_55"
INCLUDEPATH += "C:\Opt\OpenCV\include"
INCLUDEPATH += "C:\Program Files\IDS\uEye\Develop\include"
INCLUDEPATH += "C:\Opt\Boost\include\boost-1_55"
INCLUDEPATH += "C:\Opt\FlyCapture2\include"

# openCV
LIBS += -LC:\Opt\OpenCV\x64\vc12\lib -lopencv_highgui300
LIBS += -LC:\Opt\OpenCV\x64\vc12\lib -lopencv_core300
LIBS += -LC:\Opt\OpenCV\x64\vc12\lib -lopencv_imgproc300
LIBS += -LC:\Opt\OpenCV\x64\vc12\lib -lopencv_videoio300
LIBS += -LC:\Opt\OpenCV\x64\vc12\lib -lopencv_imgcodecs300

# uEye camera
LIBS += -L"C:\Program Files\IDS\uEye\Develop\Lib" -luEye_api_64
LIBS += -L"C:\Program Files\IDS\uEye\Develop\Lib" -luEye_tools_64

# new camera
LIBS += -LC:\Opt\FlyCapture2\lib64 -lFlyCapture2

# boost
LIBS += -LC:\Opt\Boost\lib -llibboost_system-vc120-mt-gd-1_55
LIBS += -LC:\Opt\Boost\lib -llibboost_filesystem-vc120-mt-gd-1_55
LIBS += -LC:\Opt\Boost\lib -llibboost_thread-vc120-mt-gd-1_55
#LIBS += -LC:\Opt\Boost\lib -llibboost_system-vc120-mt-1_55
#LIBS += -LC:\Opt\Boost\lib -llibboost_filesystem-vc120-mt-1_55
#LIBS += -LC:\Opt\Boost\lib -llibboost_thread-vc120-mt-1_55

LIBS += -lWs2_32
LIBS += -lMswsock


HEADERS  += mainwindow.h \
    tcp_ip.h \
    guiupdater.h \
    camera.h \
    videoinput.h \
    videopointgrey.h \
    imageacquisition.h \
    threadwrapper.h \
    overlay.h \
    segmentation.h \
    errorhandler.h \
    laguerredeconvolution.h \
    deconvolveprocess.h

FORMS    += mainwindow.ui