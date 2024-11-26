TOP=..

# Select the architecture
ARCH=attiny3224

# Name of the binary to produce
BIN := relay

# Reference all from the solution
VPATH=..

# Paths, local to src
THIS_DIR       := .
COMMON_DIR     := common
BOOST_DIR      := boost
LOGGER_DIR     := logger
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
   ../${LOGGER_DIR}/include \
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
   $(ASX_DIR)/src/uart.cpp \
   $(ASX_DIR)/src/hw_timer.cpp \
   $(ASX_DIR)/src/modbus_rtu.cpp \
   main.cpp \

ifdef SIM
SRCS = \
   ${LOGGER_DIR}/src/logger_common.c \
   ${LOGGER_DIR}/src/logger_cxx.cpp \
   ${LOGGER_DIR}/src/logger_init_unix.cpp \
   ${LOGGER_DIR}/src/logger_os_linux.cpp \
   ${LOGGER_DIR}/src/logger_os_linux_thread.cpp \
   ${LOGGER_DIR}/src/logger_trace_stack_linux.cpp \
   $(ASX_DIR)/src/timer.c \
   $(ASX_DIR)/src/alert.c \
   $(ASX_DIR)/src/queue.c 	\
   $(ASX_DIR)/src/reactor.c \
   $(ASX_DIR)/src/uart.cpp \
   $(ASX_DIR)/src/modbus_rtu.cpp \
   $(ASX_DIR)/src/hw_timer.cpp \
   ../sim/sim.cpp \
   main.cpp 
endif

# Inlude the actual build rules
include $(TOP)/make/rules.mak
