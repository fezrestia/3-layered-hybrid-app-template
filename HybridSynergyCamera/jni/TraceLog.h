#ifndef TRACE_LOG_H
#define TRACE_LOG_H

#include <time.h>
#include <android/log.h>

#define TAG "TraceLog"

//#define TRACE_LOG(...) LOGE("[CLOCK/1000=%d] %s:[%s]", (int32_t) clock() / 1000, __FUNCTION__, __VA_ARGS__)
#define TRACE_LOG(...)

#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))

/**
 * Check GL error.
 */
static GLenum checkGlError(const char* operation) {
    GLenum error = GL_NO_ERROR;
    error = glGetError();

    switch (error) {
        case GL_INVALID_ENUM:
            LOGE("GL ERROR : %s() : [GL_INVALID_ENUM]", operation);
            break;

        case GL_INVALID_VALUE:
            LOGE("GL ERROR : %s() : [GL_INVALID_VALUE]", operation);
            break;

        case GL_INVALID_OPERATION:
            LOGE("GL ERROR : %s() : [GL_INVALID_OPERATION]", operation);
            break;

        case GL_INVALID_FRAMEBUFFER_OPERATION:
            LOGE("GL ERROR : %s() : [GL_INVALID_FRAMEBUFFER_OPERATION]", operation);
            break;

        case GL_OUT_OF_MEMORY:
            LOGE("GL ERROR : %s() : [GL_OUT_OF_MEMORY]", operation);
            break;
    }

    return error;
}

#endif // TRACE_LOG_H
