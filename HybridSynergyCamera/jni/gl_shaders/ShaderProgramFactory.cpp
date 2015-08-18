#include "ShaderProgramFactory.hpp"

#include "TraceLog.h"

#include <sys/types.h>


namespace fezrestia {

// CONSTRUCTOR.
ShaderProgramFactory::ShaderProgramFactory() {
    // NOP.
}

// DESTRUCTOR.
ShaderProgramFactory::~ShaderProgramFactory() {
    // NOP.
}

/**
 * Initialize.
 */
void ShaderProgramFactory::Initialize() {
    // Reset.
    for (int i = 0; i < ShaderType_MAX; ++i) {
        mShaderPrograms[i] = INVALID_PROGRAM;
    }
}

/**
 * Finalize.
 */
void ShaderProgramFactory::Finalize() {
    // Release.
    for (int i = 0; i < ShaderType_MAX; ++i) {
        destroyProgram(mShaderPrograms[i]);
        mShaderPrograms[i] = INVALID_PROGRAM;
    }
}

/**
 * Get shader program according to type.
 */
GLuint ShaderProgramFactory::CreateShaderProgram(AAssetManager* assetManager, ShaderType type) {
    if (mShaderPrograms[type] != INVALID_PROGRAM) {
        // Already created.
        return mShaderPrograms[type];
    }

    TRACE_LOG("Create new ShaderProgram.");

    // Get vertex/fragment shader src file path.
    const char* vertexShaderSrcFile = getShaderSrcFileName(GL_VERTEX_SHADER, type);
    const char* fragmentShaderSrcFile = getShaderSrcFileName(GL_FRAGMENT_SHADER, type);

    if (vertexShaderSrcFile == NULL || fragmentShaderSrcFile == NULL) {
        LOGE("Unexpected ShaderType.");
        return INVALID_PROGRAM;
    }

    // Vertex.
    // Open assets.
    AAsset* vertexSrcAsset = AAssetManager_open(
            assetManager,
            vertexShaderSrcFile,
            AASSET_MODE_RANDOM);
    GLsizei vertSrcLen = AAsset_getLength(vertexSrcAsset);
    // Src codes.
    GLchar vertSrc[vertSrcLen + 1];
    // Read.
    AAsset_seek(vertexSrcAsset, 0, 0);
    AAsset_read(vertexSrcAsset, vertSrc, vertSrcLen);
    vertSrc[vertSrcLen] = NULL; // Terminal.
    // Close assets.
    AAsset_close(vertexSrcAsset);

    // Fragment.
    // Open assets.
    AAsset* fragmentSrcAsset = AAssetManager_open(
            assetManager,
            fragmentShaderSrcFile,
            AASSET_MODE_RANDOM);
    GLsizei fragSrcLen = AAsset_getLength(fragmentSrcAsset);
    // Src codes.
    GLchar fragSrc[fragSrcLen + 1];
    // Read.
    AAsset_seek(fragmentSrcAsset, 0, 0);
    AAsset_read(fragmentSrcAsset, fragSrc, fragSrcLen);
    fragSrc[fragSrcLen] = NULL; // Terminal.
    // Close assets.
    AAsset_close(fragmentSrcAsset);

    // Create program.
    mShaderPrograms[type] = createProgram(vertSrc, vertSrcLen + 1, fragSrc, fragSrcLen + 1);

    return mShaderPrograms[type];
}

const char* ShaderProgramFactory::getShaderSrcFileName(GLenum isVertOrFrag, ShaderType type) {
    switch (isVertOrFrag) {
        case GL_VERTEX_SHADER:
            switch (type) {
                case ShaderType_SINGLE_COLOR:
                    return "glsl_src/single_color_vertex.glsl";

                case ShaderType_YUV:
                    return "glsl_src/yvu420sp_vertex.glsl";

                case ShaderType_SURFACE_TEXTURE:
                    return "glsl_src/surface_texture_frame_vertex.glsl";
            }
            break;

        case GL_FRAGMENT_SHADER:
            switch (type) {
                case ShaderType_SINGLE_COLOR:
                    return "glsl_src/single_color_fragment.glsl";

                case ShaderType_YUV:
                    return "glsl_src/yvu420sp_fragment.glsl";

                case ShaderType_SURFACE_TEXTURE:
                    return "glsl_src/surface_texture_frame_fragment.glsl";
            }
            break;
    }

    LOGE("Unexpected ShaderType");
    return NULL;
}

GLuint ShaderProgramFactory::createProgram(
        GLchar* vertexSource, GLsizei vertexSourceLength,
        GLchar* fragmentSource, GLsizei fragmentSourceLength) {
    // Load and compile shaders.
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource, vertexSourceLength);
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource, fragmentSourceLength);

    if (vertexShader == 0 || fragmentShader == 0) {
        LOGE("Failed to loadShader()");
        if (vertexShader != 0) {
            glDeleteShader(vertexShader);
        }
        if (fragmentShader != 0) {
            glDeleteShader(fragmentShader);
        }

        return INVALID_PROGRAM;
    }

    // Create and link program.
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    checkGlError(__FUNCTION__);

    if (vertexShader != 0) {
        glDeleteShader(vertexShader);
    }
    if (fragmentShader != 0) {
        glDeleteShader(fragmentShader);
    }

    GLint isSuccess = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &isSuccess);
    if (isSuccess != GL_TRUE) {
        LOGE("Failed to link program.");

        GLint logLength;
        GLint writtenLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        char pLog[256];
        if (256 < logLength) logLength = 256;
        glGetProgramInfoLog(program, logLength, &writtenLength, pLog);

        if (program != 0) {
            destroyProgram(program);
        }

        LOGE("ProgramInfoLog:%s", pLog);
        return INVALID_PROGRAM;
    }

    return program;
}

GLuint ShaderProgramFactory::loadShader(
        GLenum isVertOrFrag,
        const GLchar* source,
        GLsizei sourceLength) {
    GLuint shader = glCreateShader(isVertOrFrag);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    checkGlError(__FUNCTION__);

    GLint isSuccess = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isSuccess);
    if (isSuccess != GL_TRUE) {
        LOGE("Failed to compile shader.");

        GLint logLength;
        GLint writtenLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        char pLog[256];
        if (256 < logLength) logLength = 256;
        glGetShaderInfoLog(shader, logLength, &writtenLength, pLog);

        LOGE("ShaderInfoLog:%s", pLog);
    }

    return shader;
}

GLuint ShaderProgramFactory::destroyProgram(GLuint program) {
    if (program != 0) {
        glDeleteProgram(program);
        checkGlError(__FUNCTION__);
    }
}

}; // namespace fezrestia
