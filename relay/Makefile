TOP=..

# Select the architecture
ARCH=attiny824

# Name of the binary to produce
BIN := relay

# Reference all from the solution
VPATH=..

# Paths, local to src
THIS_DIR       := .
COMMON_DIR     := common
BOOST_DIR      := boost
ASX_DIR        := asx
STDCXX_DIR     := libstdc++
ETL_DIR        := etl

# -I throughout (C and C++)
INCLUDE_DIRS = \
   src \
   include \
   conf \
   ../$(COMMON_DIR)/include \
   ../${BOOST_DIR} \
   ../${ETL_DIR}/include \
	../$(STDCXX_DIR)/include \
   ../${ASX_DIR}/include \
   ../${ASX_DIR}/include/utils \
   ../${ASX_DIR}/include/utils/preprocessor \

# Mixed library and common files
SRCS := \
   $(ASX_DIR)/src/alert.c \
   $(ASX_DIR)/src/builtin.cpp \
   $(ASX_DIR)/src/ccp.c \
   $(ASX_DIR)/src/_ccp.s \
   $(ASX_DIR)/src/digital_input.c \
   $(ASX_DIR)/src/digital_output.c \
   $(ASX_DIR)/src/mem.c \
   $(ASX_DIR)/src/queue.c 	\
   $(ASX_DIR)/src/reactor.c \
   $(ASX_DIR)/src/sysclk.c \
   $(ASX_DIR)/src/timer.c \
   $(ASX_DIR)/src/twis.c \
   $(ASX_DIR)/src/digital_input.c \
# $(ASX_DIR)/src/asx.cpp \

# Generate the modbus_rtu code
#$(ASX_DIR)/src/modbus_rtu_slave.cpp : conf/modbus_rtu_slave.hpp

# Project own files
SRCS = \
   $(ASX_DIR)/src/builtin.cpp \
   $(ASX_DIR)/src/ccp.c \
   $(ASX_DIR)/src/_ccp.s \
   $(ASX_DIR)/src/sysclk.c \
   $(ASX_DIR)/src/timer.c \
   $(ASX_DIR)/src/alert.c \
   $(ASX_DIR)/src/queue.c 	\
   $(ASX_DIR)/src/reactor.c \
   $(ASX_DIR)/src/hw_timer.cpp \
   $(ASX_DIR)/src/uart.cpp \
   main.cpp \


# Inlude the actual build rules
include $(TOP)/make/rules.mak
