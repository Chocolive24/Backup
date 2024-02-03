#version 300 es
precision highp float;

out vec4 fragColor;

// Common inputs.
in vec3 fragPos;
in vec3 normal;
in vec2 texCoords;

// normal mapping inputs.
in vec3 tangentLightDir;
in vec3 tangentViewPos;
in vec3 tangentFragPos;

// shadow inputs.
in vec4 fragPosLightSpace[3];
in float clipSpacePosZ;

struct Material{
  sampler2D texture_diffuse1;
  sampler2D texture_specular1;
  sampler2D texture_normal1;
  float shininess;
};

uniform Material material;

// Need a struct to be able to create an array of sampler2D...
struct ShadowMap {
    sampler2D data;
};

uniform ShadowMap shadowMap[3];

uniform float maxNearCascade;
uniform float maxMiddleCascade;
uniform float farPlane;

uniform bool debug_color;

float ShadowCalculation(int cascaded_idx)
{
    // Perform perspective divide (return value in range [-1, 1]).
    // Useless for orthographic projection but obligatory for persepctive projection.
    vec3 projCoords = fragPosLightSpace[cascaded_idx].xyz / fragPosLightSpace[cascaded_idx].w;

    // Transform the NDC coordinates to the range [0,1] to get a position in the depth map.
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) {
        return 0.0;
    }
        
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap[cascaded_idx].data, projCoords.xy).r;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;  

    vec3 lightDir = -tangentLightDir; //normalize(worldlightPos - fragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // check whether current frag pos is in shadow
    //float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

    // PCF
    float shadow = 0.0;
    vec2 texelSize = vec2(1.0) / vec2(textureSize(shadowMap[cascaded_idx].data, 0));

    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap[cascaded_idx].data, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }

    shadow /= 9.0;

    return shadow;
}

void main()
{           
    // Normal
    vec3 normal = texture(material.texture_normal1, texCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0); //[0,1] -> [-1,1]

    // Texture color.
    vec3 tex_diffuse = texture(material.texture_diffuse1, texCoords).rgb;
    vec3 tex_specular = texture(material.texture_specular1, texCoords).rgb;

    // ambient
    vec3 ambient = tex_diffuse * 0.15;

    // diffuse
    vec3 lightDir = -tangentLightDir; // normalize(tangentLightPos - tangentFragPos); // direction to the light.

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = tex_diffuse * diff;

    // specluar
    vec3 viewDir = normalize(tangentViewPos - tangentFragPos);
    vec3 halfWayDir = normalize(lightDir + viewDir);

    float spec = pow(max(dot(normal, halfWayDir), 0.0), 64.0);
    vec3 specular = tex_specular * spec;    

    // calculate shadow
    float cascadeFarPlanes[3] = float[3](
        maxNearCascade,
        maxMiddleCascade,
        farPlane
    );

    vec3 debug_col = vec3(0.0);
    float shadow = 0.0;
    for (int i = 0; i < 3; i++) {
        if (clipSpacePosZ <= cascadeFarPlanes[i]) {
            shadow = ShadowCalculation(i);

            if (!debug_color) {
              break;
            }

            if (i == 0) {
              debug_col = vec3(1.0, 0.0, 0.0);
            }
            else if (i == 1) {
              debug_col = vec3(0.0, 1.0, 0.0);
            }
            else if (i == 2) {
              debug_col = vec3(0.0, 0.0, 1.0);
            }

            break;
        }
    }
    
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular) + debug_col);    
    
    fragColor = vec4(lighting, 1.0);
}