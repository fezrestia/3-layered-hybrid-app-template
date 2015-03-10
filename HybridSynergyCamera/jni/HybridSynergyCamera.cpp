#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
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



app_context* gAppContext = NULL;



//// ACTIVITY RELATED ////////////////////////////////////////////////////////////////////////////

// onActivityCreated()
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnActivityCreated(
        JNIEnv* jenv,
        jclass clazz,
        jobject jAssetManager) {
    TRACE_LOG("E");

    gAppContext = new app_context();

    // JNI Env.
    gAppContext->mJNIEnv = jenv;

    // Activity.
    jclass activityClazz = gAppContext->mJNIEnv->FindClass(
            "com/fezrestia/android/hybridsynergycamera/HybridSynergyCameraActivity");
    jmethodID methodId = gAppContext->mJNIEnv->GetStaticMethodID(
            activityClazz,
            "sendCommandFromNative",
            "(II)V");
    gAppContext->mActivityClazz = activityClazz;
    gAppContext->mActivityCallbackMethodId = methodId;
    gAppContext->mJNIEnv->DeleteLocalRef(activityClazz);

    // Asset.
    gAppContext->mAndroidAssetManager = AAssetManager_fromJava(
            gAppContext->mJNIEnv,
            jAssetManager);

    TRACE_LOG("X");
}

// onActivityDestroyed()
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnActivityDestroyed(
        JNIEnv* jenv,
        jclass clazz) {
    TRACE_LOG("E");

    if (gAppContext != NULL) {
        delete gAppContext;
        gAppContext = NULL;
    }

    TRACE_LOG("X");
}

//////////////////////////////////////////////////////////////////////////// ACTIVITY RELATED ////



//// SURFACE RELATED /////////////////////////////////////////////////////////////////////////////

// onSurfaceInitialized()
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnSurfaceInitialized(
        JNIEnv* jenv,
        jclass clazz,
        jobject surface) {
    TRACE_LOG("E");

    // Native window.
    gAppContext->mTargetNativeWindow = ANativeWindow_fromSurface(jenv, surface);

    // Initialize EGL.
    context_initialize_egl(gAppContext);

    // Initialize GL.
    context_initialize_gl(gAppContext);

    TRACE_LOG("X");
    return 0;
}

// onSurfaceFinalized()
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnSurfaceFinalized(
        JNIEnv* jenv,
        jclass clazz) {
    TRACE_LOG("E");

    // Finalize GL.
    context_finalize_gl(gAppContext);

    // Finalize EGL.
    context_finalize_egl(gAppContext);

    if (gAppContext->mTargetNativeWindow != NULL) {
        ANativeWindow_release(gAppContext->mTargetNativeWindow);
        gAppContext->mTargetNativeWindow = NULL;
    }

    TRACE_LOG("X");
    return 0;
}

///////////////////////////////////////////////////////////////////////////// SURFACE RELATED ////














static void context_initialize_gl(app_context* appContext) {
    TRACE_LOG("E");

    // Enable this EGL.
    if (eglMakeCurrent(
            gAppContext->mThisEgl->mEglDisplay,
            appContext->mThisEgl->mEglReadSurface,
            appContext->mThisEgl->mEglDrawSurface,
            appContext->mThisEgl->mEglContext) == EGL_FALSE) {
        LOGE("Failed to enable this EGL.");
        return;
    }

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

    // Initialize render target elements.
    appContext->mMainFrameRenderer = new YuvFrame();
    appContext->mMainFrameRenderer->initialize(GL_VIEW_PORT_NORM_WIDTH, GL_VIEW_PORT_NORM_HEIGHT);
    GLuint yuvShader = appContext->mShaderProgramFactory->CreateShaderProgram(
            appContext->mAndroidAssetManager,
            ShaderProgramFactory::ShaderType_YUV);
    appContext->mMainFrameRenderer->setShaderProgram(yuvShader);

    // Return EGL to system default.
    if (isMainThread()) {
        // Main thread.
        if (eglMakeCurrent(
                gAppContext->mSystemDefaultEgl->mEglDisplay,
                appContext->mSystemDefaultEgl->mEglReadSurface,
                appContext->mSystemDefaultEgl->mEglDrawSurface,
                appContext->mSystemDefaultEgl->mEglContext) == EGL_FALSE) {
            LOGE("Failed to return EGL to System Default.");
        }
    } else {
        // Other background threaad.
        if (eglMakeCurrent(
                eglGetCurrentDisplay(),
                EGL_NO_SURFACE,
                EGL_NO_SURFACE,
                EGL_NO_CONTEXT) == EGL_FALSE) {
            LOGE("Failed to clear EGL.");
        }
    }

    TRACE_LOG("X");
}

static void context_finalize_gl(app_context* appContext) {
    TRACE_LOG("E");

    // Enable this EGL.
    if (eglMakeCurrent(
            gAppContext->mThisEgl->mEglDisplay,
            appContext->mThisEgl->mEglReadSurface,
            appContext->mThisEgl->mEglDrawSurface,
            appContext->mThisEgl->mEglContext) == EGL_FALSE) {
        LOGE("Failed to enable this EGL.");
        return;
    }

    // Release and delete render target.
    if (appContext->mMainFrameRenderer != NULL) {
        appContext->mMainFrameRenderer->finalize();
        delete appContext->mMainFrameRenderer;
        appContext->mMainFrameRenderer = NULL;
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
    if (isMainThread()) {
        // Main thread.
        if (eglMakeCurrent(
                gAppContext->mSystemDefaultEgl->mEglDisplay,
                appContext->mSystemDefaultEgl->mEglReadSurface,
                appContext->mSystemDefaultEgl->mEglDrawSurface,
                appContext->mSystemDefaultEgl->mEglContext) == EGL_FALSE) {
            LOGE("Failed to return EGL to System Default.");
        }
    } else {
        // Other background threaad.
        if (eglMakeCurrent(
                eglGetCurrentDisplay(),
                EGL_NO_SURFACE,
                EGL_NO_SURFACE,
                EGL_NO_CONTEXT) == EGL_FALSE) {
            LOGE("Failed to clear EGL.");
        }
    }

    TRACE_LOG("X");
}





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
    appContext->mSystemDefaultEgl = new egl_pack();
    appContext->mSystemDefaultEgl->mEglDisplay = eglGetCurrentDisplay();
    appContext->mSystemDefaultEgl->mEglReadSurface = eglGetCurrentSurface(EGL_READ);
    appContext->mSystemDefaultEgl->mEglDrawSurface = eglGetCurrentSurface(EGL_DRAW);
    appContext->mSystemDefaultEgl->mEglContext = eglGetCurrentContext();

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

    // Get EGL window surface.
    EGLSurface surface = eglCreateWindowSurface(
            display,
            config,
            gAppContext->mTargetNativeWindow,
            NULL);
    // Get EGL rendering context.
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, eglContextAttribs);

    // Cache this EGL.
    appContext->mThisEgl = new egl_pack();
    appContext->mThisEgl->mEglDisplay = display;
    appContext->mThisEgl->mEglContext = context;
    appContext->mThisEgl->mEglReadSurface = surface;
    appContext->mThisEgl->mEglDrawSurface = surface;

    // Enable this EGL.
    if (eglMakeCurrent(
            appContext->mThisEgl->mEglDisplay,
            appContext->mThisEgl->mEglReadSurface,
            appContext->mThisEgl->mEglDrawSurface,
            appContext->mThisEgl->mEglContext) == EGL_FALSE) {
        LOGE("Failed to enable this EGL.");
        return -1;
    }

   // Get resolution.
    EGLint w, h;
    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);
    appContext->mDisplayWidth = w;
    appContext->mDisplayHeight = h;

    // Set GL view port.
    if (h < w) {
        // This is Landscape.
        EGLint verticalOffset = (w - h) / 2;
        glViewport(0, -1 * verticalOffset, w, w); // Square.
    } else {
        // This is Portrait.
        EGLint horizontalOffset = (h - w) / 2;
        glViewport(-1 * horizontalOffset, 0, h, h); // Square.
    }

    // Return EGL to system default.
    if (isMainThread()) {
        // Main thread.
        if (eglMakeCurrent(
                gAppContext->mSystemDefaultEgl->mEglDisplay,
                appContext->mSystemDefaultEgl->mEglReadSurface,
                appContext->mSystemDefaultEgl->mEglDrawSurface,
                appContext->mSystemDefaultEgl->mEglContext) == EGL_FALSE) {
            LOGE("Failed to return EGL to System Default.");
        }
    } else {
        // Other background threaad.
        if (eglMakeCurrent(
                eglGetCurrentDisplay(),
                EGL_NO_SURFACE,
                EGL_NO_SURFACE,
                EGL_NO_CONTEXT) == EGL_FALSE) {
            LOGE("Failed to clear EGL.");
        }
    }

    TRACE_LOG("X");
    return 0;
}

static void context_finalize_egl(app_context* appContext) {
    TRACE_LOG("E");

    // Return EGL to system default.
    if (isMainThread()) {
        // Main thread.
        if (eglMakeCurrent(
                gAppContext->mSystemDefaultEgl->mEglDisplay,
                appContext->mSystemDefaultEgl->mEglReadSurface,
                appContext->mSystemDefaultEgl->mEglDrawSurface,
                appContext->mSystemDefaultEgl->mEglContext) == EGL_FALSE) {
            LOGE("Failed to return EGL to System Default.");
        }
    } else {
        // Other background threaad.
        if (eglMakeCurrent(
                eglGetCurrentDisplay(),
                EGL_NO_SURFACE,
                EGL_NO_SURFACE,
                EGL_NO_CONTEXT) == EGL_FALSE) {
            LOGE("Failed to clear EGL.");
        }
    }

    // Release system default EGL cache.
    if (appContext->mSystemDefaultEgl != NULL) {
        delete appContext->mSystemDefaultEgl;
        appContext->mSystemDefaultEgl = NULL;
    }

    // Release this EGL.
    if (appContext->mThisEgl != NULL) {
        eglDestroyContext(
            appContext->mThisEgl->mEglDisplay,
            appContext->mThisEgl->mEglContext);
        eglDestroySurface(
            appContext->mThisEgl->mEglDisplay,
            appContext->mThisEgl->mEglReadSurface); // Same as draw surface.
        eglTerminate(appContext->mThisEgl->mEglDisplay);

        delete appContext->mThisEgl;
        appContext->mThisEgl = NULL;
    }

    TRACE_LOG("X");
}




static void context_render_frame(app_context* appContext) {
    // Check display availability.
    if (appContext->mThisEgl->mEglDisplay == NULL) {
//        LOGE("Display is not available.");
        return;
    }

    // Enable this EGL.
    if (eglMakeCurrent(
            appContext->mThisEgl->mEglDisplay,
            appContext->mThisEgl->mEglReadSurface,
            appContext->mThisEgl->mEglDrawSurface,
            appContext->mThisEgl->mEglContext) == EGL_FALSE) {
        LOGE("Failed to enable this EGL.");
        return;
    }

    // Clear frame buffer.
    // Color, (red, green, blue, alpha).
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // Buffer bit.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float globalMatrix[16];
    Matrix4x4_SetIdentity(globalMatrix);
    Matrix4x4_Multiply(globalMatrix, appContext->mViewMatrix, globalMatrix);
    Matrix4x4_Multiply(globalMatrix, appContext->mProjectionMatrix, globalMatrix);

    if (appContext->mMainFrameRenderer != NULL
            && appContext->mMainFrame != NULL
            && appContext->mMainFrame->mIsValid) {
        // Calc optimal through size.
        int finderWidth = 0;
        int finderHeight = 0;

        const float previewAspect
                = (float) appContext->mMainFrame->mHeight / (float) appContext->mMainFrame->mWidth;
        const float displayAspect
                = (float) appContext->mDisplayHeight / (float) appContext->mDisplayWidth;
        if (previewAspect < displayAspect) {
            // Screen is more square. Preview width match screen width.
            finderWidth = appContext->mDisplayWidth;
            finderHeight = finderWidth * previewAspect;
        } else {
            // Screen is more wide. Preview height match screen height.
            finderHeight = appContext->mDisplayHeight;
            finderWidth = finderHeight / previewAspect;
        }

        // Ratio. (Screen Height < Width)
        float finderWidthRatio = (float) finderWidth / (float) appContext->mDisplayWidth;
        float finderHeightRatio = (float) finderHeight / (float) appContext->mDisplayWidth;

        // Render.
        appContext->mMainFrameRenderer->setTextureYuv(
                YuvFrame::YuvFrame_Format_YVU420SP,
                appContext->mMainFrame->mBuffer,
                appContext->mMainFrame->mWidth,
                appContext->mMainFrame->mHeight);
        appContext->mMainFrameRenderer->setAlpha(1.0f);
        appContext->mMainFrameRenderer->setGlobalMatrix(globalMatrix);
        appContext->mMainFrameRenderer->scale(finderWidthRatio, finderHeightRatio, 1.0f);
        appContext->mMainFrameRenderer->render();
    }

    // Swap buffer.
    eglSwapBuffers(appContext->mThisEgl->mEglDisplay, appContext->mThisEgl->mEglDrawSurface);

    // Return EGL to system default.
    if (isMainThread()) {
        // Main thread.
        if (eglMakeCurrent(
                gAppContext->mSystemDefaultEgl->mEglDisplay,
                appContext->mSystemDefaultEgl->mEglReadSurface,
                appContext->mSystemDefaultEgl->mEglDrawSurface,
                appContext->mSystemDefaultEgl->mEglContext) == EGL_FALSE) {
            LOGE("Failed to return EGL to System Default.");
        }
    } else {
        // Other background threaad.
        if (eglMakeCurrent(
                eglGetCurrentDisplay(),
                EGL_NO_SURFACE,
                EGL_NO_SURFACE,
                EGL_NO_CONTEXT) == EGL_FALSE) {
            LOGE("Failed to clear EGL.");
        }
    }
}




static void sendCommandToJava(app_context* appContext, int32_t command, int32_t arg) {
    TRACE_LOG("E");

    appContext->mJNIEnv->CallStaticVoidMethod(
            appContext->mActivityClazz,
            appContext->mActivityCallbackMethodId,
            command,
            arg);

    TRACE_LOG("X");
}






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

// onPreviewFrameUpdated()
extern "C" JNIEXPORT jint JNICALL Java_com_fezrestia_android_hybridsynergycamera_HybridSynergyCameraActivity_nativeOnPreviewFrameUpdated(
        JNIEnv* jenv,
        jclass clazz,
        jint frameWidth,
        jint frameHeight,
        jint imageFormat,
        jbyteArray frameBuffer,
        jint frameBufferOffset,
        jint frameBufferSize) {
    TRACE_LOG("E");

    // Check.
    if (gAppContext->mMainFrame->mBufferSize < frameBufferSize) {
        LOGE("ERROR:Frame buffer size is too large. allocated=%d, handed=%d",
                gAppContext->mMainFrame->mBufferSize,
                frameBufferSize);
        return -1;
    }

    // Cache.
    unsigned char* bufJava = (unsigned char*) jenv->GetPrimitiveArrayCritical(frameBuffer, NULL);
    memcpy(
            gAppContext->mMainFrame->mBuffer,
//            bufJava + frameBufferOffset, //TODO: Why is this offset not necessary ?
            bufJava,
            frameBufferSize * sizeof(unsigned char));
    jenv->ReleasePrimitiveArrayCritical(frameBuffer, bufJava, JNI_ABORT);

    gAppContext->mIsRenderRequested = true;
    gAppContext->mMainFrame->mIsValid = true;




    context_render_frame(gAppContext);



    TRACE_LOG("X");
    return 0;
}

////////////////////////////////////////////////////////////////////////////// CAMERA RELATED ////




// Check UI thread.
static bool isMainThread() {
    return gettid() == getpid();
}



}; // namespace fezrestia

