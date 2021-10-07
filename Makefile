# FLAGS will be passed to both the C and C++ compiler
FLAGS += -I Gamma
CFLAGS +=
CXXFLAGS +=

SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/DSP/*.cpp)
SOURCES += Gamma/src/arr.cpp
SOURCES += Gamma/src/Domain.cpp
SOURCES += Gamma/src/scl.cpp

DISTRIBUTABLES += $(wildcard LICENSE*) res

RACK_DIR ?= ../Rack-SDK
include $(RACK_DIR)/plugin.mk

