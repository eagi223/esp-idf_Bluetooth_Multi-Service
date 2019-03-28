#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the 
# src/ directory, compile them and link them into lib(subdirectory_name).a 
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#

APP_DIR   	   := 

APP_INCLUDEDIRS := . #Add current directory
 
APP_SRCDIRS     += $(APP_DIR).
APP_SRCDIRS     += $(APP_DIR)wireless
APP_SRCDIRS		+= $(APP_DIR)wireless/ble_services

COMPONENT_ADD_INCLUDEDIRS := $(APP_INCLUDEDIRS)
COMPONENT_SRCDIRS		  := $(APP_SRCDIRS)
