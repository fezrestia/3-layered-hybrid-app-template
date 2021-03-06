precision highp float;

varying vec2 vTexCoordHandler;

uniform sampler2D uTextureY;
uniform sampler2D uTextureVu;

uniform float uAlpha;

const mat3 yuv2rgb = mat3(
        1.000,  0.000,  1.402,
        1.000, -0.344, -0.714,
        1.000,  1.772,  0.000
);

void main()
{
    vec3 yuv = vec3(
            texture2D(uTextureY, vTexCoordHandler).r,
            texture2D(uTextureVu, vTexCoordHandler).a - 0.5,
            texture2D(uTextureVu, vTexCoordHandler).r - 0.5
    );

    vec3 rgb = yuv * yuv2rgb;

    vec4 color = vec4(rgb, uAlpha);

    color = clamp(color, 0.0, 1.0);

    gl_FragColor = color;
}
