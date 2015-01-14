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
    ../../src/renderer.cpp \
    ../../src/voxelizer.cpp

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
    ../../src/renderer.h \
    ../../src/thirdParty/boost/threadpool.hpp \
    ../../src/thirdParty/boost/threadpool/scheduling_policies.hpp \
    ../../src/thirdParty/boost/threadpool/pool.hpp \
    ../../src/thirdParty/boost/threadpool/future.hpp \
    ../../src/thirdParty/boost/threadpool/task_adaptors.hpp \
    ../../src/thirdParty/boost/threadpool/size_policies.hpp \
    ../../src/thirdParty/boost/threadpool/shutdown_policies.hpp \
    ../../src/thirdParty/boost/threadpool/pool_adaptors.hpp \
    ../../src/thirdParty/boost/threadpool/detail/locking_ptr.hpp \
    ../../src/thirdParty/boost/threadpool/detail/pool_core.hpp \
    ../../src/thirdParty/boost/threadpool/detail/worker_thread.hpp \
    ../../src/thirdParty/boost/threadpool/detail/future.hpp \
    ../../src/thirdParty/boost/threadpool/detail/scope_guard.hpp \
    ../../src/voxelizer.h

FORMS    += ../../src/ui/mainwindow.ui \
    ../../src/ui/camerapropertiesui.ui \
    ../../src/ui/renderpropertiesui.ui

LIBS += -lglut -lGLU -lGLEW -lboost_system -lboost_thread

DEFINES += SHADER_DIR=$$PWD/../../src/shaders

# to compile OpenEXR
CONFIG +=link_pkgconfig
PKGCONFIG += OpenEXR

RESOURCES += \
    ../../src/ui/resources.qrc
