#ifndef _SMOUSE_JNI_H_
#define _SMOUSE_JNI_H_

#include <jni.h>

JNIEXPORT jint JNICALL Java_com_android_internal_widget_SMouseTouchView_nativeOpenDevice
  (JNIEnv *, jobject);

JNIEXPORT jint JNICALL Java_com_android_internal_widget_SMouseTouchView_nativeCloseDevice
  (JNIEnv *, jobject, jint);

JNIEXPORT jint JNICALL Java_com_android_internal_widget_SMouseTouchView_nativeWriteValues
  (JNIEnv *, jobject, jint, jint, jint, jint, jint);

#endif
