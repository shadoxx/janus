#-------------------------------------------------
#
# Project created by QtCreator 2011-05-26T17:27:40
#
#-------------------------------------------------

# Define version
__VERSION=62.0

#JamesMcCrae: define this when doing Oculus-submitted builds (either for Rift of GearVR app categories)
#DEFINES += OCULUS_SUBMISSION_BUILD
#Daydream store build
#DEFINES += DAYDREAM_SUBMISSION_BUILD

DEFINES += __JANUS_VERSION=\\\"$$__VERSION\\\"
DEFINES += __JANUS_VERSION_COMPLETE=__JANUS_VERSION\\\".$$system(git --git-dir ./.git --work-tree . describe --always --tags --abbrev=7)\\\"
win32{
DEFINES += RIFT_ID=\\\"$$system(type riftid.txt)\\\"
DEFINES += GEAR_ID=\\\"$$system(type gearid.txt)\\\"
}
!win32{
DEFINES += RIFT_ID=\\\"$$system(cat riftid.txt)\\\"
DEFINES += GEAR_ID=\\\"$$system(cat gearid.txt)\\\"
}

# Default rules for deployment.
android:include(deployment.pri)

# ensure one "debug_and_release" in CONFIG, for clarity...
debug_and_release {
    CONFIG -= debug_and_release
    CONFIG += debug_and_release
}
    # ensure one "debug" or "release" in CONFIG so they can be used as
    #   conditionals instead of writing "CONFIG(debug, debug|release)"...
CONFIG(debug, debug|release) {
    CONFIG -= debug release
    CONFIG += debug
}
CONFIG(release, debug|release) {
    CONFIG -= debug release
    CONFIG += release force_debug_info
# force_debug_info
}

QT       += core opengl gui network xml script scripttools websockets

android{
    QT += androidextras sensors
}

win32-msvc* {
    QMAKE_CXXFLAGS += /wd4251
    QMAKE_CXXFLAGS += /F 32000000
    QMAKE_LFLAGS += /STACK:32000000
    DEFINES += _VARIADIC_MAX=6
}

unix:!macx{
    QMAKE_CXXFLAGS += -Wl,--stack,32000000
    QMAKE_CXXFLAGS += -std=c++11 -Wno-unused-local-typedefs
}

unix:macx{
    QMAKE_CXXFLAGS += -Wl,--stack,32000000
    QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++
    QMAKE_LFLAGS += -stdlib=libc++
}

CONFIG += c++11
CONFIG += qtnamespace
CONFIG += -opengl desktop -no-angle

TARGET = janusvr
TEMPLATE = app

SOURCES += \
    src/abstracthmdmanager.cpp \
    src/abstractrenderer.cpp \
    src/abstractwebsurface.cpp \
    src/abstractwebview.cpp \    
    src/asset.cpp \
    src/assetghost.cpp \
    src/assetimage.cpp \
    src/assetimagedata.cpp \
    src/assetimagedataq.cpp \
    src/assetobject.cpp \
    src/assetrecording.cpp \
    src/assetscript.cpp \
    src/assetshader.cpp \
    src/assetskybox.cpp \
    src/assetsound.cpp \
    src/assetvideo.cpp \    
    src/assetwebsurface.cpp \    
    src/assetwindow.cpp \
    src/audioutil.cpp \
    src/baseassetdata.cpp \
    src/bookmarkmanager.cpp \
    src/cefwebview.cpp \
    src/codeeditorwindow.cpp \
    src/contentimporter.cpp \
    src/controllermanager.cpp \
    src/cookiejar.cpp \    
    src/domnode.cpp \
    src/environment.cpp \
    src/filteredcubemapmanager.cpp \        
    src/game.cpp \
    src/gamepad.c \
    src/geom.cpp \
    src/glwidget.cpp \
    src/hierarchywindow.cpp \
    src/htmlpage.cpp \    
    src/leaphands.cpp \
    src/lightmanager.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/mathutil.cpp \
    src/mediaplayer.cpp \    
    src/menu.cpp \
    src/multiplayermanager.cpp \
    src/navigationwindow.cpp \
    src/particlesystem.cpp \
    src/performancelogger.cpp \
    src/player.cpp \
    src/propertieswindow.cpp \
    src/renderer.cpp \
    src/renderergl33.cpp \
    src/rendererinterface.cpp \    
    src/room.cpp \
    src/roomobject.cpp \
    src/roomphysics.cpp \
    src/roomtemplate.cpp \
    src/scriptablevector.cpp \
    src/scriptablexmlhttprequest.cpp \
    src/scriptbuiltins.cpp \
    src/scriptvalueconversions.cpp \
    src/settingsmanager.cpp \
    src/settingswindow.cpp \
    src/socialwindow.cpp \
    src/soundmanager.cpp \
    src/spinanimation.cpp \
    src/textgeom.cpp \
    src/textureimportercmft.cpp \    
    src/textureimportergli.cpp \
    src/textureimporterqimage.cpp \
    src/texturemanager.cpp \    
    src/virtualkeyboard.cpp \
    src/webasset.cpp

unix:!android:!macx:SOURCES += ./resources/cef/linux/libcef_dll/base/cef_atomicops_x86_gcc.cc

HEADERS += \
    src/abstracthmdmanager.h \
    src/abstractrenderer.h \    
    src/abstractwebsurface.h \
    src/abstractwebview.h \    
    src/asset.h \
    src/assetghost.h \
    src/assetimage.h \
    src/assetimagedata.h \
    src/assetimagedataq.h \
    src/assetobject.h \
    src/assetrecording.h \
    src/assetscript.h \
    src/assetshader.h \
    src/assetskybox.h \
    src/assetsound.h \
    src/assetvideo.h \    
    src/assetwebsurface.h \    
    src/assetwindow.h \
    src/audioutil.h \
    src/baseassetdata.h \
    src/bookmarkmanager.h \
    src/cefwebview.h \
    src/codeeditorwindow.h \
    src/contentimporter.h \
    src/controllermanager.h \
    src/cookiejar.h \    
    src/domnode.h \
    src/environment.h \
    src/filteredcubemapmanager.h \        
    src/game.h \
    src/gamepad.h \
    src/geom.h \
    src/glwidget.h \
    src/hierarchywindow.h \
    src/htmlpage.h \
    src/leaphands.h \
    src/lightmanager.h \
    src/mainwindow.h \
    src/mathutil.h \
    src/mediaplayer.h \    
    src/menu.h \
    src/multiplayermanager.h \
    src/navigationwindow.h \
    src/particlesystem.h \
    src/performancelogger.h \
    src/player.h \
    src/propertieswindow.h \
    src/renderer.h \
    src/renderergl33.h \
    src/rendererinterface.h \    
    src/room.h \
    src/roomobject.h \
    src/roomphysics.h \
    src/roomtemplate.h \
    src/scriptablevector.h \
    src/scriptablexmlhttprequest.h \
    src/scriptbuiltins.h \
    src/scriptvalueconversions.h \
    src/settingsmanager.h \
    src/settingswindow.h \
    src/socialwindow.h \
    src/soundmanager.h \
    src/spinanimation.h \
    src/textgeom.h \
    src/textureimportercmft.h \    
    src/textureimportergli.h \
    src/textureimporterqimage.h \
    src/texturemanager.h \    
    src/virtualkeyboard.h \
    src/webasset.h

# NVIDIA HBAO
#win32:INCLUDEPATH += "./resources/HBAOPlus/include"
#win32:LIBS += -L"$$PWD/resources/HBAOPlus/lib"
#win32:LIBS += "-lGFSDK_SSAO_GL.win64"

# Vive support (if not an Oculus Submission)
!contains(DEFINES, OCULUS_SUBMISSION_BUILD) {
    win32:HEADERS += src/vivemanager.h
    win32:SOURCES += src/vivemanager.cpp
}

unix:!macx:HEADERS += src/vivemanager.h
unix:!macx:SOURCES += src/vivemanager.cpp

#Gamepad not working on OSX
unix:macx:SOURCES -= "src/gamepad.c"
unix:macx:HEADERS -= "src/gamepad.h"

#Remove dependencies unused by Android
android:SOURCES -= "src/gamepad.c" \
    "src/vivemanager.cpp" \    
    "src/cefwebview.cpp" \
    "src/filteredcubemapmanager.cpp" \
    "src/hierarchywindow.cpp" \
    "src/propertieswindow.cpp" \
    "src/assetwindow.cpp" \
    "src/codeeditorwindow.cpp" \
    "src/navigationwindow.cpp" \
    "src/renderergl44_loadingthread.cpp" \
    "src/renderergl44_renderthread.cpp"
android:HEADERS -= "src/gamepad.h" \
    "src/vivemanager.h" \    
    "src/cefwebview.h" \
    "src/filteredcubemapmanager.h" \
    "src/hierarchywindow.h" \
    "src/propertieswindow.h" \
    "src/assetwindow.h" \
    "src/codeeditorwindow.h" \
    "src/navigationwindow.h" \
    "src/renderergl44_loadingthread.h" \
    "src/renderergl44_renderthread.h"
android:SOURCES += "src/androidwebview.cpp" \
    "src/slidingtabwidget.cpp" \
    "src/jniutil.cpp" \
    "src/gvrmanager.cpp"
android:HEADERS += "src/androidwebview.h" \
    "src/slidingtabwidget.h" \
    "src/jniutil.h" \
    "src/gvrmanager.h"

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    android:SOURCES += "src/gearmanager.cpp"
}

contains(ANDROID_TARGET_ARCH,armeabi) {
    android:HEADERS += "src/gearmanager.h"
}

# Chromium Embedded Framework CEF
win32:INCLUDEPATH += "./resources/cef/windows/"
unix:!macx:!android:INCLUDEPATH += "./resources/cef/linux"

win32:LIBS += -L"$$PWD/dependencies/windows/"
unix:!macx:!android:LIBS += -L"$$PWD/dependencies/linux/"

CONFIG(debug) {
    win32:LIBS += -L"$$PWD/resources/cef/windows/lib/debug/"
    unix:!macx:!android:LIBS += -L"$$PWD/resources/cef/linux/lib/debug/"
}
CONFIG(release) {
    win32:LIBS += -L"$$PWD/resources/cef/windows/lib/release/"
    unix:!macx:!android:LIBS += -L"$$PWD/resources/cef/linux/lib/release/"
}

win32:LIBS += -llibcef -llibcef_dll_wrapper
unix:!macx:!android:LIBS += -lcef -lcef_dll_wrapper

#GoogleVR
contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    INCLUDEPATH += "./resources/gvr-android-sdk/libraries/headers"
    LIBS += -L"$$PWD/dependencies/android/armeabi-v7a/"
    LIBS += -lgvr
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi-v7a/libgvr.so
}

contains(ANDROID_TARGET_ARCH,x86) {
    INCLUDEPATH += "./resources/gvr-android-sdk/libraries/headers"
    LIBS += -L"$$PWD/dependencies/android/x86/"
    LIBS += -lgvr
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/x86/libgvr.so
}

#Oculus Mobile
contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    INCLUDEPATH += "./resources/ovr_sdk_mobile/VrApi/Include"

    CONFIG(debug) {
        LIBS += -L"$$PWD/dependencies/android/armeabi-v7a/Debug/"
        LIBS += -lvrapi
        ANDROID_EXTRA_LIBS += \
            $$PWD/dependencies/android/armeabi-v7a/Debug/libvrapi.so
    }
    CONFIG(release) {
        LIBS += -L"$$PWD/dependencies/android/armeabi-v7a/Release/"
        LIBS += -lvrapi
        ANDROID_EXTRA_LIBS += \
            $$PWD/dependencies/android/armeabi-v7a/Release/libvrapi.so
    }
}

contains(ANDROID_TARGET_ARCH,armeabi) {
    INCLUDEPATH += "./resources/ovr_sdk_mobile/VrApi/Include"

    CONFIG(debug) {
        LIBS += -L"$$PWD/dependencies/android/armeabi/Debug/"
        LIBS += -lvrapi
        ANDROID_EXTRA_LIBS += \
            $$PWD/dependencies/android/armeabi/Debug/libvrapi.so
    }
    CONFIG(release) {
        LIBS += -L"$$PWD/dependencies/android/armeabi/Release/"
        LIBS += -lvrapi
        ANDROID_EXTRA_LIBS += \
            $$PWD/dependencies/android/armeabi/Release/libvrapi.so
    }
}

# cmft: Cross-platform open-source command-line cubemap filtering tool.
win32:INCLUDEPATH += "./resources/cmft/dependency/bx/include/compat/msvc"
unix:macx:INCLUDEPATH += "./resources/cmft/dependency/bx/include/compat/osx"
INCLUDEPATH += "./resources/cmft/dependency"
INCLUDEPATH += "./resources/cmft/dependency/bx/include"
INCLUDEPATH += "./resources/cmft/dependency/bx/3rdparty"
INCLUDEPATH += "./resources/cmft/dependency/cl/include"
INCLUDEPATH += "./resources/cmft/dependency/dm/include"
INCLUDEPATH += "./resources/cmft/dependency/stb"
INCLUDEPATH += "./resources/cmft/include"
win32:LIBS += -L"$$PWD/resources/cmft/lib"
unix:!macx:!android:LIBS += -L"$$PWD/resources/cmft/lib"
unix:macx:LIBS += -L"$$PWD/resources/cmft/lib/Mac"

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    LIBS += -L"$$PWD/dependencies/android/armeabi-v7a/"
    LIBS += -lcmft
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi-v7a/libcmft.so
}

contains(ANDROID_TARGET_ARCH,armeabi) {
    LIBS += -L"$$PWD/dependencies/android/armeabi/"
    LIBS += -lcmft
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi/libcmft.so
}

contains(ANDROID_TARGET_ARCH,x86) {
    LIBS += -L"$$PWD/dependencies/android/x86/"
    LIBS += -lcmft
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/x86/libcmft.so
}

CONFIG(debug) {
    win32:LIBS += -lcmftDebug
    unix:macx:LIBS += -lcmftDebug
    unix:!macx:!android:LIBS += -lcmftRelease
}
CONFIG(release) {
    !android:LIBS += -lcmftRelease
}

# gli
INCLUDEPATH += "./resources/gli"
INCLUDEPATH += "./resources/glm"

# half
INCLUDEPATH += "./resources/half_1.12/include"

#OVR Platform on Windows
win32:HEADERS += src/riftmanager.h
win32:SOURCES += src/riftmanager.cpp

# OVR Platform SDK (essential for Oculus Home build)
win32:SOURCES += "./resources/OVRPlatformSDK_v1.24.0/Windows/OVR_PlatformLoader.cpp"
win32:INCLUDEPATH += "./resources/OVRPlatformSDK_v1.24.0/Include" #note that Windows version is built against latest version (0.8+) of LibOVR
win32:LIBS += -L"$$PWD/resources/OVRPlatformSDK_v1.24.0/Windows"
win32:LIBS += -lLibOVRPlatform64_1

#OVR Platform on Android
android:INCLUDEPATH += "./resources/OVRPlatformSDK_v1.24.0/Include" #note that Windows version is built against latest version (0.8+) of LibOVR
contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    LIBS += -L"$$PWD/dependencies/android/armeabi-v7a/"
    LIBS += -lovrplatformloader
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi-v7a/libovrplatformloader.so
}

contains(ANDROID_TARGET_ARCH,armeabi) {
    LIBS += -L"$$PWD/dependencies/android/armeabi/"
    LIBS += -lovrplatformloader
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi/libovrplatformloader.so
}

# LibOVR
win32:INCLUDEPATH += "./resources/LibOVR_v1.24.0/Include" #note that Windows version is built against latest version (0.8+) of LibOVR
CONFIG(debug) {
    win32:LIBS += -L"$$PWD/resources/LibOVR_v1.24.0/Lib/Windows/x64/Debug/VS2015"
}
CONFIG(release) {
    win32:LIBS += -L"$$PWD/resources/LibOVR_v1.24.0/Lib/Windows/x64/Release/VS2015"
}
win32:LIBS += -llibOVR

# openVR (note that we only include it if OCULUS_SUBMISSION_BUILD is not defined)
!contains(DEFINES, OCULUS_SUBMISSION_BUILD) {
win32:INCLUDEPATH += "./resources/openvr/headers"
win32:LIBS += -L"$$PWD/resources/openvr/lib/win64"
win32:LIBS += -lopenvr_api
}
unix:!macx:!android:INCLUDEPATH += "./resources/openvr/headers"
unix:!macx:!android:LIBS += -L"$$PWD/resources/openvr/lib/linux64"
unix:!macx:!android:LIBS += -lopenvr_api

# OpenAL
INCLUDEPATH += "./resources/openal-soft-1.17.2/include"
win32:LIBS += -L"$$PWD/resources/openal-soft-1.17.2/libs/Win64"
win32:LIBS += -lOpenAL32
unix:macx:LIBS += -L"$$PWD/resources/openal-soft-1.17.2/bin"
unix:macx:LIBS += -lopenal

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    LIBS += -L"$$PWD/dependencies/android/armeabi-v7a/"
    LIBS += -lopenal
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi-v7a/libopenal.so
}

contains(ANDROID_TARGET_ARCH,armeabi) {
    LIBS += -L"$$PWD/dependencies/android/armeabi/"
    LIBS += -lopenal
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi/libopenal.so
}

contains(ANDROID_TARGET_ARCH,x86) {
    LIBS += -L"$$PWD/dependencies/android/x86/"
    LIBS += -lopenal
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/x86/libopenal.so
}

# OpenSSL
INCLUDEPATH +="./resources/openssl_win64/include"
win32:LIBS += -L"$$PWD/resources/openssl_win64/libs"
win32:LIBS += -llibeay32 -lssleay32
OPENSSL_LIBS ='-L"$$PWD/resources/openssl_win64/libs" -llibeay32 -lssleay32'
CONFIG += openssl-linked

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    INCLUDEPATH +="./resources/openssl_android/openssl-1.0.2l_arm/include"
    LIBS += -L"$$PWD/dependencies/android/armeabi-v7a/"
    LIBS += -lcrypto -lssl
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi-v7a/libcrypto.so \
        $$PWD/dependencies/android/armeabi-v7a/libssl.so
}

contains(ANDROID_TARGET_ARCH,armeabi) {
    INCLUDEPATH +="./resources/openssl_android/openssl-1.0.2l_arm/include"
    LIBS += -L"$$PWD/dependencies/android/armeabi/"
    LIBS += -lcrypto -lssl
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi/libcrypto.so \
        $$PWD/dependencies/android/armeabi/libssl.so
}

contains(ANDROID_TARGET_ARCH,x86) {
    INCLUDEPATH +="./resources/openssl_android/openssl-1.0.2l_x86/include"
    LIBS += -L"$$PWD/dependencies/android/x86/"
    LIBS += -lcrypto -lssl
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/x86/libcrypto.so \
        $$PWD/dependencies/android/x86/libssl.so
}

# Opus
INCLUDEPATH +="./resources/opus-1.2.1/include"
CONFIG(debug) {
    win32:LIBS += -L"$$PWD/resources/opus-1.2.1/win32/VS2015/x64/debug"
}
CONFIG(release) {
    win32:LIBS += -L"$$PWD/resources/opus-1.2.1/win32/VS2015/x64/release"
}
unix:macx:LIBS += -L"/usr/local/lib" #used for Opus - OSX
!android:LIBS += -lopus

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    LIBS += -L"$$PWD/dependencies/android/armeabi-v7a/"
    LIBS += -lopus
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi-v7a/libopus.so
}

contains(ANDROID_TARGET_ARCH,armeabi) {
    LIBS += -L"$$PWD/dependencies/android/armeabi/"
    LIBS += -lopus
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi/libopus.so
}

contains(ANDROID_TARGET_ARCH,x86) {
    LIBS += -L"$$PWD/dependencies/android/x86/"
    LIBS += -lopus
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/x86/libopus.so
}

# VLC
win32:INCLUDEPATH +="./resources/libvlc/include"
macx:INCLUDEPATH +="./resources/libvlc/include"
unix:android:INCLUDEPATH +="./resources/libvlc/include"

win32:LIBS += -L"$$PWD/resources/libvlc/libs"
macx:LIBS += -L"$$PWD/resources/libvlc/libs/Mac/lib"
unix:android:LIBS += -L"$$PWD/resources/libvlc/libs"

win32:LIBS += -lvlc
unix:!android:LIBS += -lvlc

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    LIBS += -L"$$PWD/dependencies/android/armeabi-v7a/"
    LIBS += -lvlcjni
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi-v7a/libvlcjni.so
}

contains(ANDROID_TARGET_ARCH,armeabi) {
    LIBS += -L"$$PWD/dependencies/android/armeabi/"
    LIBS += -lvlcjni
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi/libvlcjni.so
}

contains(ANDROID_TARGET_ARCH,x86) {
    LIBS += -L"$$PWD/dependencies/android/x86/"
    LIBS += -lvlcjni
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/x86/libvlcjni.so
}

# Bullet Physics - on Linux, this is 2.87 and installed via libbullet-dev
win32:INCLUDEPATH +="./resources/bullet3/src"
CONFIG(debug) {
    win32:LIBS += -L"$$PWD/resources/bullet3/lib/Debug"
}
CONFIG(release) {
    win32:LIBS += -L"$$PWD/resources/bullet3/lib/Release"
}
win32:LIBS += -lBulletDynamics -lBulletCollision -lLinearMath

android:INCLUDEPATH +="./resources/bullet3/src"
unix:!macx:!android:INCLUDEPATH += "/usr/include/bullet"
unix:macx:INCLUDEPATH +="./resources/bullet3/src"
unix:macx:LIBS += -L"/usr/local/lib" #used for Bullet - OSX
unix:!android:LIBS += -lBulletDynamics -lBulletCollision -lLinearMath

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    LIBS += -L"$$PWD/dependencies/android/armeabi-v7a/"
    LIBS += -lBullet
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi-v7a/libBullet.so
}

contains(ANDROID_TARGET_ARCH,armeabi) {
    LIBS += -L"$$PWD/dependencies/android/armeabi/"
    LIBS += -lBullet
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi/libBullet.so
}

contains(ANDROID_TARGET_ARCH,x86) {
    LIBS += -L"$$PWD/dependencies/android/x86/"
    LIBS += -lBullet
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/x86/libBullet.so
}

# Assimp
INCLUDEPATH += "./resources/assimp/include"

CONFIG(debug) {
    LIBS += -L"$$PWD/resources/assimp/lib/Debug"
    win32:LIBS += -lassimp-vc140-mt -lIrrXML
}
CONFIG(release) {
    LIBS += -L"$$PWD/resources/assimp/lib/Release"
    win32:LIBS += -lassimp-vc140-mt -lIrrXML
}
LIBS += -L"$$PWD/resources/assimp/lib"
unix:macx:LIBS += -L"$$PWD/resources/assimp/lib/Mac"
unix:!android:LIBS += -lassimp

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    LIBS += -L"$$PWD/dependencies/android/armeabi-v7a/"
    LIBS += -lassimp
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi-v7a/libassimp.so
}

contains(ANDROID_TARGET_ARCH,armeabi) {
    LIBS += -L"$$PWD/dependencies/android/armeabi/"
    LIBS += -lassimp
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi/libassimp.so
}

contains(ANDROID_TARGET_ARCH,x86) {
    LIBS += -L"$$PWD/dependencies/android/x86/"
    LIBS += -lassimp
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/x86/libassimp.so
}

# Generic Windows libs
win32:LIBS += -lopengl32 -lglu32 -ladvapi32 -lwinmm

# Other Linux libs (need to be separately installed via apt, etc.)
unix:!macx:!android:LIBS += -lX11 -ludev -lGLU -lrt -ldl -lopenal -lz #-lXrandr -lXinerama
unix:!macx:!android:{
   QMAKE_LFLAGS += -Wl,-rpath,"'\$$ORIGIN'"
}

unix:macx:LIBS += -lz -framework IOKit -framework CoreFoundation
unix:macx:QMAKE_LFLAGS += -F"$$PWD/resources/qtpdf/lib/mac"
unix:macx:LIBS += -framework QtPdf
unix:macx:{
   QMAKE_LFLAGS += -Wl,-rpath,"@loader_path/../Frameworks"
}

android:LIBS += -ljnigraphics -landroid -lz -lOpenSLES -lEGL

#C++ STL
contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    LIBS += -L"$$PWD/dependencies/android/armeabi-v7a/"
    LIBS += -lc++_shared
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi-v7a/libc++_shared.so
}

contains(ANDROID_TARGET_ARCH,armeabi) {
    LIBS += -L"$$PWD/dependencies/android/armeabi/"
    LIBS += -lc++_shared
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/armeabi/libc++_shared.so
}

contains(ANDROID_TARGET_ARCH,x86) {
    LIBS += -L"$$PWD/dependencies/android/x86/"
    LIBS += -lc++_shared
    ANDROID_EXTRA_LIBS += \
        $$PWD/dependencies/android/x86/libc++_shared.so
}

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    INCLUDEPATH += "./resources/androidAPI21/arch-arm/usr/include"
}

contains(ANDROID_TARGET_ARCH,armeabi) {
    INCLUDEPATH += "./resources/androidAPI21/arch-arm/usr/include"
}

contains(ANDROID_TARGET_ARCH,x86) {
    INCLUDEPATH += "./resources/androidAPI21/arch-x86/usr/include"
}

#icon
win32:RC_FILE = janusvr.rc
unix:macx:ICON = janusvr.icns

RESOURCES +=

DISTFILES +=

android:DISTFILES += \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/libs/* \
    android/AndroidManifest.xml \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/src/org/janus/JanusActivity.java \
    android/src/org/janus/WebViewManager.java \
    android/src/org/janus/GearManager.java \
    android/src/org/janus/GVRManager.java \
    android/src/org/janus/JoystickView.java \
    android/src/org/janus/ButtonView.java \
    android/src/org/janus/SplashView.java

ASSETS.path = /assets
ASSETS.files = $$files($$PWD/android/assets/*)
INSTALLS += ASSETS

# OpenGL
DEFINES += OPENGL

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
