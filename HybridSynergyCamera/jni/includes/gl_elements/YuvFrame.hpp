#ifndef YUV_FRAME_H
#define YUV_FRAME_H

#include "ElementBase.hpp"

// Texture IDs.
#define TEXTURE_INDEX_Y 0
#define TEXTURE_INDEX_VU 1

// Texture count.
#define NUM_OF_TEXTURES 2

namespace fezrestia {

class YuvFrame : public ElementBase {

public:

    /**
     * YUV format definitions.
     */
    enum Format {
        YuvFrame_Format_YVU420SP,
        YuvFrame_Format_END,
    };

    // CONSTRUCTOR.
    YuvFrame();

    // DESTRUCTOR.
    ~YuvFrame();

    /**
     * Initialize.
     */
    void initialize(float screenNormWidth, float screenNormHeight);

    /**
     * Finalize.
     */
    void finalize();

    /**
     * Set shader program index.
     */
    void setShaderProgram(GLuint shaderProgram);

    /**
     * Set texture YUV buffers.
     */
    void setTextureYuv(
        Format format,
        GLvoid* buffer,
        GLsizei width,
        GLsizei height);

    /**
     * Set alpha channel.
     */
    void setAlpha(GLfloat alpha);

    /**
     * GL call for render.
     */
    void render();

private:
    //// MEMBER FIELDS ///////////////////////////////////////////////////////////////////////////

    /** Shader program. */
    GLuint mShaderProgram;

    /** GLSL index for background texture. */
    GLuint mGLSL_aVertex;
    GLuint mGLSL_aTexCoord;
    GLuint mGLSL_uMvpMatrix;
    GLuint mGLSL_uTextureY;
    GLuint mGLSL_uTextureVu;
    GLuint mGLSL_uAlpha;

    /** Buffer objects. */
    GLuint mVertexBuffer;
    GLuint mTexCoordBuffer;
    GLuint mTextureBuffers[NUM_OF_TEXTURES];

    /** Alpha. */
    GLfloat mAlpha;

    //// FUNCTIONS ///////////////////////////////////////////////////////////////////////////////

    void initializeTextures();

    void finalizeTextures();

    GLuint enableLocalFunctions();

    GLuint enableShaderProgram();

    GLuint disableLocalFunctions();

    void updateTextureBuffer(
            GLuint textureId,
            GLsizei width,
            GLsizei height,
            GLvoid* buffer,
            GLenum bufferFormat);

    void doRender();

    void initializeShaderProgram();

    void initializeVertexAndTextureCoordinatesBuffer();

    void finalizeShaderProgram();

    void finalizeVertexAndTextureCoordinatesBuffer();
};

}; // namespace fezrestia

#endif // YUV_FRAME_H
