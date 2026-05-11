#version 460

layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec3 VertexVelocity;
layout(location = 2) in float VertexAge;

out vec3 Position;
out vec3 Velocity;
out float Age;

out vec2 TexCoord;
out float Transp;

uniform mat4 MV;
uniform mat4 Proj;

uniform float Time;
uniform float DeltaT;
uniform float ParticleLifetime;
uniform vec3 Accel;
uniform vec3 Emitter;

uniform sampler1D RandomTex;

uniform float ParticleSize;
uniform int Pass;

float rand(float seed)
{
    return texture(RandomTex, fract(seed)).x;
}

vec3 FairyDirection(float id)
{
    vec3 direction = vec3(
        -0.12 + rand(id * 0.071 + 0.13) * 0.24,  // slight left/right
        -0.65 + rand(id * 0.113 + 0.37) * 0.25,  // mostly downward
        -0.12 + rand(id * 0.191 + 0.59) * 0.24   // slight forward/back
    );

    if (length(direction) < 0.001)
    {
        direction = vec3(0.0, -1.0, 0.0);
    }

    return normalize(direction);
}

void update()
{
    float dt = clamp(DeltaT, 0.0, 0.033);

    Age = VertexAge + dt;

    if (Age < 0.0)
    {
        Position = VertexPosition;
        Velocity = VertexVelocity;
        return;
    }

    if (Age > ParticleLifetime)
    {
        float id = float(gl_VertexID);

        vec3 direction = FairyDirection(id);

        float speed = 0.5 + rand(id * 0.271 + 0.83) * 0.15;

        vec3 startOffset = vec3(
            -0.45 + rand(id * 0.331 + 0.11) * 0.90,
            -0.05 + rand(id * 0.443 + 0.22) * 0.10,
            -0.45 + rand(id * 0.557 + 0.33) * 0.90
        );

Position = Emitter + startOffset;
        Velocity = direction * speed;

        Age = 0.0;
    }
    else
    {
        Position = VertexPosition + VertexVelocity * dt;
        Velocity = VertexVelocity + Accel * dt;
    }
}

void render()
{
    float agePercent = clamp(VertexAge / ParticleLifetime, 0.0, 1.0);

    float lifeRatio = 1.0 - agePercent;

    float twinkle =
        0.5 + 0.5 * sin(Time * 18.0 + float(gl_InstanceID) * 2.4);

    float sizeVariation =
        0.75 + rand(float(gl_InstanceID) * 0.417 + 0.24) * 0.75;

    Transp = 0.75 * lifeRatio;

    vec4 eyePos = MV * vec4(VertexPosition, 1.0);

    float size =
        ParticleSize
        * sizeVariation
        * (0.6 + 0.8 * twinkle)
        * lifeRatio;

    vec2 offsets[6] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2( 1.0,  1.0),

        vec2(-1.0, -1.0),
        vec2( 1.0,  1.0),
        vec2(-1.0,  1.0)
    );

    vec2 texCoords[6] = vec2[](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0),

        vec2(0.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 1.0)
    );

    eyePos.xy += offsets[gl_VertexID] * size;

    TexCoord = texCoords[gl_VertexID];

    gl_Position = Proj * eyePos;
}

void main()
{
    if (Pass == 1)
    {
        update();
    }
    else
    {
        render();
    }
}