#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

// Common outputs.
out vec3 fragPos;
out vec3 normal;
out vec2 texCoords;

// Normal mapping outputs.
out vec3 tangentLightDir;
out vec3 tangentViewPos;
out vec3 tangentFragPos;

// Shadow calculation outputs.
out vec4 fragPosLightSpace[3];
out float clipSpacePosZ;

struct Transform{
  mat4 model;
  mat4 view;
  mat4 projection;
};

// Common uniforms.
uniform Transform transform;
uniform mat4 normalMatrix;

// Normal mapping uniforms.
uniform vec3 lightDir;
uniform vec3 viewPos;

// Shadow uniforms.
uniform mat4 lightSpaceMatrix[3];

void main()
{    
    // Common outputs.
    vec4 worldPos = transform.model * vec4(aPos, 1.0);
    fragPos = vec3(worldPos);
    normal = mat3(normalMatrix) * aNormal;
    texCoords = aTexCoords;
    
    gl_Position = transform.projection * transform.view * worldPos;

    // normal mapping outputs.
    mat3 normalMatrix = mat3(normalMatrix);
    vec3 T = normalize(normalMatrix * normalize(aTangent));
    vec3 N = normalize(normalMatrix * normalize(aNormal));
    vec3 B = normalize(normalMatrix * normalize(aBitangent));

    mat3 TBN = transpose(mat3(T, B, N)); //inverting

    //TBN translates from world space to tangent space
    tangentLightDir = TBN * lightDir;
    tangentViewPos  = TBN * viewPos;
    tangentFragPos  = TBN * fragPos;

    // shadow outputs.
    for (int i = 0; i < 3; i++) {
      fragPosLightSpace[i] = lightSpaceMatrix[i] * worldPos;
    }
    
    clipSpacePosZ = gl_Position.z;
}
