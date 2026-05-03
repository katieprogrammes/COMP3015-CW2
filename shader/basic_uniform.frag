#version 460

in vec3 Position;
in vec3 Normal;
in vec4 ShadowCoord;
in vec2 TexCoord;

layout (location = 0) out vec4 FragColor;

uniform sampler2D DiffuseTex;
uniform sampler2DShadow ShadowMap;

uniform int UseTexture;

uniform struct LightInfo{
    vec4 Position;
    vec3 La;
    vec3 L;
    vec3 Intensity;
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

subroutine void RenderPassType();
subroutine uniform RenderPassType RenderPass;

subroutine (RenderPassType)
void shadeWithShadow()
{

    //Base colour
    vec3 baseColor;
    if (UseTexture == 1) {
        vec2 uv = clamp(TexCoord, 0.001, 0.999);
        baseColor = texture(DiffuseTex, uv).rgb;
    } else {
        baseColor = Material.Kd;
    }
    vec3 lit = baseColor * blinnPhong(Position, normalize(Normal));

    //Shadow
    float shadow = 1.0;
	if( ShadowCoord.z >= 0 ) {
		shadow = textureProj(ShadowMap, ShadowCoord);
	}

    shadow = pow(shadow, 3.0); //Makes shadow stronger

    //Applying shadow to relevant areas
    vec3 ambient = baseColor * Light.La * Material.Ka;
    vec3 nonAmbient = lit - ambient;
    lit = ambient + nonAmbient * shadow;

    //Rim lighting
    float rim = 1.0 - max(dot(normalize(Normal), normalize(-Position)), 0.0);
    lit += vec3(1.0) * pow(rim, 3.0) * 0.1;

    //Fog
    float dist = abs(Position.z);
    float fogFactor = clamp((Fog.MaxDist - dist) / (Fog.MaxDist - Fog.MinDist), 0.0, 1.0);
    vec3 finalColor = mix(Fog.Color, lit, fogFactor);

    FragColor = vec4(finalColor, 1.0);
    //FragColor = vec4(lit, 1.0);
}

subroutine (RenderPassType)
void recordDepth()
{
	// Do nothing, depth will be written automatically
}

void main() {
    RenderPass();
}
