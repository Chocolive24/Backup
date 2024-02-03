#version 300 es
precision highp float;

layout(location = 0) out vec4 fragColor;

in vec3 fragPos;
in vec3 normal;
in vec2 texCoords;
  
struct Material {
  vec3  albedo;
  float metallic;
  float roughness;
  float ao;
};

const float PI = 3.14159265359;

uniform vec3 viewPos;

uniform Material material;

const int kLightNbr = 4;
uniform vec3 lightPositions[kLightNbr];
uniform vec3 lightColors[kLightNbr];

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

// F0 is the surface reflection at zero incidence or how much the surface reflects 
// if looking directly at the surface.
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    // The clamp prevents from black spots.
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(normal); 
    vec3 V = normalize(viewPos - fragPos);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, material.albedo, material.metallic);

    vec3 Lo = vec3(0.0);
    for(int i = 0; i < kLightNbr; ++i) 
    {
        vec3 L = normalize(lightPositions[i] - fragPos);
        vec3 H = normalize(V + L);
  
        float distance    = length(lightPositions[i] - fragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = lightColors[i] * attenuation; 

        float NDF = DistributionGGX(N, H, material.roughness);       
        float G   = GeometrySmith(N, V, L, material.roughness); 
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        // Calculate the Cook-Torrance BRDF.
        vec3 numerator    = NDF * G * F;
        // 0.0001 avoid to divide by 0 if the dot is equal to 0.
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0)  + 0.0001;
        vec3 specular     = numerator / denominator; 

        // kS is equal to Fresnel
        vec3 kS = F;

        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - material.metallic;

        // Calculate outgoing radiance.
        float NdotL = max(dot(N, L), 0.0);        
        Lo += (kD * material.albedo / PI + specular) * radiance * NdotL;
    }

    // ambient lighting (note that the next IBL will replace 
    // this ambient lighting with environment lighting).
    vec3 ambient = vec3(0.03) * material.albedo * material.ao;

    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    fragColor = vec4(color, 1.0);
}