SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += $(wildcard LICENSE*) res

RACK_DIR ?= ../Rack-SDK/
include $(RACK_DIR)/plugin.mk

