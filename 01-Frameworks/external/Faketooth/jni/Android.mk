LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := \
	libfaketooth

LOCAL_SRC_FILES := \
	faketooth-audio.cpp \
	faketooth-jni.c 

LOCAL_SHARED_LIBRARIES := \
	libutils \
	libmedia \
	liblog

include $(BUILD_SHARED_LIBRARY)
