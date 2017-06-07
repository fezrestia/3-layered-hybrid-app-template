#ifndef COM_FEZRESTIA_ANDROID_HYBRIDSYNERGYCAMERA_HYBRIDSYNERGYCAMERAACTIVITY_H
#define COM_FEZRESTIA_ANDROID_HYBRIDSYNERGYCAMERA_HYBRIDSYNERGYCAMERAACTIVITY_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*);
JNIEXPORT void JNI_OnUnload(JavaVM* vm, void*);

//// ACTIVITY RELATED ////////////////////////////////////////////////////////////////////////////

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeOnActivityCreated
 * Signature: (Lcom/fezrestia/android/hybridsynergycamera/HybridSynergyCameraActivity;)I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnActivityCreated(
        JNIEnv* jenv,
        jclass clazz,
        jobject thiz);

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeOnActivityDestroyed
 * Signature: (V)I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnActivityDestroyed(
        JNIEnv* jenv,
        jclass clazz);

//////////////////////////////////////////////////////////////////////////// ACTIVITY RELATED ////



//// CAMERA RELATED //////////////////////////////////////////////////////////////////////////////

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeOnCameraPrepared
 * Signature: (III)I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnCameraPrepared(
        JNIEnv* jenv,
        jclass clazz,
        jint frameWidth,
        jint frameHeight,
        jint imageFormat);

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeOnCameraReleased
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnCameraReleased(
        JNIEnv* jenv,
        jclass clazz);

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeOnCameraPreviewStreamUpdated
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnCameraPreviewStreamUpdated(
        JNIEnv* jenv,
        jclass clazz);

////////////////////////////////////////////////////////////////////////////// CAMERA RELATED ////



//// SURFACE RELATED /////////////////////////////////////////////////////////////////////////////

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeOnUiSurfaceInitialized
 * Signature: (Landroid/view/Surface;)I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnUiSurfaceInitialized(
        JNIEnv* jenv,
        jclass clazz,
        jobject surface);

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeOnUiSurfaceFinalized
 * Signature: (V)I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnUiSurfaceFinalized(
        JNIEnv* jenv,
        jclass clazz);

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeOnCameraPreviewStreamInitialized
 * Signature: (Landroid/view/Surface;II)I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnCameraPreviewStreamInitialized(
        JNIEnv* jenv,
        jclass clazz,
        jobject surface,
        jint width,
        jint height);

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeOnCameraPreviewStreamFinalized
 * Signature: (V)I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnCameraPreviewStreamFinalized(
        JNIEnv* jenv,
        jclass clazz);

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeGetTextureNameOfCameraPreviewStream
 * Signature: (V)I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeGetTextureNameOfCameraPreviewStream(
        JNIEnv* jenv,
        jclass clazz);

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeSetSurfaceTextureTransformMatrix
 * Signature: ([F)I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeSetSurfaceTextureTransformMatrix(
        JNIEnv* jenv,
        jclass clazz,
        jfloatArray textureTransformMatrix);

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeBindApplicationEglContext
 * Signature: (V)I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeBindApplicationEglContext(
        JNIEnv* jenv,
        jclass clazz);

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeUnbindApplicationEglContext
 * Signature: (V)I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeUnbindApplicationEglContext(
        JNIEnv* jenv,
        jclass clazz);

///////////////////////////////////////////////////////////////////////////// SURFACE RELATED ////



#ifdef __cplusplus
}
#endif

#endif // COM_FEZRESTIA_ANDROID_HYBRIDSYNERGYCAMERA_HYBRIDSYNERGYCAMERAACTIVITY_H
