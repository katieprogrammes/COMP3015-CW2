#version 460

in vec2 TexCoord;
in float Transp;

layout(location = 0) out vec4 FragColor;

void main()
{
    vec2 p = TexCoord * 2.0 - 1.0;
    float d = length(p);

    float glow = smoothstep(1.0, 0.0, d);
    float core = smoothstep(0.35, 0.0, d);

    float alpha = (glow * 0.35 + core * 0.75) * Transp;

    if (alpha < 0.01)
    {
        discard;
    }

    vec3 fairyBlue = vec3(0.0, 0.067, 1.0);
    vec3 fairyWhite = vec3(0.95, 1.0, 1.0);

    vec3 colour = mix(fairyBlue, fairyWhite, core);

    FragColor = vec4(colour, alpha);
}