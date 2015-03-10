#include <GLES2/gl2.h>

#include "SimpleFrame.hpp"

#include "android_opengl_matrix.h"
#include "TraceLog.h"

namespace fezrestia {

// CONSTRUCTOR.
SimpleFrame::SimpleFrame() {
    // NOP.
}

// DESTRUCTOR.
SimpleFrame::~SimpleFrame() {
    // NOP.
}

/**
 * Initialize.
 */
void SimpleFrame::initialize(float screenNormWidth, float screenNormHeight) {
    // Super.
    ElementBase::initialize(screenNormWidth, screenNormHeight);
}

/**
 * Finalize.
 */
void SimpleFrame::finalize() {
    // Super.
    ElementBase::finalize();

    // Delete shader programs.
    finalizeShaderProgram();
}

/**
 * Set shader program index.
 */
void SimpleFrame::setShaderProgram(GLuint shaderProgram) {
    // Setup shader.
    mShaderProgram = shaderProgram;

    // Create and initialize shader and program.
    initializeShaderProgram();
}

/**
 * Set simple color, filled in all of this frame.
 */
void SimpleFrame::setColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    mColor[0] = red;
    mColor[1] = green;
    mColor[2] = blue;
    mColor[3] = alpha;
}

/**
 * GL call for render.
 */
void SimpleFrame::render() {
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

GLuint SimpleFrame::enableLocalFunctions() {
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

GLuint SimpleFrame::enableShaderProgram() {
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

GLuint SimpleFrame::disableLocalFunctions() {
    // Vertex / Texture for background.
    glDisableVertexAttribArray(mGLSL_aVertex);
    glDisableVertexAttribArray(mGLSL_aTexCoord);

    // No error.
    return GL_TRUE;
}

void SimpleFrame::doRender() {
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

    // Inject color vector.
    glUniform4f(mGLSL_uSimpleColor, mColor[0], mColor[1], mColor[2], mColor[3]);

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

void SimpleFrame::initializeShaderProgram() {
    TRACE_LOG("E");

    // Link vertex information with field of shader source codes.
    mGLSL_aVertex = glGetAttribLocation(mShaderProgram, "aVertex");
    // Link texture information with field of shader source codes.
    mGLSL_aTexCoord = glGetAttribLocation(mShaderProgram, "aTexCoord");
    // Link MVP matrix.
    mGLSL_uMvpMatrix = glGetUniformLocation(mShaderProgram, "uMvpMatrix");
    // Link Color vector.
    mGLSL_uSimpleColor = glGetUniformLocation(mShaderProgram, "uSimpleColor");

    checkGlError(__FUNCTION__);

    // Create vertex and texture coordinates buffer objects.
    initializeVertexAndTextureCoordinatesBuffer();

    TRACE_LOG("X");
}

void SimpleFrame::initializeVertexAndTextureCoordinatesBuffer() {
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

void SimpleFrame::finalizeShaderProgram() {
    mShaderProgram = 0;

    // Delete vertex and texture coordinates buffer objects.
    finalizeVertexAndTextureCoordinatesBuffer();
}

void SimpleFrame::finalizeVertexAndTextureCoordinatesBuffer() {
    // Delete buffer objects.
    glDeleteBuffers(
            1, // Size
            &mVertexBuffer); // Buffer
    glDeleteBuffers(
            1, // Size
            &mTexCoordBuffer); // Buffer
}

}; // namespace fezrestia
