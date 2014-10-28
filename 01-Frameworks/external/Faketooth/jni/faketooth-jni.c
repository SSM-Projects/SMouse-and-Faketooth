#include <jni.h>
#include "faketooth-audio.h"

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothA2DPInit(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothA2DPInit();
}

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothA2DPEnable(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothA2DPEnable();
}

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothA2DPDo(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothA2DPDo();
}

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothA2DPDisable(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothA2DPDisable();
}

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothSCOInit(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothSCOInit();
}

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothSCOEnable(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothSCOEnable();
}

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothSCODo(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothSCODo();
}

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothSCODisable(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothSCODisable();
}

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothSCOMicInit(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothSCOMicInit();
}

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothSCOMicEnable(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothSCOMicEnable();
}

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothSCOMicDo(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothSCOMicDo();
}

JNIEXPORT jint JNICALL Java_android_app_FaketoothService_nativeFaketoothSCOMicDisable(
        JNIEnv* env, jobject obj) {
    return (jint)faketoothSCOMicDisable();
}
