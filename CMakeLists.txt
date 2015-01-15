cmake_minimum_required(VERSION 2.8.2)
project(VoxelToy)
set(VERSION 0.0.0)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(GLEW)
find_package(Boost)

include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${GLUT_INCLUDE_DIR})
include_directories(${GLEW_INCLUDE_DIR})
include_directories(${OPENEXR_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR} src)

# OpenEXR libraries ============================================================

find_library(IMF_LIBRARY    IlmImf    ${OPENEXR_LIB_DIR})
find_library(IMATH_LIBRARY  Imath     ${OPENEXR_LIB_DIR})
find_library(HALF_LIBRARY   Half      ${OPENEXR_LIB_DIR})
find_library(IEX_LIBRARY    Iex       ${OPENEXR_LIB_DIR})
find_library(THREAD_LIBRARY IlmThread ${OPENEXR_LIB_DIR})

set(OPENEXR_LIBRARIES 
	${IMF_LIBRARY}
	${IMATH_LIBRARY}
	${HALF_LIBRARY}
	${IEX_LIBRARY}
	${THREAD_LIBRARY})

# Boost libraries ==============================================================

find_library(BOOST_SYSTEM_LIBRARY boost_system ${Boost_LIB_DIR})
find_library(BOOST_THREAD_LIBRARY boost_thread ${Boost_LIB_DIR})

set(BOOST_LIBRARIES 
	${BOOST_SYSTEM_LIBRARY}
	${BOOST_THREAD_LIBRARY})

# 
add_definitions(-DSHADER_DIR=${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/glsl)
# This is apparently a bug with boost and older compilers not defining TIME_UTC
# https://bbs.archlinux.org/viewtopic.php?pid=1126374
add_definitions(-DTIME_UTC=1)

# Source files =================================================================
file(GLOB_RECURSE SOURCES src/*.cpp src/*.cc CACHED )
file(GLOB_RECURSE HEADERS src/*.h src/*.inl CACHED )
file(GLOB_RECURSE VoxelToy_FORMS src/ui/*.ui)
file(GLOB_RECURSE VoxelToy_RESOURCES src/ui/*.qrc)

set(CMAKE_CXX_FLAGS "-fPIC")

# QT Library ===================================================================
# As QT moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (QT4)
	# Compile with QT 4 

	find_package(Qt4 REQUIRED)
	set(QT_USE_QTOPENGL TRUE)
	include(${QT_USE_FILE})
	add_definitions(${QT_DEFINITIONS})
	
	QT4_WRAP_UI(VoxelToy_FORMS_HEADERS ${VoxelToy_FORMS})
	QT4_WRAP_CPP(VoxelToy_HEADERS_MOC ${HEADERS})
	QT4_ADD_RESOURCES(VoxelToy_RESOURCES_RCC ${VoxelToy_RESOURCES})
	set(QT_USE_QTOPENGL TRUE)


else()
	# Compile with QT 5
	add_definitions(-DQT5)

	# Widgets finds its own dependencies (QtGui and QtCore).
	find_package(Qt5Widgets REQUIRED)
	find_package(Qt5OpenGL REQUIRED)

	# Add the include directories for the Qt 5 Widgets module to
	# the compile lines.
	include_directories(${Qt5Widgets_INCLUDE_DIRS})
	include_directories(${Qt5OpenGL_INCLUDE_DIRS})

	# Use the compile definitions defined in the Qt 5 Widgets module
	add_definitions(${Qt5Widgets_DEFINITIONS})
	add_definitions(${Qt5OpenGL_DEFINITIONS})

	# Executables fail to build with Qt 5 in the default configuration
	# without -fPIE. We add that here.
	set(CMAKE_CXX_FLAGS 
		"${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}"
		"${Qt5OpenGL_EXECUTABLE_COMPILE_FLAGS}"
		)
	set(CMAKE_CXX_FLAGS "-fPIE")

	QT5_WRAP_UI(VoxelToy_FORMS_HEADERS ${VoxelToy_FORMS})
	QT5_ADD_RESOURCES(VoxelToy_RESOURCES_RCC ${VoxelToy_RESOURCES})

endif()

# Output executable ============================================================

ADD_EXECUTABLE(VoxelToy ${SOURCES} 
	${VoxelToy_FORMS_HEADERS} 
	${VoxelToy_HEADERS_MOC}
	${VoxelToy_RESOURCES_RCC}
	)

if (QT4)
else()
	qt5_use_modules(VoxelToy Widgets)

	set(QT_LIBRARIES Qt5::Widgets Qt5::OpenGL)
endif()

# Linked libraries =============================================================

target_link_libraries(VoxelToy 
	${GLEW_LIBRARY}
	${GLUT_LIBRARY}
	${OPENGL_LIBRARIES}
	${OPENEXR_LIBRARIES}
	${QT_LIBRARIES}
	${BOOST_LIBRARIES}
	-lpthread)


