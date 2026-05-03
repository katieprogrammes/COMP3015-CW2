#version 460

in vec3 Position;
in vec3 Normal;

uniform sampler2D DiffuseTex;
in vec2 TexCoord;

layout (location = 0) out vec4 FragColor;

uniform struct LightInfo{
    vec4 Position;
    vec3 La;
    vec3 L;
} Light;

uniform struct MaterialInfo{
    vec3 Kd;
    vec3 Ka;
    vec3 Ks;
    float Shininess;
} Material;

uniform struct FogInfo{
    float MaxDist;
    float MinDist;
    vec3 Color;
}Fog;

vec3 blinnPhong(vec3 position, vec3 n){
    vec3 diffuse=vec3(0), spec=vec3(0);
    vec3 ambient=Light.La*Material.Ka;
    vec3 s=normalize(Light.Position.xyz-position);
    float sDotN=max(dot(s,n),0.0);
    diffuse=Material.Kd*sDotN;
    if (sDotN>0.0){
        vec3 v=normalize(-position.xyz);
        vec3 h=normalize(v+s);
        spec=Material.Ks*pow(max(dot(h,n),0.0),Material.Shininess);
    }
    return ambient+(diffuse+spec)*Light.L;
}

void main() {
    float dist=abs(Position.z);
    float fogFactor=(Fog.MaxDist-dist)/(Fog.MaxDist-Fog.MinDist);
    fogFactor=clamp(fogFactor,0.0,1.0);
    vec2 uv = clamp(TexCoord, 0.001, 0.999);
    vec3 texColor = texture(DiffuseTex, uv).rgb;
    vec3 shadeColor = texColor * blinnPhong(Position, normalize(Normal));
    float rim = 1.0 - max(dot(normalize(Normal), normalize(-Position)), 0.0);
    vec3 rimColor = vec3(1.0) * pow(rim, 3.0);
    shadeColor += rimColor * 0.25;     
    vec3 color=mix(Fog.Color,shadeColor,fogFactor);
    FragColor = vec4(color, 1.0);
}
