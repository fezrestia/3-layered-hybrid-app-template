#ifndef SHADER_PROGRAM_FACTORY_H
#define SHADER_PROGRAM_FACTORY_H

#include <GLES2/gl2.h>

#include <android/asset_manager.h>

#define INVALID_PROGRAM 0

namespace fezrestia {

class ShaderProgramFactory {

public:
    // Shader program type.
    enum ShaderType {
        ShaderType_SINGLE_COLOR,
        ShaderType_YUV,
        ShaderType_SURFACE_TEXTURE,

        ShaderType_MAX
    };

public:
    // CONSTRUCTOR.
    ShaderProgramFactory();

    // DESTRUCTOR.
    ~ShaderProgramFactory();

    /**
     * Initialize.
     */
    void Initialize();

    /**
     * Finalize.
     */
    void Finalize();

    /**
     * Get shader program according to type.
     */
    GLuint CreateShaderProgram(AAssetManager* assetManager, ShaderType type);

private:
    // FIELDS ////////////////////////////////////////////////////////////////////////////////////

    GLuint mShaderPrograms[ShaderType_MAX];

    // FUNCTIONS /////////////////////////////////////////////////////////////////////////////////

    const char* getShaderSrcFileName(GLenum isVertOrFrag, ShaderType type);

    GLuint createProgram(
            GLchar* vertexSource, GLsizei vertexSourceLength,
            GLchar* fragmentSource, GLsizei fragmentSourceLength);

    GLuint loadShader(
            GLenum shaderType,
            const GLchar* source,
            GLsizei soruceLength);

    GLuint destroyProgram(GLuint program);

};

}; // namespace fezrestia

#endif // SHADER_PROGRAM_FACTORY_H
