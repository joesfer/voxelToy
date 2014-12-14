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

SOURCES += ../../src/mainwindow.cpp \
    ../../src/main.cpp \
    ../../src/glwidget.cpp \
    ../../src/camera.cpp \
    ../../src/shader.cpp \
    ../../src/noise.cpp \
        ../../src/content.cpp

HEADERS  += ../../src/mainwindow.h \
    ../../src/glwidget.h \
    ../../src/camera.h \
    ../../src/shader.h \
    ../../src/noise.h \
        ../../src/content.h

FORMS    += ../../src/mainwindow.ui

LIBS += -lglut -lGLU -lGLEW

DEFINES += SHADER_DIR=$$PWD/../../resources

# to compile OpenEXR
CONFIG +=link_pkgconfig
PKGCONFIG += OpenEXR
