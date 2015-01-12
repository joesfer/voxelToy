#-------------------------------------------------
#
# Project created by QtCreator 2014-11-08T09:49:53
#
#-------------------------------------------------

QT       += core gui opengl openglextensions

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = voxelToyUI
TEMPLATE = app

INCLUDEPATH += ../../src

SOURCES += ../../src/ui/mainwindow.cpp \
			../../src/main.cpp \
			../../src/camera.cpp \
			../../src/shader.cpp \
			../../src/noise.cpp \
			../../src/content.cpp \
    ../../src/ui/glwidget.cpp \
    ../../src/ui/camerapropertiesui.cpp \
    ../../src/ui/renderpropertiesui.cpp \
    ../../src/mesh.cpp \
    ../../src/meshLoader.cpp \
    ../../src/thirdParty/tinyobjloader/tiny_obj_loader.cc \
    ../../src/renderer.cpp

HEADERS  += ../../src/ui/mainwindow.h \
    ../../src/camera.h \
    ../../src/shader.h \
    ../../src/noise.h \
        ../../src/content.h \
    ../../src/ui/glwidget.h \
    ../../src/ui/camerapropertiesui.h \
    ../../src/ui/renderpropertiesui.h \
    ../../src/mesh.h \
    ../../src/meshLoader.h \
    ../../src/thirdParty/tinyobjloader/tiny_obj_loader.h \
    ../../src/renderer.h

FORMS    += ../../src/ui/mainwindow.ui \
    ../../src/ui/camerapropertiesui.ui \
    ../../src/ui/renderpropertiesui.ui

LIBS += -lglut -lGLU -lGLEW

DEFINES += SHADER_DIR=$$PWD/../../src/shaders

# to compile OpenEXR
CONFIG +=link_pkgconfig
PKGCONFIG += OpenEXR

RESOURCES += \
    ../../src/ui/resources.qrc
