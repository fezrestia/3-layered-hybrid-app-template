#extension GL_OES_EGL_image_external : require

precision mediump float;

uniform samplerExternalOES uTextureOes;

varying vec2 vTexCoordHandler;

uniform float uAlpha;

void main()
{
    vec4 color = texture2D(uTextureOes, vTexCoordHandler);

    color.a = uAlpha;

    gl_FragColor = color;
}
