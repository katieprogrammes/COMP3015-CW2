#version 460

#define PI 3.14159265

layout(location = 0) out vec4 FragColor;

uniform vec4 Color;

layout(binding = 2) uniform sampler2D NoiseTex;
uniform int UseFairyNoise = 0;
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

        vec3 fairyBlue = vec3(0.25, 0.75, 1.0);
        vec3 fairyWhite = vec3(0.85, 0.98, 1.0);

        finalColor.rgb = mix(fairyBlue, fairyWhite, t);
        finalColor.rgb *= 0.85 + t * 0.45;
        //finalColor.rgb *= 0.65 + t * 0.85; //more intense option
    }

    FragColor = finalColor;
}