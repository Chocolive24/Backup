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
in vec4 fragPosLightSpace;

struct Material{
  sampler2D texture_diffuse1;
  sampler2D texture_specular1;
  sampler2D texture_normal1;
  float shininess;
};

uniform Material material;

uniform sampler2D shadowMap;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // Perform perspective divide (return value in range [-1, 1]).
    // Useless for orthographic projection but obligatory for persepctive projection.
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Transform the NDC coordinates to the range [0,1] to get a position in the depth map.
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) {
        return 0.0;
    }
        
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;  

    vec3 lightDir = -tangentLightDir; //normalize(worldlightPos - fragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // check whether current frag pos is in shadow
    //float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

    // PCF
    float shadow = 0.0;
    vec2 texelSize = vec2(1.0) / vec2(textureSize(shadowMap, 0));

    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
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
    float shadow = ShadowCalculation(fragPosLightSpace);       
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular));    
    
    fragColor = vec4(lighting, 1.0);
}