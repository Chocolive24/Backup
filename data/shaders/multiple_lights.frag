#version 300 es
precision highp float;

in vec3 fragPos;
in vec3 normal;
in vec2 texCoords;

layout(location = 0) out vec4 outColor;

struct Material{
  sampler2D diffuse_map;
  sampler2D specular_map;
  float shininess;
};

struct DirectionalLight{
  vec3 direction;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

struct PointLight{
  vec3 position;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

  float constant;
  float linear;
  float quadratic;
};

struct SpotLight{
  vec3 position;
  vec3 direction;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

  float constant;
  float linear;
  float quadratic;

  float cutoff;
  float outer_cutoff;
};

uniform Material material;

uniform DirectionalLight dir_light;

#define NR_POINT_LIGHTS 2
uniform PointLight point_lights[NR_POINT_LIGHTS];

uniform SpotLight spot_light;

uniform vec3 viewPos;

vec3 CalcDirLight(DirectionalLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);

    // Diffuse shading.
    float diff = max(dot(normal, lightDir), 0.0);

    // Specular shading.
    vec3 halfWayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfWayDir), 0.0), material.shininess);

    // Combine results.
    vec3 ambient  = light.ambient  * vec3(texture(material.diffuse_map, texCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse_map, texCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular_map, texCoords));
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 halfWayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfWayDir), 0.0), material.shininess);

    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
  			            light.quadratic * (distance * distance));

    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(material.diffuse_map, texCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse_map, texCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular_map, texCoords));
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 halfWayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfWayDir), 0.0), material.shininess);

    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutoff - light.outer_cutoff;
    float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.0, 1.0);

    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse_map, texCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse_map, texCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular_map, texCoords));

    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;

    return (ambient + diffuse + specular);
}

void main()
{
    // properties
    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(viewPos - fragPos);

    // phase 1: Directional lighting
    vec3 result = CalcDirLight(dir_light, norm, viewDir);
    // phase 2: Point lights
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(point_lights[i], norm, fragPos, viewDir);
    // phase 3: Spot light
    result += CalcSpotLight(spot_light, norm, fragPos, viewDir);

    outColor = vec4(result, 1.0);
}