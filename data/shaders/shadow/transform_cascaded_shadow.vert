#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 fragPos;
out vec3 normal;
out vec2 texCoords;
out vec4 fragPosLightSpace[3];
out float clipSpacePosZ;

struct Transform{
  mat4 model;
  mat4 view;
  mat4 projection;
};

uniform Transform transform;
uniform mat4 normalMatrix;
uniform mat4 lightSpaceMatrix[3];

void main()
{    
    vec4 worldPos = transform.model * vec4(aPos, 1.0);
    fragPos = vec3(worldPos);
    texCoords = aTexCoords;
    normal = mat3(normalMatrix) * aNormal;

    // shadow outputs.
    for (int i = 0; i < 3; i++) {
      fragPosLightSpace[i] = lightSpaceMatrix[i] * worldPos;
    }
    
    gl_Position = transform.projection * transform.view * worldPos;

    clipSpacePosZ = gl_Position.z;
}