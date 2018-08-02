#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_PATH   := $(shell pwd)
WORKSPACE_PATH := $(shell dirname $(PROJECT_PATH))
#IDF_PATH       = $(WORKSPACE_PATH)/library/esp-idf
BUILD_DIR_BASE = $(PROJECT_PATH)/build

PROJECT_NAME := smart_light_switch

export IDF_PATH
export PROJECT_PATH
export WORKSPACE_PATH
export PROJECT_NAME
export EXTRA_CFLAGS

include $(IDF_PATH)/make/project.mk

