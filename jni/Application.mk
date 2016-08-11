# EbookConv project
NDK_TOOLCHAIN_VERSION := clang
APP_PLATFORM := android-14
APP_CFLAGS += -Wno-write-strings -Wno-logical-op-parentheses -Wno-unsequenced -Wno-parentheses -Wno-switch -Wno-#warnings -Wno-invalid-source-encoding
APP_CPPFLAGS += -fexceptions -Wno-logical-op-parentheses -Wno-unsequenced -Wno-parentheses -Wno-switch -Wno-#warnings -Wno-invalid-source-encoding
APP_ARM_MODE := thumb
APP_STL := stlport_shared
#APP_CPPFLAGS += -fexceptions
ifeq ($(NDK_DEBUG),1)
    APP_OPTIM := debug
	APP_CFLAGS += -D_DEBUG
    APP_ABI := x86 mips armeabi armeabi-v7a arm64-v8a
else 
    APP_OPTIM := release
    APP_ABI := mips armeabi armeabi-v7a arm64-v8a x86
endif
