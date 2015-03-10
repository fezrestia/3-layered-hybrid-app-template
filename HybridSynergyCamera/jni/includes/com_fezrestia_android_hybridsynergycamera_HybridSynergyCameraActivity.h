#ifndef COM_FEZRESTIA_ANDROID_HYBRIDSYNERGYCAMERA_HYBRIDSYNERGYCAMERAACTIVITY_H
#define COM_FEZRESTIA_ANDROID_HYBRIDSYNERGYCAMERA_HYBRIDSYNERGYCAMERAACTIVITY_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif



//// ACTIVITY RELATED ////////////////////////////////////////////////////////////////////////////

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeOnActivityCreated
 * Signature: (Landroid/content/res/AssetManager;)I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnActivityCreated(
        JNIEnv* jenv,
        jclass clazz,
        jobject jAssetManager);

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
 * Method:    nativeOnPreviewFrameUpdated
 * Signature: (III[BII)I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnPreviewFrameUpdated(
        JNIEnv* jenv,
        jclass clazz,
        jint frameWidth,
        jint frameHeight,
        jint imageFormat,
        jbyteArray frameBuffer,
        jint frameBufferOffset,
        jint frameBufferSize);

////////////////////////////////////////////////////////////////////////////// CAMERA RELATED ////



//// SURFACE RELATED /////////////////////////////////////////////////////////////////////////////

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeOnSurfaceInitialized
 * Signature: (Landroid/view/Surface;)I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnSurfaceInitialized(
        JNIEnv* jenv,
        jclass clazz,
        jobject surface);

/*
 * Class:     com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity
 * Method:    nativeOnSurfaceFinalized
 * Signature: (V)I
 */
JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnSurfaceFinalized(
        JNIEnv* jenv,
        jclass clazz);

///////////////////////////////////////////////////////////////////////////// SURFACE RELATED ////



#ifdef __cplusplus
}
#endif

#endif // COM_FEZRESTIA_ANDROID_HYBRIDSYNERGYCAMERA_HYBRIDSYNERGYCAMERAACTIVITY_H
