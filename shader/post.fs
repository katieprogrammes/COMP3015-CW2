#version 460 core

in vec2 TexCoord;

layout(location = 0) out vec4 FragColor;

uniform sampler2D SceneTex;
uniform sampler2D BloomTex;

uniform float BloomStrength = 0.35;

void main()
{
    vec3 sceneColor = texture(SceneTex, TexCoord).rgb;
    vec3 bloomColor = texture(BloomTex, TexCoord).rgb;

    vec3 finalColor = sceneColor + bloomColor * BloomStrength;

    FragColor = vec4(finalColor, 1.0);
}