TOP_LOCAL_PATH := $(call my-dir)

LOCAL_PATH := $(TOP_LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_JNI_SHARED_LIBRARIES := libsmouse

include $(call all-makefiles-under, $(LOCAL_PATH))
