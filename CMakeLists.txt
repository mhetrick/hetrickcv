cmake_minimum_required(VERSION 3.16)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
project(HetrickCV VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PLUGIN_NAME ${PROJECT_NAME})
set(ADDITIONAL_PLUGIN_DISTRIBUTABLES res LICENSE.txt README.md)
include(RackSDK.cmake)

message(STATUS "HetrickCV for Rack Build Process" )
message(STATUS "Special Thanks to baconpaul" )
message(STATUS "Installing into '${CMAKE_INSTALL_PREFIX}'")

file(GLOB SOURCES src/*.cpp src/DSP/*.cpp)
add_compile_options(-fvisibility=hidden -fvisibility-inlines-hidden)

target_include_directories(${RACK_PLUGIN_LIB} PRIVATE src)
target_sources(${RACK_PLUGIN_LIB} PRIVATE
        ${SOURCES})

set(GammaFiles
    /Gamma/src/arr.cpp
    /Gamma/src/DFT.cpp
    /Gamma/src/Domain.cpp
    /Gamma/src/FFT_fftpack.cpp
    /Gamma/src/fftpack++1.cpp
    /Gamma/src/fftpack++2.cpp
    /Gamma/src/scl.cpp
)