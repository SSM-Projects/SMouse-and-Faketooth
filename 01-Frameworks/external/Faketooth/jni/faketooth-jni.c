#include <jni.h>
#include "faketooth-audio.h"

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothInit(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothInit();
}

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothEnable(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothEnable();
}

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothDo(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothDo();
}

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothDisable(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothDisable();
}
