LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := init_patch
LOCAL_SRC_FILES := patch.c main.c
LOCAL_CPPFLAGS := -Wall -fPIE
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -lc -llog -fPIE -pie

include $(BUILD_EXECUTABLE)    # <-- Use this to build an executable.
