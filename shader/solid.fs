#version 460

#define PI 3.14159265

layout(location = 0) out vec4 FragColor;

uniform vec4 Color;

layout(binding = 2) uniform sampler2D NoiseTex;
uniform int UseFairyNoise = 0;
uniform int FairyType = 0;
uniform float Time = 0.0;
uniform float NoiseSeed = 0.0;

in vec2 TexCoord;

void main()
{
    vec4 finalColor = Color;

    if (UseFairyNoise == 1)
    {
        vec2 movingCoord = TexCoord + vec2(Time * 0.04 + NoiseSeed, Time * 0.025);
        //vec2 movingCoord = TexCoord + vec2(Time * 0.12 + NoiseSeed, Time * 0.08); //more intense option

        vec4 noise = texture(NoiseTex, movingCoord);

        float t = (cos(noise.g * PI) + 1.0) / 2.0;

        //Evil Fairy
        if (FairyType == 2)
    {
        vec3 evilBlack = vec3(0.0, 0.0, 0.0);
        vec3 evilRed = vec3(1.0, 0.02, 0.04);
        vec3 evilPurple = vec3(0.28, 0.0, 0.16);

        vec3 corrupted = mix(evilBlack, evilPurple, t);
        corrupted = mix(corrupted, evilRed, smoothstep(0.45, 1.0, t));

        finalColor.rgb = corrupted;
        finalColor.rgb *= 0.75 + t * 0.65;
    }

        else
    {
        //Good Fairy
        vec3 fairyDeepBlue = vec3(0.02, 0.32, 0.85);
        vec3 fairyCyan = vec3(0.05, 0.95, 1.0);

        finalColor.rgb = mix(fairyDeepBlue, fairyCyan, t);
        finalColor.rgb *= 0.65 + t * 0.55;
    }
}

    FragColor = finalColor;
}