#version 460 core

in vec2 TexCoord;

layout(location = 0) out vec4 FragColor;

uniform sampler2D SceneTex;
uniform float Threshold;

float luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

void main()
{
    vec3 color = texture(SceneTex, TexCoord).rgb;

    float brightness = luminance(color);

    //Normal fairy colours
    bool blueFairy =
        color.b > color.r * 1.25 &&
        color.g > color.r * 1.10;

    //Corrupted fairy colours
    bool redFairy =
        color.r > color.g * 1.5 &&
        color.r > color.b * 1.1;

    bool fairyColour = blueFairy || redFairy;

    if (brightness > Threshold && fairyColour)
    {
        FragColor = vec4(color, 1.0);
    }
    else
    {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}