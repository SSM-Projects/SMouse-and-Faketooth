#include "smouse-io.h"
#include "smouse-jni.h"

JNIEXPORT jint JNICALL Java_com_android_internal_widget_SMouseTouchView_nativeOpenDevice
  (JNIEnv *env, jobject obj) {
	return (jint)openDevice();
}

JNIEXPORT jint JNICALL Java_com_android_internal_widget_SMouseTouchView_nativeCloseDevice
  (JNIEnv *env, jobject obj, jint fd) {
	return (jint)closeDevice(fd);
}

JNIEXPORT jint JNICALL Java_com_android_internal_widget_SMouseTouchView_nativeWriteValues
  (JNIEnv *env, jobject obj, jint fd, jint btn, jint wheel, jint moveX, jint moveY) {
	return (jint)writeValues(fd, btn, wheel, moveX, moveY);
}
