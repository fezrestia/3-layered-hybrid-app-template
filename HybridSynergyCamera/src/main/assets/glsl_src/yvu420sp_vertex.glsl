attribute vec3 aVertex;
attribute vec2 aTexCoord;

uniform mat4 uMvpMatrix;

varying vec2 vTexCoordHandler;

void main()
{
    gl_Position = vec4(aVertex, 1.0);
    gl_Position = uMvpMatrix * gl_Position;

    vTexCoordHandler = aTexCoord;
}
