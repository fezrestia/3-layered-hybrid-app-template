#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/native_window_jni.h>
#include <android/asset_manager_jni.h>
#include <unistd.h>

#include "HybridSynergyCamera.hpp"

#include "android_opengl_matrix.h"
#include "YuvFrame.hpp"
#include "ShaderProgramFactory.hpp"
#include "TraceLog.h"
#include "com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity.h"

namespace fezrestia {

// JAVA VM.
static JavaVM* gVm;

// Total application context.
static app_context* gAppContext = NULL;

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    TRACE_LOG("E");

    gVm = vm;

    TRACE_LOG("X");
    return JNI_VERSION_1_6;
}

extern "C" void JNI_OnUnload(JavaVM* vm, void* reserved) {
    TRACE_LOG("E");

    gVm = NULL;

    TRACE_LOG("X");
}

//// ACTIVITY RELATED ////////////////////////////////////////////////////////////////////////////

// onActivityCreated()
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnActivityCreated(
        JNIEnv* jenv,
        jclass clazz,
        jobject thiz) {
    TRACE_LOG("E");

    gAppContext = new app_context();

    // Cache JAVA references.
    jclass activityClazz = jenv->FindClass(
            "com/fezrestia/android/hybridsynergycamera/HybridSynergyCameraActivity");
    jmethodID getVertexShaderCodeMethodId = jenv->GetMethodID(
            activityClazz,
            "getVertexShaderCode",
            "()Ljava/lang/String;");
    jmethodID getFragmentShaderCodeMethodId = jenv->GetMethodID(
            activityClazz,
            "getFragmentShaderCode",
            "()Ljava/lang/String;");
    jenv->DeleteLocalRef(activityClazz);

    // Class/Method cache.
    gAppContext->mJavaClazz = activityClazz;
    gAppContext->mJavaObj = jenv->NewGlobalRef(thiz);

    // Cache shader code.
    jstring vertexShaderCodeString = (jstring) jenv->CallObjectMethod(
            thiz,
            getVertexShaderCodeMethodId);
    gAppContext->mVertexShaderCodeString = (jstring) jenv->NewGlobalRef(vertexShaderCodeString);
    jstring fragmentShaderCodeString = (jstring) jenv->CallObjectMethod(
            thiz,
            getFragmentShaderCodeMethodId);
    gAppContext->mFragmentShaderCodeString = (jstring) jenv->NewGlobalRef(fragmentShaderCodeString);

    // Initialize EGL.
    context_initialize_egl(gAppContext);

    TRACE_LOG("X");
}

// onActivityDestroyed()
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnActivityDestroyed(
        JNIEnv* jenv,
        jclass clazz) {
    TRACE_LOG("E");

    // Finalize EGL.
    context_finalize_egl(gAppContext);

    if (gAppContext != NULL) {
        // Release references.
        jenv->DeleteGlobalRef(gAppContext->mJavaObj);
        jenv->DeleteGlobalRef(gAppContext->mVertexShaderCodeString);
        jenv->DeleteGlobalRef(gAppContext->mFragmentShaderCodeString);

        delete gAppContext;
        gAppContext = NULL;
    }

    TRACE_LOG("X");
}

//////////////////////////////////////////////////////////////////////////// ACTIVITY RELATED ////



//// SURFACE RELATED /////////////////////////////////////////////////////////////////////////////

// onUiSurfaceInitialized()
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnUiSurfaceInitialized(
        JNIEnv* jenv,
        jclass clazz,
        jobject surface) {
    TRACE_LOG("E");

    // Life-cycle check.
    if (gAppContext == NULL) {
        LOGE("gAppContext is already released.");
        return 0;
    }

    // Native window.
    gAppContext->mNativeWindowUi = ANativeWindow_fromSurface(jenv, surface);

    // Initialize EGL surface.
    context_initialize_egl_surface_ui(gAppContext);

    // Initialize GL.
    context_initialize_gl(gAppContext, jenv);

    // Camera preview stream textures.
    {
        context_change_current_egl_to(gAppContext, gAppContext->mApplicationEgl->mEglSurfaceUi);
        // Generate texture.
        glGenTextures(1, gAppContext->mTextureCameraPreviewStream);
        GLuint texId = gAppContext->mTextureCameraPreviewStream[0];
        // Link texture to target.
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, texId);
        // Magnification type. Linear interpolation for performance.
        glTexParameterf(
                GL_TEXTURE_EXTERNAL_OES,
                GL_TEXTURE_MAG_FILTER,
                GL_NEAREST);
        // Minimization type. Linear interpolation for performance.
        glTexParameterf(
                GL_TEXTURE_EXTERNAL_OES,
                GL_TEXTURE_MIN_FILTER,
                GL_NEAREST);
        // Clamp edge.
        glTexParameterf(
                GL_TEXTURE_EXTERNAL_OES,
                GL_TEXTURE_WRAP_S,
                GL_CLAMP_TO_EDGE);
        glTexParameterf(
                GL_TEXTURE_EXTERNAL_OES,
                GL_TEXTURE_WRAP_T,
                GL_CLAMP_TO_EDGE);
        // Un-link texture from target.
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
        context_return_egl_to_system_default(gAppContext);
    }

    TRACE_LOG("X");
    return 0;
}

// onUiSurfaceFinalized()
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnUiSurfaceFinalized(
        JNIEnv* jenv,
        jclass clazz) {
    TRACE_LOG("E");

    // Life-cycle check.
    if (gAppContext == NULL) {
        LOGE("gAppContext is already released.");
        return 0;
    }

    // Camera preview stream textures.
    {
        context_change_current_egl_to(gAppContext, gAppContext->mApplicationEgl->mEglSurfaceUi);
        glDeleteTextures(1, gAppContext->mTextureCameraPreviewStream);
        gAppContext->mTextureCameraPreviewStream[0] = 0;
        context_return_egl_to_system_default(gAppContext);
    }

    // Finalize GL.
    context_finalize_gl(gAppContext);

    // Finalize EGL surface.
    context_finalize_egl_surface_ui(gAppContext);

    TRACE_LOG("X");
    return 0;
}

// onCameraPreviewStreamInitialized()
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnCameraPreviewStreamInitialized(
        JNIEnv* jenv,
        jclass clazz,
        jobject surface,
        jint width,
        jint height) {
    TRACE_LOG("E");

    // Cache size.
    gAppContext->mSurfaceCameraPreviewStreamWidth = width;
    gAppContext->mSurfaceCameraPreviewStreamHeight = height;

    TRACE_LOG("X");
    return 0;
}

// onCameraPreviewStreamFinalized()
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnCameraPreviewStreamFinalized(
        JNIEnv* jenv,
        jclass clazz) {
    TRACE_LOG("E");

    // NOP.

    TRACE_LOG("X");
    return 0;
}

// getTextureNameOfCameraPreviewStream
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeGetTextureNameOfCameraPreviewStream(
        JNIEnv* jenv,
        jclass clazz) {
    return gAppContext->mTextureCameraPreviewStream[0];
}

// setSurfaceTextureTransformMatrix
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeSetSurfaceTextureTransformMatrix(
        JNIEnv* jenv,
        jclass clazz,
        jfloatArray textureTransformMatrix) {
    TRACE_LOG("E");

    // Load matrix.
    jfloat* matrix = (jfloat*) jenv->GetPrimitiveArrayCritical(textureTransformMatrix, NULL);
    gAppContext->mSurfaceTextureFrame->setTextureTransformMatrix(matrix);
    jenv->ReleasePrimitiveArrayCritical(textureTransformMatrix, matrix, JNI_ABORT);

    TRACE_LOG("X");
    return 0;
}

// bindApplicationEglContext
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeBindApplicationEglContext(
        JNIEnv* jenv,
        jclass clazz) {
    TRACE_LOG("E");

    context_change_current_egl_to(gAppContext, gAppContext->mApplicationEgl->mEglSurfaceUi);

    //TODO:
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, gAppContext->mTextureCameraPreviewStream[0]);

    TRACE_LOG("X");
    return 0;
}

// unbindApplicationEglContext
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeUnbindApplicationEglContext(
        JNIEnv* jenv,
        jclass clazz) {
    TRACE_LOG("E");

    //TODO:
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

    context_return_egl_to_system_default(gAppContext);

    TRACE_LOG("X");
    return 0;
}

///////////////////////////////////////////////////////////////////////////// SURFACE RELATED ////



//// EGL INITIALIZE / FINALIZE ///////////////////////////////////////////////////////////////////

static int context_initialize_egl(app_context* appContext) {
    TRACE_LOG("E");

    // Attributes.
    const EGLint eglConfigAttribs[] = {
            EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE,       EGL_WINDOW_BIT,
            EGL_RED_SIZE,           8,
            EGL_GREEN_SIZE,         8,
            EGL_BLUE_SIZE,          8,
            EGL_DEPTH_SIZE,         16,
            EGL_STENCIL_SIZE,       8,
            EGL_NONE
    };
    const EGLint eglContextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION,     2,
            EGL_NONE
    };

    // EGL display connection.

    // Cache system default configuration.
    appContext->mSystemDefaultEgl = new system_default_egl();

    // Get display.
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    // Initialize display.
    eglInitialize(display, 0, 0);

    // Set config.
    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(display, eglConfigAttribs, &config, 1, &numConfigs);

    // Get EGL frame buffer info.
    EGLint format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    // Get EGL rendering context.
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, eglContextAttribs);

    // Cache this EGL.
    appContext->mApplicationEgl = new application_egl();
    appContext->mApplicationEgl->mEglDisplay = display;
    appContext->mApplicationEgl->mEglConfig = config;
    appContext->mApplicationEgl->mEglContext = context;

    TRACE_LOG("X");
    return 0;
}

static int context_finalize_egl(app_context* appContext) {
    TRACE_LOG("E");

    // Release this EGL.
    if (appContext->mApplicationEgl != NULL) {
        eglDestroyContext(
                appContext->mApplicationEgl->mEglDisplay,
                appContext->mApplicationEgl->mEglContext);
        eglTerminate(appContext->mApplicationEgl->mEglDisplay);
    }

    // Release system default EGL cache.
    if (appContext->mSystemDefaultEgl != NULL) {
        delete appContext->mSystemDefaultEgl;
        appContext->mSystemDefaultEgl = NULL;
    }
    // Release application EGL cache.
    if (appContext->mApplicationEgl != NULL) {
        delete appContext->mApplicationEgl;
        appContext->mApplicationEgl = NULL;
    }

    TRACE_LOG("X");
    return 0;
}

static int context_initialize_egl_surface_ui(app_context* appContext) {
    TRACE_LOG("E");

    // Create EGL surface.
    EGLSurface surface = eglCreateWindowSurface(
            appContext->mApplicationEgl->mEglDisplay,
            appContext->mApplicationEgl->mEglConfig,
            appContext->mNativeWindowUi,
            NULL);
    appContext->mApplicationEgl->mEglSurfaceUi = surface;

    // Enable UI EGL.
    context_change_current_egl_to(appContext, appContext->mApplicationEgl->mEglSurfaceUi);

    // Get resolution.
    EGLint width;
    EGLint height;
    eglQuerySurface(
            appContext->mApplicationEgl->mEglDisplay,
            appContext->mApplicationEgl->mEglSurfaceUi,
            EGL_WIDTH,
            &width);
    eglQuerySurface(
            appContext->mApplicationEgl->mEglDisplay,
            appContext->mApplicationEgl->mEglSurfaceUi,
            EGL_HEIGHT,
            &height);
    appContext->mSurfaceUiWidth = width;
    appContext->mSurfaceUiHeight = height;

    // Set GL view port.
    if (height < width) {
        // This is Landscape.
        EGLint verticalOffset = (width - height) / 2;
        glViewport(0, -1 * verticalOffset, width, width); // Square.
    } else {
        // This is Portrait.
        EGLint horizontalOffset = (height - width) / 2;
        glViewport(-1 * horizontalOffset, 0, height, height); // Square.
    }

    // Return EGL to system default.
    context_return_egl_to_system_default(appContext);

    TRACE_LOG("X");
    return 0;
}

static int context_finalize_egl_surface_ui(app_context* appContext) {
    TRACE_LOG("E");

    // Release UI EGL surface.
    if (appContext->mApplicationEgl->mEglSurfaceUi != NULL) {
        eglDestroySurface(
                appContext->mApplicationEgl->mEglDisplay,
                appContext->mApplicationEgl->mEglSurfaceUi);
        appContext->mApplicationEgl->mEglSurfaceUi = NULL;
    }

    // Release native window.
    if (appContext->mNativeWindowUi != NULL) {
        ANativeWindow_release(appContext->mNativeWindowUi);
        appContext->mNativeWindowUi = NULL;
    }

    TRACE_LOG("X");
    return 0;
}

static int context_initialize_egl_surface_camera_preview_stream(app_context* appContext) {
    TRACE_LOG("E");

    // Create EGL surface.
    EGLSurface surface = eglCreateWindowSurface(
            appContext->mApplicationEgl->mEglDisplay,
            appContext->mApplicationEgl->mEglConfig,
            appContext->mNativeWindowCameraPreviewStream,
            NULL);
    appContext->mApplicationEgl->mEglSurfaceCameraPreviewStream = surface;

    // Enable UI EGL.
    context_change_current_egl_to(
            appContext,
            appContext->mApplicationEgl->mEglSurfaceCameraPreviewStream);

    // Get resolution.
    EGLint width;
    EGLint height;
    eglQuerySurface(
            appContext->mApplicationEgl->mEglDisplay,
            appContext->mApplicationEgl->mEglSurfaceCameraPreviewStream,
            EGL_WIDTH,
            &width);
    eglQuerySurface(
            appContext->mApplicationEgl->mEglDisplay,
            appContext->mApplicationEgl->mEglSurfaceCameraPreviewStream,
            EGL_HEIGHT,
            &height);
    appContext->mSurfaceCameraPreviewStreamWidth = width;
    appContext->mSurfaceCameraPreviewStreamHeight = height;

    // Return EGL to system default.
    context_return_egl_to_system_default(appContext);

    TRACE_LOG("X");
    return 0;
}

static int context_finalize_egl_surface_camera_preview_stream(app_context* appContext) {
    TRACE_LOG("E");

    // Release camera preview stream EGL surface.
    if (appContext->mApplicationEgl->mEglSurfaceCameraPreviewStream != NULL) {
        eglDestroySurface(
                appContext->mApplicationEgl->mEglDisplay,
                appContext->mApplicationEgl->mEglSurfaceCameraPreviewStream);
        appContext->mApplicationEgl->mEglSurfaceCameraPreviewStream = NULL;
    }

    // Release native window.
    if (appContext->mNativeWindowCameraPreviewStream != NULL) {
        ANativeWindow_release(appContext->mNativeWindowCameraPreviewStream);
        appContext->mNativeWindowCameraPreviewStream = NULL;
    }

    TRACE_LOG("X");
    return 0;
}

static int context_change_current_egl_to(app_context* appContext, EGLSurface clientEglSurface) {
    TRACE_LOG("E");

    if (isMainThread()) {
        // Cache system default EGL.
        appContext->mSystemDefaultEgl->mEglDisplay = eglGetCurrentDisplay();
        appContext->mSystemDefaultEgl->mEglReadSurface = eglGetCurrentSurface(EGL_READ);
        appContext->mSystemDefaultEgl->mEglDrawSurface = eglGetCurrentSurface(EGL_DRAW);
        appContext->mSystemDefaultEgl->mEglContext = eglGetCurrentContext();
    }

    // Enable client EGL.
    if (eglMakeCurrent(
            appContext->mApplicationEgl->mEglDisplay,
            clientEglSurface,
            clientEglSurface,
            appContext->mApplicationEgl->mEglContext) == EGL_FALSE) {
        LOGE("Failed to enable client EGL.");
        return -1;
    }

    TRACE_LOG("X");
    return 0;
}

static int context_return_egl_to_system_default(app_context* appContext) {
    TRACE_LOG("E");

    // Return EGL to system default.
    if (isMainThread()) {
        // Main thread.
        if (eglMakeCurrent(
                appContext->mSystemDefaultEgl->mEglDisplay,
                appContext->mSystemDefaultEgl->mEglReadSurface,
                appContext->mSystemDefaultEgl->mEglDrawSurface,
                appContext->mSystemDefaultEgl->mEglContext) == EGL_FALSE) {
            LOGE("Failed to return EGL to System Default.");
            return -1;
        }
    } else {
        // Other background threaad.
        if (eglMakeCurrent(
                appContext->mApplicationEgl->mEglDisplay,
                EGL_NO_SURFACE,
                EGL_NO_SURFACE,
                EGL_NO_CONTEXT) == EGL_FALSE) {
            LOGE("Failed to clear EGL.");
            return -1;
        }
    }

    TRACE_LOG("X");
    return 0;
}

/////////////////////////////////////////////////////////////////// EGL INITIALIZE / FINALIZE ////



//// GL INITIALIZE / FINALIZE ////////////////////////////////////////////////////////////////////

static void context_initialize_gl(app_context* appContext, JNIEnv* jenv) {
    TRACE_LOG("E");

    // Enable UI EGL.
    context_change_current_egl_to(appContext, appContext->mApplicationEgl->mEglSurfaceUi);

    // View.
    appContext->mViewMatrix = new float[16];
    // Projection.
    appContext->mProjectionMatrix = new float[16];

    // Initialize.
    Matrix4x4_SetIdentity(appContext->mViewMatrix);
    Matrix4x4_SetEyeView(
            appContext->mViewMatrix,
            0.0f,   0.0f,   100.0f,     // See from where.
            0.0f,   0.0f,     0.0f,     // Look at where.
            0.0f,   1.0f,     0.0f);    // Perpendicular axis.
    Matrix4x4_SetIdentity(appContext->mProjectionMatrix);
    Matrix4x4_SetParallelProjection(
            appContext->mProjectionMatrix,
            -1.0f,      // Left of near plane.
            1.0f,       // Right of near plane.
            -1.0f,      // Bottom of near plane.
            1.0f,       // Top of near plane.
            0.0f,       // Distance to near plane.
            200.0f);    // Distance to far plane.
//    Matrix4x4_SetFrustumProjection(
//            appContext->mProjectionMatrix,
//            -1.0f,
//            1.0f,
//            -1.0f,
//            1.0f,
//            50.0f,
//            150.0f);

    // Initialize ShaderProgramFactory.
    appContext->mShaderProgramFactory = new ShaderProgramFactory();
    appContext->mShaderProgramFactory->Initialize();

    // Gen shader code bytes.
    jstring vertexStr = gAppContext->mVertexShaderCodeString;
    const char* vertexBytes = jenv->GetStringUTFChars(vertexStr, NULL);
    size_t vertexBytesLen = jenv->GetStringLength(vertexStr);
    jstring fragmentStr = gAppContext->mFragmentShaderCodeString;
    const char* fragmentBytes = jenv->GetStringUTFChars(fragmentStr, NULL);
    size_t fragmentBytesLen = jenv->GetStringLength(fragmentStr);

    // SurfaceTexture renderer.
    appContext->mSurfaceTextureFrame = new SurfaceTextureFrame();
    appContext->mSurfaceTextureFrame->initialize(
            GL_VIEW_PORT_NORM_WIDTH,
            GL_VIEW_PORT_NORM_HEIGHT);
    GLuint surfaceTextureShader = appContext->mShaderProgramFactory->CreateShaderProgram(
            ShaderProgramFactory::ShaderType_SURFACE_TEXTURE,
            vertexBytes,
            vertexBytesLen,
            fragmentBytes,
            fragmentBytesLen);
    appContext->mSurfaceTextureFrame->setShaderProgram(surfaceTextureShader);

    // Release shader code bytes.
    jenv->ReleaseStringUTFChars(vertexStr, vertexBytes);
    jenv->ReleaseStringUTFChars(fragmentStr, fragmentBytes);

    // Return EGL to system default.
    context_return_egl_to_system_default(appContext);

    TRACE_LOG("X");
}

static void context_finalize_gl(app_context* appContext) {
    TRACE_LOG("E");

    // Enable UI EGL.
    context_change_current_egl_to(appContext, appContext->mApplicationEgl->mEglSurfaceUi);

    // Release and delete render target.
    if (appContext->mMainFrameRenderer != NULL) {
        appContext->mMainFrameRenderer->finalize();
        delete appContext->mMainFrameRenderer;
        appContext->mMainFrameRenderer = NULL;
    }
    if (appContext->mSurfaceTextureFrame != NULL) {
        appContext->mSurfaceTextureFrame->finalize();
        delete appContext->mSurfaceTextureFrame;
        appContext->mSurfaceTextureFrame = NULL;
    }

    // Finalize ShaderProgramFactory.
    if (appContext->mShaderProgramFactory != NULL) {
        appContext->mShaderProgramFactory->Finalize();
        delete appContext->mShaderProgramFactory;
        appContext->mShaderProgramFactory = NULL;
    }

    // View.
    if (appContext->mViewMatrix != NULL) {
        delete appContext->mViewMatrix;
        appContext->mViewMatrix = NULL;
    }
    // Projection.
    if (appContext->mProjectionMatrix != NULL) {
        delete appContext->mProjectionMatrix;
        appContext->mProjectionMatrix = NULL;
    }

    // Return EGL to system default.
    context_return_egl_to_system_default(appContext);

    TRACE_LOG("X");
}

//////////////////////////////////////////////////////////////////// GL INITIALIZE / FINALIZE ////



//// CAMERA RELATED //////////////////////////////////////////////////////////////////////////////

// onCameraPrepared()
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnCameraPrepared(
        JNIEnv* jenv,
        jclass clazz,
        jint frameWidth,
        jint frameHeight,
        jint imageFormat) {
    TRACE_LOG("E");

    // Main Frame.
    if (gAppContext->mMainFrame == NULL) {
        // Allocate.
        gAppContext->mMainFrame = new frame_buffer();
        // Initialize.
        gAppContext->mMainFrame->mIsValid = false;
    } else {
        LOGE("ERROR:Already main frame allocated.");
        return -1;
    }
    gAppContext->mMainFrame->mWidth = frameWidth;
    gAppContext->mMainFrame->mHeight = frameHeight;
    gAppContext->mMainFrame->mFormat = imageFormat;

    // Allocate frame buffer.
    gAppContext->mMainFrame->mBufferSize = frameWidth * frameHeight * 3 / 2; // YVU420SP

    // Allocate.
    gAppContext->mMainFrame->mBuffer = (unsigned char*)
            malloc(sizeof(unsigned char) * gAppContext->mMainFrame->mBufferSize);

    // Check.
    if (gAppContext->mMainFrame->mBuffer == NULL) {
        LOGE("ERROR:Memory Allocation error.");

        if (gAppContext->mMainFrame->mBuffer != NULL) {
            free(gAppContext->mMainFrame->mBuffer);
            gAppContext->mMainFrame->mBuffer = NULL;
        }

        return -1;
    }

    TRACE_LOG("X");
    return 0;
}

// onCameraReleased()
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnCameraReleased(
        JNIEnv* jenv,
        jclass clazz) {
    TRACE_LOG("E");

    // Preview should be stopped.
    gAppContext->mMainFrame->mIsValid = false;

    // Release buffer.
    if (gAppContext->mMainFrame->mBuffer != NULL) {
        free(gAppContext->mMainFrame->mBuffer);
        gAppContext->mMainFrame->mBuffer = NULL;
    }

    // Delete main frame.
    if (gAppContext->mMainFrame != NULL) {
        delete gAppContext->mMainFrame;
        gAppContext->mMainFrame = NULL;
    }

    TRACE_LOG("X");
    return 0;
}

// onCameraPreviewStreamUpdated
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnCameraPreviewStreamUpdated(
        JNIEnv* jenv,
        jclass clazz) {
    TRACE_LOG("E");

    // Check display availability.
    if (gAppContext->mApplicationEgl->mEglDisplay == NULL) {
//        LOGE("Display is not available.");
        return -1;
    }

    // Already application EGLContext is bound.

    context_render_camera_preview_stream(gAppContext);

    // Application EGLContext will be unbound.


    TRACE_LOG("X");
    return 0;
}

static void context_render_camera_preview_stream(app_context* appContext) {
    TRACE_LOG("E");

    // Clear color, (red, green, blue, alpha).
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // Clear buffer bit.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float globalMatrix[16];
    Matrix4x4_SetIdentity(globalMatrix);
    Matrix4x4_Multiply(globalMatrix, appContext->mViewMatrix, globalMatrix);
    Matrix4x4_Multiply(globalMatrix, appContext->mProjectionMatrix, globalMatrix);

    // Calc optimal through size.
    int finderWidth = 0;
    int finderHeight = 0;
    const float previewAspect
            = (float) appContext->mSurfaceCameraPreviewStreamWidth
            / (float) appContext->mSurfaceCameraPreviewStreamHeight;
    const float displayAspect
            = (float) appContext->mSurfaceUiWidth
            / (float) appContext->mSurfaceUiHeight;
    if (previewAspect < displayAspect) {
        // Screen is more wide. Preview height match screen height.
        finderHeight = appContext->mSurfaceUiHeight;
        finderWidth = finderHeight * previewAspect;
    } else {
        // Screen is more square. Preview width match screen width.
        finderWidth = appContext->mSurfaceUiWidth;
        finderHeight = finderWidth / previewAspect;
    }

/*
    // Debug block.
    LOGE("FinderWidth = %d", finderWidth);
    LOGE("FinderHeight = %d", finderHeight);
    LOGE("StreamWidth = %d", appContext->mSurfaceCameraPreviewStreamWidth);
    LOGE("StreamHeight = %d", appContext->mSurfaceCameraPreviewStreamHeight);
    LOGE("UI Width = %d", appContext->mSurfaceUiWidth);
    LOGE("UI Height = %d", appContext->mSurfaceUiHeight);
*/

    // Ratio. (Screen Height < Width)
    float finderWidthRatio = (float) finderWidth / (float) appContext->mSurfaceUiWidth;
    float finderHeightRatio = (float) finderHeight / (float) appContext->mSurfaceUiWidth;

    // Render.
    appContext->mSurfaceTextureFrame->setTextureId(appContext->mTextureCameraPreviewStream[0]);
    appContext->mSurfaceTextureFrame->setAlpha(1.0f);
    appContext->mSurfaceTextureFrame->setGlobalMatrix(globalMatrix);
    appContext->mSurfaceTextureFrame->scale(finderWidthRatio, finderHeightRatio, 1.0f);
    appContext->mSurfaceTextureFrame->render();

    if (checkGlError(__FUNCTION__) != GL_NO_ERROR) {
        LOGE("context_render_camera_preview_stream():[GL Error]");
    }

    glFlush();
    glFinish();

    // Swap buffer.
    eglSwapBuffers(
            appContext->mApplicationEgl->mEglDisplay,
            appContext->mApplicationEgl->mEglSurfaceUi);

    TRACE_LOG("X");
}

////////////////////////////////////////////////////////////////////////////// CAMERA RELATED ////






//// UTILS ///////////////////////////////////////////////////////////////////////////////////////

// Check UI thread.
static bool isMainThread() {
    return gettid() == getpid();
}

/////////////////////////////////////////////////////////////////////////////////////// UTILS ////



}; // namespace fezrestia

