# Check the build type
ifeq ($(DEBUG),1)
	BUILD_DIR:=Debug
else
	BUILD_DIR:=Release
endif

# Change the compiler
CXX := $(ToolchainDir)/avr-g++
CC := $(ToolchainDir)/avr-gcc
SIZE := $(ToolchainDir)/avr-size
MKDIR := mkdir
MUTE  := $(if $(VERBOSE),,@)

BIN_EXT :=.elf

ARCHFLAGS :=-mmcu=$(ARCH) -B "$(DEVICE_STARTUP_ROOT)\gcc\dev\$(ARCH)"
CPPFLAGS += -isystem "$(DEVICE_STARTUP_ROOT)/include"
CFLAGS += -funsigned-char -funsigned-bitfields -ffunction-sections -fdata-sections -fshort-enums 
CFLAGS += -O$(if $(DEBUG),g,s)
CXXFLAGS += -fno-threadsafe-statics

LDFLAGS += -Wl,-Map="$(BIN).map" -Wl,--start-group -Wl,-lm  -Wl,--end-group -Wl,--gc-sections -Wl,--demangle -Wl,-flto

OBJS = $(foreach file, $(SRCS), $(BUILD_DIR)/$(basename $(file)).o)

define DIAG
endef

define POST_LINK
endef
