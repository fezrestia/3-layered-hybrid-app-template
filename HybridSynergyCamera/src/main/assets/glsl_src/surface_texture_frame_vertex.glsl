attribute vec4 aVertex;
attribute vec4 aTexCoord;

uniform mat4 uMvpMatrix;
uniform mat4 uOesTexMatrix;

varying vec2 vTexCoordHandler;

void main()
{
    gl_Position = uMvpMatrix * aVertex;

    vec4 totalMatrix = uOesTexMatrix * aTexCoord;
    vTexCoordHandler = totalMatrix.xy;
}
