#version 300 es
precision highp float;

layout (location = 0) out vec4 fragColor;
  
in vec2 texCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

struct Light {
    vec3 position;
    vec3 color;

    float linear;
    float quadratic;
    float radius;
};

const int NR_LIGHTS = 32;
uniform Light lights[NR_LIGHTS];
uniform vec3 viewPos;

void main()
{             
    // retrieve data from gbuffer
    vec3 fragPos = texture(gPosition, texCoords).rgb;
    vec3 normal = texture(gNormal, texCoords).rgb;
    vec3 diffuse = texture(gAlbedoSpec, texCoords).rgb;
    float specular = texture(gAlbedoSpec, texCoords).a;
    
    // then calculate lighting as usual
    vec3 lighting  = diffuse * 0.1; // hard-coded ambient component
    vec3 viewDir  = normalize(viewPos - fragPos);

    for(int i = 0; i < NR_LIGHTS; ++i)
    {
        // calculate distance between light source and current fragment
        float distance = length(lights[i].position - fragPos);
        if(distance < lights[i].radius)
        {
            // diffuse
            vec3 lightDir = normalize(lights[i].position - fragPos);
            vec3 diffuse = max(dot(normal, lightDir), 0.0) * diffuse * lights[i].color;

            // specular
            vec3 halfwayDir = normalize(lightDir + viewDir);  
            float spec = pow(max(dot(normal, halfwayDir), 0.0), 16.0);
            vec3 specular = lights[i].color * spec * specular;
            // attenuation

            float attenuation = 1.0 / (1.0 + lights[i].linear * distance + 
                                lights[i].quadratic * distance * distance);
            diffuse *= attenuation;
            specular *= attenuation;
            lighting += diffuse + specular;
        }
    }    

    fragColor = vec4(lighting, 1.0);
}  