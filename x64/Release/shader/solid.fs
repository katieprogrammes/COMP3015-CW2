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
        vec3 fairyBlue = vec3(0.00, 0.08, 0.55);
        vec3 fairyCyan = vec3(0.05, 1.0, 1.0);

        float shimmer = smoothstep(0.25, 0.95, t);

        finalColor.rgb = mix(fairyBlue, fairyCyan, shimmer);
        finalColor.rgb *= 0.75 + shimmer * 0.85;
    }
}

    FragColor = finalColor;
}