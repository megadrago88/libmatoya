LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

TARGET_OUT = bin/android/$(TARGET_ARCH_ABI)

FLAGS = \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	-Wno-switch \
	-Wno-atomic-alignment \
	-Wno-unknown-pragmas \
	-std=c99 \
	-fPIC

ifdef DEBUG
FLAGS := $(FLAGS) -O0 -g
else
FLAGS := $(FLAGS) -O3 -g0 -fvisibility=hidden
endif

LOCAL_MODULE_FILENAME := libmatoya
LOCAL_MODULE := libmatoya

LOCAL_C_INCLUDES := \
	src \
	src/unix \
	src/unix/linux \
	src/unix/linux/android \
	deps

DEFS = \
	-D_POSIX_C_SOURCE=200112L \
	-DMTY_GL_ES \
	-DMTY_GL_EXTERNAL \
	-DMTY_CRYPTO_EXTERNAL

LOCAL_CFLAGS = $(DEFS) $(FLAGS)

LOCAL_SRC_FILES := \
	src/compress.c \
	src/crypto.c \
	src/fs.c \
	src/json.c \
	src/log.c \
	src/memory.c \
	src/proc.c \
	src/sort.c \
	src/hash.c \
	src/list.c \
	src/queue.c \
	src/thread.c \
	src/gfx/gl.c \
	src/gfx/gl-ui.c \
	src/render.c \
	src/unix/crypto.c \
	src/unix/fs.c \
	src/unix/memory.c \
	src/unix/proc.c \
	src/unix/thread.c \
	src/unix/time.c \
	src/unix/aes-gcm-openssl.c \
	src/unix/linux/android/window.c \
	src/unix/linux/android/audio.c

include $(BUILD_STATIC_LIBRARY)
