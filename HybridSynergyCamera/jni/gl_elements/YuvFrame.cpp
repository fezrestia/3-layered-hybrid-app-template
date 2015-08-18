#include <GLES2/gl2.h>

#include "YuvFrame.hpp"

#include "android_opengl_matrix.h"
#include "TraceLog.h"

namespace fezrestia {

// CONSTRUCTOR.
YuvFrame::YuvFrame() {
    // NOP.
}

// DESTRUCTOR.
YuvFrame::~YuvFrame() {
    // NOP.
}

/**
 * Initialize.
 */
void YuvFrame::initialize(float screenNormWidth, float screenNormHeight) {
    // Super.
    ElementBase::initialize(screenNormWidth, screenNormHeight);

    // Create texture buffers.
    initializeTextures();
}

/**
 * Finalize.
 */
void YuvFrame::finalize() {
    // Super.
    ElementBase::finalize();

    // Delete shader programs.
    finalizeShaderProgram();

    // Delete texture buffers.
    finalizeTextures();
}

/**
 * Set shader program index.
 */
void YuvFrame::setShaderProgram(GLuint shaderProgram) {
    // Setup shader.
    mShaderProgram = shaderProgram;

    // Create and initialize shader and program.
    initializeShaderProgram();
}

/**
 * Initialize texture buffers.
 */
void YuvFrame::initializeTextures() {
    glGenTextures(NUM_OF_TEXTURES, mTextureBuffers);
}

/**
 * Finalize texture buffers.
 */
void YuvFrame::finalizeTextures() {
    glDeleteTextures(NUM_OF_TEXTURES, mTextureBuffers);
}

/**
 * Set texture YUV buffers.
 */
void YuvFrame::setTextureYuv(
        Format format,
        GLvoid* buffer,
        GLsizei width,
        GLsizei height) {

    GLvoid* bufY = NULL;
    GLvoid* bufVu = NULL;

    switch (format) {
        case YuvFrame_Format_YVU420SP:
            bufY = buffer;
            bufVu = buffer + width * height;
            break;

        default:
            // NOP.
            break;
    }

    if (bufY == NULL || bufVu == NULL) {
        LOGE("setTextureYuv():[Buffer is NULL]");
        return;
    }

    updateTextureBuffer(
            mTextureBuffers[TEXTURE_INDEX_Y],
            width,
            height,
            bufY,
            GL_LUMINANCE);
    updateTextureBuffer(
            mTextureBuffers[TEXTURE_INDEX_VU],
            width / 2,
            height / 2,
            bufVu,
            GL_LUMINANCE_ALPHA);
}

/**
 * Update texture buffer.
 */
void YuvFrame::updateTextureBuffer(
        GLuint textureId,
        GLsizei width,
        GLsizei height,
        GLvoid* buffer,
        GLenum bufferFormat) {
    // Link texture to target.
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Magnification type. Linear interpolation for performance.
    glTexParameterf(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);
    // Minimization type. Linear interpolation for performance.
    glTexParameterf(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR);

    // 2D texture image.
    glTexImage2D(
            GL_TEXTURE_2D, // Target.
            0, // Detail level, 0 is base image.
            bufferFormat, // Format.
            width, // Input image width.
            height, // Input image height.
            0, // Width border. 0 is must.
            bufferFormat, // Format. Must be same as above format.
            GL_UNSIGNED_BYTE, // Data type.
            buffer); // Target data.

    // Un-link texture from target.
    glBindTexture(GL_TEXTURE_2D, 0);
}

/**
 * Set alpha channel.
 */
void YuvFrame::setAlpha(GLfloat alpha) {
    mAlpha = alpha;
}

/**
 * GL call for render.
 */
void YuvFrame::render() {
    if (!isVisible()) {
        // Do not render.
        return;
    }

    if (!enableLocalFunctions()) {
        LOGE("render():[Enable functions failed.]");
        return;
    }

    doRender();

    if (!disableLocalFunctions()) {
        LOGE("render():[Disable functions failed.]");
        return;
    }
}

GLuint YuvFrame::enableLocalFunctions() {
    // Vertex / Texture for background.
    glEnableVertexAttribArray(mGLSL_aVertex);
    glEnableVertexAttribArray(mGLSL_aTexCoord);

    // Enable shader.
    if (!enableShaderProgram()) {
        LOGE("enableFunctions():[Enable shader program failed.]");
        return GL_FALSE;
    }

    // No error.
    return GL_TRUE;
}

GLuint YuvFrame::enableShaderProgram() {
    // Install program object to GL renderer and validate.
    if (mShaderProgram == 0) {
        LOGE("enableShaderProgram():[Program is Invalid]");
        return GL_FALSE;
    }
    glUseProgram(mShaderProgram);
    glValidateProgram(mShaderProgram);

    if (checkGlError(__FUNCTION__) != GL_NO_ERROR) {
        LOGE("enableShaderProgram():[Program Error]");
        return GL_FALSE;
    }

    return GL_TRUE;
}

GLuint YuvFrame::disableLocalFunctions() {
    // Vertex / Texture for background.
    glDisableVertexAttribArray(mGLSL_aVertex);
    glDisableVertexAttribArray(mGLSL_aTexCoord);

    // No error.
    return GL_TRUE;
}

void YuvFrame::doRender() {
    TRACE_LOG("E");

    // Register vertex.
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    glVertexAttribPointer(
            mGLSL_aVertex,
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Register tex-coord.
    glBindBuffer(GL_ARRAY_BUFFER, mTexCoordBuffer);
    glVertexAttribPointer(
            mGLSL_aTexCoord,
            2,
            GL_FLOAT,
            GL_FALSE,
            0,
            0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Activate and bind texture.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextureBuffers[TEXTURE_INDEX_Y]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mTextureBuffers[TEXTURE_INDEX_VU]);

    if (checkGlError(__FUNCTION__) != GL_NO_ERROR) {
        LOGE("doRender():[Bind textures Error]");
        return;
    }

    // Link texture id and field of shader source codes.
    glUniform1i(mGLSL_uTextureY, TEXTURE_INDEX_Y);
    glUniform1i(mGLSL_uTextureVu, TEXTURE_INDEX_VU);

    // Link alpha channel.
    glUniform1f(mGLSL_uAlpha, mAlpha);

    // MVP matrix.
    float mvpMatrix[16];
    Matrix4x4_SetIdentity(mvpMatrix);
    // Global x Local matrix.
    Matrix4x4_Multiply(mvpMatrix, getSequencedLocalMatrix(), mvpMatrix);
    Matrix4x4_Multiply(mvpMatrix, getGlobalMatrix(), mvpMatrix);
    glUniformMatrix4fv(
            mGLSL_uMvpMatrix, // Location
            1, // Length
            GL_FALSE,
            mvpMatrix);

    // Render.
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    checkGlError(__FUNCTION__);
    TRACE_LOG("X");
}

void YuvFrame::initializeShaderProgram() {
    TRACE_LOG("E");

    // Link vertex information with field of shader source codes.
    mGLSL_aVertex = glGetAttribLocation(mShaderProgram, "aVertex");
    // Link texture information with field of shader source codes.
    mGLSL_aTexCoord = glGetAttribLocation(mShaderProgram, "aTexCoord");
    // Link MVP matrix.
    mGLSL_uMvpMatrix = glGetUniformLocation(mShaderProgram, "uMvpMatrix");
    // Link Texture Y.
    mGLSL_uTextureY = glGetUniformLocation(mShaderProgram, "uTextureY");
    // Link Texture VU.
    mGLSL_uTextureVu = glGetUniformLocation(mShaderProgram, "uTextureVu");
    // Link alpha.
    mGLSL_uAlpha = glGetUniformLocation(mShaderProgram, "uAlpha");

    checkGlError(__FUNCTION__);

    // Create vertex and texture coordinates buffer objects.
    initializeVertexAndTextureCoordinatesBuffer();

    TRACE_LOG("X");
}

void YuvFrame::initializeVertexAndTextureCoordinatesBuffer() {
    const GLuint vertexBufLen = 12;
    const GLuint texCoordBufLen = 8;

    float vertex[vertexBufLen] = {
            - getScreenNormWidth() / 2.0f,    getScreenNormHeight() / 2.0f, 0.0f, // Left-Top
            - getScreenNormWidth() / 2.0f,  - getScreenNormHeight() / 2.0f, 0.0f, // Left-Bottom
              getScreenNormWidth() / 2.0f,    getScreenNormHeight() / 2.0f, 0.0f, // Right-Top
              getScreenNormWidth() / 2.0f,  - getScreenNormHeight() / 2.0f, 0.0f, // Right-Bottom
    };
    float texCoord[texCoordBufLen] = {
            0.0f,   0.0f, // Left-Top
            0.0f,   1.0f, // Left-Bottom
            1.0f,   0.0f, // Right-Top
            1.0f,   1.0f, // Right-Bottom
    };

    // Create buffer object.
    glGenBuffers(1, &mVertexBuffer);
    glGenBuffers(1, &mTexCoordBuffer);

    // Bind and write vertex.
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(GLfloat) * vertexBufLen,
            vertex,
            GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Bind and write tex-coord.
    glBindBuffer(GL_ARRAY_BUFFER, mTexCoordBuffer);
    glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(GLfloat) * texCoordBufLen,
            texCoord,
            GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    checkGlError(__FUNCTION__);
}

void YuvFrame::finalizeShaderProgram() {
    mShaderProgram = 0;

    // Delete vertex and texture coordinates buffer objects.
    finalizeVertexAndTextureCoordinatesBuffer();
}

void YuvFrame::finalizeVertexAndTextureCoordinatesBuffer() {
    // Delete buffer objects.
    glDeleteBuffers(
            1, // Size
            &mVertexBuffer); // Buffer
    glDeleteBuffers(
            1, // Size
            &mTexCoordBuffer); // Buffer
}

}; // namespace fezrestia
