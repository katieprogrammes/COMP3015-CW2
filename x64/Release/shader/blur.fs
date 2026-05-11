#version 460 core

in vec2 TexCoord;

layout(location = 0) out vec4 FragColor;

uniform sampler2D ImageTex;
uniform int Horizontal;

void main()
{
    vec2 texOffset = 1.0 / textureSize(ImageTex, 0);

    float weights[5] = float[](
        0.227027,
        0.1945946,
        0.1216216,
        0.054054,
        0.016216
    );

    vec3 result = texture(ImageTex, TexCoord).rgb * weights[0];

    for (int i = 1; i < 5; i++)
    {
        if (Horizontal == 1)
        {
            result += texture(ImageTex, TexCoord + vec2(texOffset.x * i, 0.0)).rgb * weights[i];
            result += texture(ImageTex, TexCoord - vec2(texOffset.x * i, 0.0)).rgb * weights[i];
        }
        else
        {
            result += texture(ImageTex, TexCoord + vec2(0.0, texOffset.y * i)).rgb * weights[i];
            result += texture(ImageTex, TexCoord - vec2(0.0, texOffset.y * i)).rgb * weights[i];
        }
    }

    FragColor = vec4(result, 1.0);
}