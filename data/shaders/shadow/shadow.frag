#version 300 es
precision highp float;

out vec4 fragColor;

in vec3 fragPos;
in vec3 normal;
in vec2 texCoords;
in vec4 fragPosLightSpace;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightDir;
uniform vec3 viewPos;

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

    vec3 lightDir = -lightDir; //normalize(lightPos - fragPos);
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
    vec3 color = texture(diffuseTexture, texCoords).rgb;
    vec3 normal = normalize(normal);
    vec3 lightColor = vec3(1.0);

    // ambient
    vec3 ambient = 0.15 * lightColor;

    // diffuse
    vec3 lightDir = -lightDir; //normalize(lightPos - fragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;

    // specular
    vec3 viewDir = normalize(viewPos - fragPos);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;    

    // calculate shadow
    float shadow = ShadowCalculation(fragPosLightSpace);       
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;    
    
    fragColor = vec4(lighting, 1.0);
}