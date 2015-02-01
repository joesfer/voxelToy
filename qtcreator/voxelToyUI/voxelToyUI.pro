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
			../../src/camera/camera.cpp \
			../../src/shaders/shader.cpp \
			../../src/renderer/noise.cpp \
			../../src/renderer/content.cpp \
    ../../src/ui/glwidget.cpp \
    ../../src/ui/camerapropertiesui.cpp \
    ../../src/ui/renderpropertiesui.cpp \
    ../../src/mesh/mesh.cpp \
    ../../src/mesh/meshLoader.cpp \
    ../../src/thirdParty/tinyobjloader/tiny_obj_loader.cc \
    ../../src/renderer/renderer.cpp \
    ../../src/svo/voxelizer.cpp \
    ../../src/timer/gpuTimer.cpp \
    ../../src/camera/orbitCameraController.cpp \
    ../../src/camera/flyCameraController.cpp \
    ../../src/camera/cameraParameters.cpp \
    ../../src/camera/cameraController.cpp \
    ../../src/renderer/voxLoader.cpp \
    ../../src/tools/toolFocalDistance.cpp \
    ../../src/tools/toolAddVoxel.cpp

HEADERS  += ../../src/ui/mainwindow.h \
    ../../src/camera/camera.h \
    ../../src/shaders/shader.h \
    ../../src/renderer/noise.h \
        ../../src/renderer/content.h \
    ../../src/ui/glwidget.h \
    ../../src/ui/camerapropertiesui.h \
    ../../src/ui/renderpropertiesui.h \
    ../../src/mesh/mesh.h \
    ../../src/mesh/meshLoader.h \
    ../../src/thirdParty/tinyobjloader/tiny_obj_loader.h \
    ../../src/renderer/renderer.h \
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
    ../../src/svo/voxelizer.h \
    ../../src/timer/gpuTimer.h

FORMS    += ../../src/ui/mainwindow.ui \
    ../../src/ui/camerapropertiesui.ui \
    ../../src/ui/renderpropertiesui.ui

LIBS += -lglut -lGLU -lGLEW -lboost_system -lboost_thread

DEFINES += SHADER_DIR=$$PWD/../../src/shaders/glsl
DEFINES += QT5

# to compile OpenEXR
CONFIG +=link_pkgconfig
PKGCONFIG += OpenEXR

RESOURCES += \
    ../../src/ui/resources.qrc \
    ../../src/ui/qdarkstyle/style.qrc

OTHER_FILES +=
