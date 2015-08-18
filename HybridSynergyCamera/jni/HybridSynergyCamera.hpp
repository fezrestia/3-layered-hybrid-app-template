#ifndef NATIVE_CAMERA_H
#define NATIVE_CAMERA_H

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/sensor.h>
#include <jni.h>

#include "ShaderProgramFactory.hpp"
#include "ElementBase.hpp"
#include "YuvFrame.hpp"
#include "SurfaceTextureFrame.hpp"

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
    int mWidth;
    int mHeight;
    int mFormat;

    // Buffer.
    unsigned char* mBuffer;
    size_t mBufferSize;
} frame_buffer;

typedef struct {
    EGLDisplay mEglDisplay;
    EGLSurface mEglReadSurface;
    EGLSurface mEglDrawSurface;
    EGLContext mEglContext;
} system_default_egl;

typedef struct {
    EGLDisplay mEglDisplay;
    EGLConfig mEglConfig;
    EGLSurface mEglSurfaceUi;
    EGLSurface mEglSurfaceCameraPreviewStream;
    EGLContext mEglContext;
} application_egl;

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
    system_default_egl* mSystemDefaultEgl;
    // Application EGL.
    application_egl* mApplicationEgl;

    // GL global.
    float* mViewMatrix;
    float* mProjectionMatrix;

    // GL local.
    ShaderProgramFactory* mShaderProgramFactory;

    // Native windlw.
    ANativeWindow* mNativeWindowUi;
    ANativeWindow* mNativeWindowCameraPreviewStream;

    // Resolution.
    int mSurfaceUiWidth;
    int mSurfaceUiHeight;
    int mSurfaceCameraPreviewStreamWidth;
    int mSurfaceCameraPreviewStreamHeight;

    // Frame renderer.
    frame_buffer* mMainFrame;
    YuvFrame* mMainFrameRenderer;
    SurfaceTextureFrame* mSurfaceTextureFrame;

    // Textures.
    GLuint mTextureCameraPreviewStream[1];
} app_context;

// Prepare accessor to JAVA layer.
static void prepareAccessorToJava(app_context* appContext);

// Release accessor to JAVA layer.
static void releaseAccessorToJava(app_context* appContext);

// Send command to JAVA layer.
static void sendCommandToJava(app_context* appContext, int32_t command);
static void sendCommandToJava(app_context* appContext, int32_t command, int32_t arg);

// EGL initialize/finalize.
static int context_initialize_egl(app_context* appContext);
static int context_finalize_egl(app_context* appContext);

// EGL surface initialize/finalize.
static int context_initialize_egl_surface_ui(app_context* appContext);
static int context_initialize_egl_surface_camera_preview_stream(app_context* appContext);
static int context_finalize_egl_surface_ui(app_context* appContext);
static int context_finalize_egl_surface_camera_preview_stream(app_context* appContext);

// EGL control.
static int context_change_current_egl_to(app_context* appContext, EGLSurface clientEglSurface);
static int context_return_egl_to_system_default(app_context* appContext);

// GL initialize.
static void context_initialize_gl(app_context* appContext);

// GL finalize.
static void context_finalize_gl(app_context* appContext);

// GL/EGL render frame.
static void context_render_frame(app_context* appContext);
static void context_render_camera_preview_stream(app_context* appContext);

// Check UI thread.
static bool isMainThread();

}; // namespace fezrestia

#endif // NATIVE_CAMERA_H
