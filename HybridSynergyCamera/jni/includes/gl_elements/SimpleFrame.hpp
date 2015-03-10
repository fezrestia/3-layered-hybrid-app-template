#ifndef SIMPLE_FRAME_H
#define SIMPLE_FRAME_H

#include "ElementBase.hpp"

namespace fezrestia {

class SimpleFrame : public ElementBase {

public:
    // CONSTRUCTOR.
    SimpleFrame();

    // DESTRUCTOR.
    ~SimpleFrame();

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
     * Set simple color, filled in all of this frame.
     */
    void setColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);

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
    GLuint mGLSL_uSimpleColor;

    /** Buffer objects. */
    GLuint mVertexBuffer;
    GLuint mTexCoordBuffer;

    /** Color vector. */
    GLfloat mColor[4];

    //// FUNCTIONS ///////////////////////////////////////////////////////////////////////////////

    GLuint enableLocalFunctions();

    GLuint enableShaderProgram();

    GLuint disableLocalFunctions();

    void doRender();

    void initializeShaderProgram();

    void initializeVertexAndTextureCoordinatesBuffer();

    void finalizeShaderProgram();

    void finalizeVertexAndTextureCoordinatesBuffer();
};

}; // namespace fezrestia

#endif // SIMPLE_FRAME_H
