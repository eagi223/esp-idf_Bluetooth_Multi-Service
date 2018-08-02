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

APP_OBJS        := $(APP_DIR)main.o 
APP_OBJS		+= $(APP_DIR)wireless/ble_interface.o 
APP_OBJS		+= $(APP_DIR)wireless/ble_common.o 
APP_OBJS		+= $(APP_DIR)wireless/ble_gap.o
APP_OBJS		+= $(APP_DIR)wireless/ble_gatt_server.o
APP_OBJS		+= $(APP_DIR)wireless/ble_services/ble_service_wifi_config.o

COMPONENT_ADD_INCLUDEDIRS := $(APP_INCLUDEDIRS)
COMPONENT_SRCDIRS		  := $(APP_SRCDIRS)
COMPONENT_OBJS            := $(APP_OBJS)