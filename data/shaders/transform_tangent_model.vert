#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out vec2 texCoords;
out vec3 tangentLightPos;
out vec3 tangentViewPos;
out vec3 tangentFragPos;

struct Transform{
  mat4 model;
  mat4 view;
  mat4 projection;
};

uniform Transform transform;
uniform mat4 normalMatrix;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    vec4 worldPos = transform.model * vec4(aPos, 1.0);
    vec3 fragPos = vec3(worldPos);
    texCoords = aTexCoords;

    mat3 normalMatrix = mat3(normalMatrix);
    vec3 T = normalize(normalMatrix * normalize(aTangent));
    vec3 N = normalize(normalMatrix * normalize(aNormal));
    vec3 B = normalize(normalMatrix * normalize(aBitangent));

    mat3 TBN = transpose(mat3(T, B, N)); //inverting

    //TBN translates from world space to tangent space
    tangentLightPos = TBN * lightPos;
    tangentViewPos  = TBN * viewPos;
    tangentFragPos  = TBN * fragPos;

    gl_Position = transform.projection * transform.view * worldPos;

}
