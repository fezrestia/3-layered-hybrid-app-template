#ifndef NATIVE_CAMERA_H
#define NATIVE_CAMERA_H

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/sensor.h>
#include <jni.h>

#include "ShaderProgramFactory.hpp"
#include "ElementBase.hpp"
#include "YuvFrame.hpp"

namespace fezrestia {



// Command to JAVA layer.
enum {
    CMD_STORE_NATIVE_APP_CONTEXT_POINTER        = 0x0001,
    CMD_ACTIVITY_ON_RESUMED                     = 0x0010,
    CMD_ACTIVITY_ON_PAUSED                      = 0x0020,
    CMD_WINDOW_INIT                             = 0x0100,
    CMD_WINDOW_TERM                             = 0x0200,

};

typedef struct {
    // Valid flag to render.
    bool mIsValid;

    // Dimensions.
    int32_t mWidth;
    int32_t mHeight;
    int32_t mFormat;

    // Buffer.
    unsigned char* mBuffer;
    size_t mBufferSize;
} frame_buffer;

typedef struct {
    EGLDisplay mEglDisplay;
    EGLSurface mEglReadSurface;
    EGLSurface mEglDrawSurface;
    EGLContext mEglContext;
} egl_pack;

typedef struct {
    // JAVA environment.
    JNIEnv* mJNIEnv;

    // JAVA Activity object.
    jclass mActivityClazz;
    jmethodID mActivityCallbackMethodId;

    // Asset manager.
    AAssetManager* mAndroidAssetManager;

    // JAVA accessor.
    jmethodID mActivitySendCommandMethodId;

    // System default EGL.
    egl_pack* mSystemDefaultEgl;

    // This EGL.
    egl_pack* mThisEgl;

    // GL global.
    bool mIsRenderRequested;
    float* mViewMatrix;
    float* mProjectionMatrix;

    // GL local.
    ShaderProgramFactory* mShaderProgramFactory;

    // Native windlw.
    ANativeWindow* mTargetNativeWindow;

    // Resolution.
    int32_t mDisplayWidth;
    int32_t mDisplayHeight;

    // Main frame.
    frame_buffer* mMainFrame;
    YuvFrame* mMainFrameRenderer;
} app_context;

// Prepare accessor to JAVA layer.
static void prepareAccessorToJava(app_context* appContext);

// Release accessor to JAVA layer.
static void releaseAccessorToJava(app_context* appContext);

// Send command to JAVA layer.
static void sendCommandToJava(app_context* appContext, int32_t command);
static void sendCommandToJava(app_context* appContext, int32_t command, int32_t arg);

// EGL initialize.
static int context_initialize_egl(app_context* appContext);

// EGL finalize.
static void context_finalize_egl(app_context* appContext);

// GL initialize.
static void context_initialize_gl(app_context* appContext);

// GL finalize.
static void context_finalize_gl(app_context* appContext);

// GL/EGL render frame.
static void context_render_frame(app_context* appContext);

// Check UI thread.
static bool isMainThread();

}; // namespace fezrestia

#endif // NATIVE_CAMERA_H
