UNAME_S = $(shell uname -s)
ARCH = $(shell uname -m)
NAME = libmatoya
PREFIX = mty

.SUFFIXES: .vert .frag

.vert.h:
	@hexdump -ve '1/1 "0x%.2x,"' $< | (echo 'static const GLchar VERT[]={' && cat && echo '0x00};') > $@

.frag.h:
	@hexdump -ve '1/1 "0x%.2x,"' $< | (echo 'static const GLchar FRAG[]={' && cat && echo '0x00};') > $@

.m.o:
	$(CC) $(OCFLAGS)  -c -o $@ $<

OBJS = \
	src/compress.o \
	src/crypto.o \
	src/fs.o \
	src/json.o \
	src/log.o \
	src/memory.o \
	src/proc.o \
	src/sort.o \
	src/hash.o \
	src/list.o \
	src/queue.o \
	src/thread.o \
	src/gfx/gl.o \
	src/gfx/gl-ui.o \
	src/gfx/render.o \
	src/gfx/window-ctx.o

OBJS := $(OBJS) \
	src/unix/fs.o \
	src/unix/memory.o \
	src/unix/proc.o \
	src/unix/thread.o \
	src/unix/time.o

SHADERS = \
	src/gfx/shaders/gl/vs.h \
	src/gfx/shaders/gl/fs.h \
	src/gfx/shaders/gl/vsui.h \
	src/gfx/shaders/gl/fsui.h

INCLUDES = \
	-Isrc \
	-Isrc/unix \
	-Ideps

DEFS = \
	-D_POSIX_C_SOURCE=200112L

FLAGS = \
	-Wall \
	-Wextra \
	-Wshadow \
	-Wno-unused-parameter \
	-Wno-switch \
	-std=c99 \
	-fPIC

TEST_FLAGS = \
	-nodefaultlibs

ifdef DEBUG
FLAGS := $(FLAGS) -O0 -g
else
FLAGS := $(FLAGS) -O3 -fvisibility=hidden
endif

############
### WASM ###
############
ifdef WASM
ifdef EMSDK

CC = emcc
AR = emar

ARCH := emscripten

OBJS := $(OBJS) \
	src/unix/crypto.o

else
WASI_SDK = $(HOME)/wasi-sdk-11.0

CC = $(WASI_SDK)/bin/clang --sysroot=$(WASI_SDK)/share/wasi-sysroot
AR = $(WASI_SDK)/bin/ar

ARCH := wasm32

endif

OBJS := $(OBJS) \
	src/unix/web/window.o

DEFS := $(DEFS) \
	-DMTY_GL_EXTERNAL \
	-DMTY_GL_ES

TARGET = web
INCLUDES := $(INCLUDES) -Isrc/unix/web

else
#############
### LINUX ###
#############
ifeq ($(UNAME_S), Linux)

OBJS := $(OBJS) \
	src/unix/crypto.o \
	src/unix/aes-gcm-openssl.o \
	src/unix/linux/default/audio.o \
	src/unix/linux/default/window.o

DEFS := $(DEFS) \
	-DMTY_CRYPTO_EXTERNAL

TEST_LIBS = \
	-lc \
	-lm \
	-ldl \
	-lpthread

TARGET = linux
INCLUDES := $(INCLUDES) -Isrc/unix/linux -Isrc/unix/linux/default
endif

#############
### APPLE ###
#############
ifeq ($(UNAME_S), Darwin)

.SUFFIXES: .metal

.metal.h:
	hexdump -ve '1/1 "0x%.2x,"' $< | (echo 'static const char MTL_LIBRARY[]={' && cat && echo '0x00};') > $@

ifndef TARGET
TARGET = macosx
endif

ifndef ARCH
ARCH = x86_64
endif

ifeq ($(TARGET), macosx)
MIN_VER = 10.11
else
MIN_VER = 11.0
FLAGS := $(FLAGS) -fembed-bitcode
DEFS := $(DEFS) -DMTY_GL_ES
endif

OBJS := $(OBJS) \
	src/unix/apple/gfx/gl-ctx.o \
	src/unix/apple/gfx/metal.o \
	src/unix/apple/gfx/metal-ui.o \
	src/unix/apple/gfx/metal-ctx.o \
	src/unix/apple/aes-gcm-cc.o \
	src/unix/apple/audio.o \
	src/unix/apple/crypto.o
	#src/unix/apple/$(TARGET)/window.o

SHADERS := $(SHADERS) \
	src/unix/apple/gfx/shaders/metal/quad.h \
	src/unix/apple/gfx/shaders/metal/ui.h

DEFS := $(DEFS) \
	-DMTY_GL_EXTERNAL

TEST_LIBS = \
	-lc \
	-framework OpenGL \
	-framework AppKit \
	-framework QuartzCore \
	-framework Metal \
	-framework AudioToolbox

FLAGS := $(FLAGS) \
	-m$(TARGET)-version-min=$(MIN_VER) \
	-isysroot $(shell xcrun --sdk $(TARGET) --show-sdk-path) \
	-arch $(ARCH)

INCLUDES := $(INCLUDES) -Isrc/unix/apple -Isrc/unix/apple/$(TARGET)
endif
endif

CFLAGS = $(INCLUDES) $(DEFS) $(FLAGS)
OCFLAGS = $(CFLAGS) -fobjc-arc

all: clean-build clear $(SHADERS)
	make objs -j4

objs: $(OBJS)
	mkdir -p bin/$(TARGET)/$(ARCH)
	$(AR) -crs bin/$(TARGET)/$(ARCH)/$(NAME).a $(OBJS)

test: clean-build clear $(SHADERS)
	make objs-test -j4

objs-test: $(OBJS) src/test.o
	$(CC) -o $(PREFIX)-test $(TEST_FLAGS) $(OBJS) src/test.o $(TEST_LIBS)
	./$(PREFIX)-test

###############
### ANDROID ###
###############

### Downloads ###
# https://developer.android.com/ndk/downloads -> Put in ~/android-ndk-xxx

ANDROID_NDK = $(HOME)/android-ndk-r21d

android: clear $(SHADERS)
	@$(ANDROID_NDK)/ndk-build -j4 \
		NDK_PROJECT_PATH=. \
		APP_BUILD_SCRIPT=Android.mk \
		APP_OPTIM=release \
		APP_PLATFORM=android-23 \
		--no-print-directory \
		| grep -v 'fcntl(): Operation not supported'

clean: clean-build
	@rm -rf obj
	@rm -f $(SHADERS)

clean-build:
	@rm -rf bin
	@rm -f src/test.o
	@rm -f $(OBJS)
	@rm -f $(NAME).so
	@rm -f $(PREFIX)-test

clear:
	@clear
