#version 460 core

in vec2 TexCoord;

layout(location = 0) out vec4 FragColor;

uniform sampler2D SceneTex;

void main()
{
    vec3 color = texture(SceneTex, TexCoord).rgb;
    FragColor = vec4(color, 1.0);
}