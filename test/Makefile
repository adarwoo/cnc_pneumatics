TOP=..

# Name of the binary to produce
BIN := test

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
   $(ASX_DIR)/src/queue.c 	\

# Project own files
SRCS += \
   test_queue.cpp \

# Inlude the actual build rules
include $(TOP)/make/rules.mak
