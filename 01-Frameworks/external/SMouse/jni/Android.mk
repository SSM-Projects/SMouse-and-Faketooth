LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE		:= \
				   libsmouse

LOCAL_SRC_FILES	:= \
				   smouse-io.c \
				   smouse-jni.c
					
LOCAL_LDLIBS		:= \
				   -llog

include $(BUILD_SHARED_LIBRARY)
