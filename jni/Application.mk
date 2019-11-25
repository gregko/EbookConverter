# EbookConv project
NDK_TOOLCHAIN_VERSION := clang
APP_PLATFORM := android-16
APP_CFLAGS += -Wno-write-strings -Wno-logical-op-parentheses -Wno-unsequenced -Wno-parentheses -Wno-switch -Wno-#warnings -Wno-invalid-source-encoding
APP_CFLAGS += -include "C:/android/atVoiceLibs/idiocy_fopen_fd.h"
APP_CPPFLAGS += -frtti -fexceptions -Wno-logical-op-parentheses -Wno-unsequenced -Wno-parentheses -Wno-switch -Wno-#warnings -Wno-invalid-source-encoding
APP_ARM_MODE := thumb
APP_STL := c++_shared
ifeq ($(NDK_DEBUG),1)
    APP_OPTIM := debug
	APP_CFLAGS += -D_DEBUG
    APP_ABI := x86 x86_64 armeabi-v7a arm64-v8a
else 
    APP_OPTIM := release
    APP_ABI := armeabi-v7a x86_64 arm64-v8a x86
endif
