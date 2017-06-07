#ifndef SHADER_PROGRAM_FACTORY_H
#define SHADER_PROGRAM_FACTORY_H

#include <GLES2/gl2.h>

#define INVALID_PROGRAM 0

namespace fezrestia {

class ShaderProgramFactory {

public:
    // Shader program target.
    enum ShaderType {
        ShaderType_SINGLE_COLOR,
        ShaderType_YUV,
        ShaderType_SURFACE_TEXTURE,

        ShaderType_MAX
    };

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
    GLuint CreateShaderProgram(
            ShaderType type,
            const char* vertexShaderCode,
            size_t vertexShaderCodeLen,
            const char* fragmentShaderCode,
            size_t fragmentShaderCodeLen);

private:

    // FIELDS ////////////////////////////////////////////////////////////////////////////////////

    GLuint mShaderPrograms[ShaderType_MAX];

    // FUNCTIONS /////////////////////////////////////////////////////////////////////////////////

    GLuint createProgram(
            GLchar* vertexSource,
            GLsizei vertexSourceLength,
            GLchar* fragmentSource,
            GLsizei fragmentSourceLength);

    GLuint loadShader(
            GLenum shaderType,
            const GLchar* source,
            GLsizei soruceLength);

    GLuint destroyProgram(GLuint program);

};

}; // namespace fezrestia

#endif // SHADER_PROGRAM_FACTORY_H
