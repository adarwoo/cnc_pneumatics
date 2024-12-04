TOP=..

# Select the architecture
ARCH=attiny3224

# Name of the binary to produce
BIN := relay

# Reference all from the solution
VPATH=$(TOP)

# Paths, local to src
COMMON_DIR     := common
BOOST_DIR      := boost
LOGGER_DIR     := logger
ASX_DIR        := asx
ETL_DIR        := etl
STDCXX_DIR     := libstdc++

# -I throughout (C and C++)
INCLUDE_DIRS = \
   src \
   include \
   conf \
   $(TOP)/$(COMMON_DIR)/include \
   $(TOP)/${BOOST_DIR} \
   $(TOP)/${ETL_DIR}/include \
   $(TOP)/${LOGGER_DIR}/include \
	$(TOP)/$(STDCXX_DIR)/include \
   $(TOP)/${ASX_DIR}/include \
   $(TOP)/${ASX_DIR}/include/utils \
   $(TOP)/${ASX_DIR}/include/utils/preprocessor \

ifndef SIM
INCLUDE_DIRS += $(TOP)/$(STDCXX_DIR)/include
endif

# Project own files
SRCS = \
   $(ASX_DIR)/src/builtin.cpp \
   $(ASX_DIR)/src/_ccp.s \
   $(ASX_DIR)/src/sysclk.c \
   $(ASX_DIR)/src/timer.c \
   $(ASX_DIR)/src/alert.c \
   $(ASX_DIR)/src/reactor.c \
   $(ASX_DIR)/src/uart.cpp \
   $(ASX_DIR)/src/hw_timer.cpp \
   $(ASX_DIR)/src/modbus_rtu.cpp \
   modbus.cpp \
   relay_ctrl.cpp \
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
   relay_ctrl.cpp \
   modbus.cpp \
   main.cpp \
   test_relay.cpp
endif

# Inlude the actual build rules
include $(TOP)/make/rules.mak

# Add dependency to generate the datagram from the config
main.cpp : conf/datagram.hpp
