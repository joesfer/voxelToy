cmake_minimum_required(VERSION 2.8.2)
project(VoxelToy)
set(VERSION 0.0.0)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

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

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(Boost REQUIRED)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} 
					src)

add_definitions(-DSHADER_DIR=${CMAKE_CURRENT_SOURCE_DIR}/src/shaders)

set(CMAKE_CXX_FLAGS "-fPIC")

file(GLOB_RECURSE SOURCES src/*.cpp CACHED )
file(GLOB_RECURSE HEADERS src/*.h src/*.inl CACHED )

file(GLOB_RECURSE VoxelToy_FORMS src/ui/*.ui)
file(GLOB_RECURSE VoxelToy_RESOURCES src/ui/*.qrc)

SET(QT_USE_QTOPENGL TRUE)

QT5_WRAP_UI(VoxelToy_FORMS_HEADERS ${VoxelToy_FORMS})
QT5_ADD_RESOURCES(VoxelToy_RESOURCES_RCC ${VoxelToy_RESOURCES})

ADD_EXECUTABLE(VoxelToy ${SOURCES} 
	${VoxelToy_FORMS_HEADERS} 
	${VoxelToy_RESOURCES_RCC}
	)

qt5_use_modules(VoxelToy Widgets)

TARGET_LINK_LIBRARIES(VoxelToy 
	Qt5::Widgets
	Qt5::OpenGL
	-lGL  -lGLEW -lglut
	-lIlmImf -lImath -lHalf -lIex -lIlmThread -pthread)

