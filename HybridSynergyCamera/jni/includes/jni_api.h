#ifndef JNI_API_H
#define JNI_API_H

#include <jni.h>

namespace fezrestia {

static jint nativeOnActivityCreated(
        JNIEnv* jenv,
        jclass clazz,
        jobject thiz);

static jint nativeOnActivityDestroyed(
        JNIEnv* jenv,
        jclass clazz);

static jint nativeOnCameraPrepared(
        JNIEnv* jenv,
        jclass clazz,
        jint frameWidth,
        jint frameHeight,
        jint imageFormat);

static jint nativeOnCameraReleased(
        JNIEnv* jenv,
        jclass clazz);

static jint nativeOnCameraPreviewStreamUpdated(
        JNIEnv* jenv,
        jclass clazz);

static jint nativeOnUiSurfaceInitialized(
        JNIEnv* jenv,
        jclass clazz,
        jobject surface);

static jint nativeOnUiSurfaceFinalized(
        JNIEnv* jenv,
        jclass clazz);

static jint nativeOnCameraPreviewStreamInitialized(
        JNIEnv* jenv,
        jclass clazz,
        jobject surface,
        jint width,
        jint height);

static jint nativeOnCameraPreviewStreamFinalized(
        JNIEnv* jenv,
        jclass clazz);

static jint nativeGetTextureNameOfCameraPreviewStream(
        JNIEnv* jenv,
        jclass clazz);

static jint nativeSetSurfaceTextureTransformMatrix(
        JNIEnv* jenv,
        jclass clazz,
        jfloatArray textureTransformMatrix);

static jint nativeBindApplicationEglContext(
        JNIEnv* jenv,
        jclass clazz);

static jint nativeUnbindApplicationEglContext(
        JNIEnv* jenv,
        jclass clazz);



// Register JNI APIs.
static JNINativeMethod gJniMethods[] = {
    // Java API name, API signature, Native API name
    { "nativeOnActivityCreated", "(Ljava/lang/Object;)I", (void*) nativeOnActivityCreated },
    { "nativeOnActivityDestroyed", "()I", (void*) nativeOnActivityDestroyed },
    { "nativeOnCameraPrepared", "(III)I", (void*) nativeOnCameraPrepared },
    { "nativeOnCameraReleased", "()I", (void*) nativeOnCameraReleased },
    { "nativeOnCameraPreviewStreamUpdated", "()I", (void*) nativeOnCameraPreviewStreamUpdated },
    { "nativeOnUiSurfaceInitialized", "(Landroid/view/Surface;)I", (void*) nativeOnUiSurfaceInitialized },
    { "nativeOnUiSurfaceFinalized", "()I", (void*) nativeOnUiSurfaceFinalized },
    { "nativeOnCameraPreviewStreamInitialized", "(Landroid/view/Surface;II)I", (void*) nativeOnCameraPreviewStreamInitialized },
    { "nativeOnCameraPreviewStreamFinalized", "()I", (void*) nativeOnCameraPreviewStreamFinalized },
    { "nativeGetTextureNameOfCameraPreviewStream", "()I", (void*) nativeGetTextureNameOfCameraPreviewStream },
    { "nativeSetSurfaceTextureTransformMatrix", "([F)I", (void*) nativeSetSurfaceTextureTransformMatrix },
    { "nativeBindApplicationEglContext", "()I", (void*) nativeBindApplicationEglContext },
    { "nativeUnbindApplicationEglContext", "()I", (void*) nativeUnbindApplicationEglContext },
};

} // namespace fezrestia

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*);

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void*);

#ifdef __cplusplus
}
#endif

#endif // JNI_API_H
