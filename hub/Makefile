TOP=..

# Select the architecture
ARCH=attiny824

# Name of the binary to produce
BIN := hub

# Reference all from the solution
VPATH=..

# Paths, local to src
THIS_DIR       := .
COMMON_DIR     := common
BOOST_DIR      := boost
ASX_DIR        := asx

# -I throughout (C and C++)
INCLUDE_DIRS = \
   src \
   include \
   conf \
   ../$(COMMON_DIR)/include \
   ../${BOOST_DIR} \
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

# Project own files
SRCS += \
   board.c \
	protocol.c \
   i2c_slave.c \
   pressure_mon.c \
   main.c \

# Inlude the actual build rules
include $(TOP)/make/rules.mak
