#version 300 es
precision highp float;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 brightColor; // second color buffer target

in vec3 fragPos;
in vec3 normal;
in vec2 texCoords;

struct Light {
    vec3 Position;
    vec3 Color;
};

uniform Light lights[4];
uniform sampler2D diffuseTexture;
uniform vec3 viewPos;

void main()
{           
    vec3 color = texture(diffuseTexture, texCoords).rgb;
    vec3 normal = normalize(normal);

    // ambient
    vec3 ambient = 0.0 * color;

    // lighting
    vec3 lighting = vec3(0.0);
    vec3 viewDir = normalize(viewPos - fragPos);

    for(int i = 0; i < 4; i++)
    {
        // diffuse
        vec3 lightDir = normalize(lights[i].Position - fragPos);
        float diff = max(dot(lightDir, normal), 0.0);
        vec3 result = lights[i].Color * diff * color;  
        
        // attenuation (use quadratic as we have gamma correction)
        float distance = length(fragPos - lights[i].Position);
        result *= 1.0 / (distance * distance);
        lighting += result;
                
    }
    vec3 result = ambient + lighting;

    // check whether result is higher than some threshold, if so, output as bloom threshold color
    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));

    if(brightness > 1.0)
        brightColor = vec4(result, 1.0);
    else
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);

    fragColor = vec4(result, 1.0);
}