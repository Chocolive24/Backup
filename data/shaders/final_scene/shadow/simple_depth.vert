#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;

struct Transform {
    mat4 model;
    // mat4 view;
    // mat4 projection;
};

uniform mat4 lightSpaceMatrix;
uniform Transform transform;

void main()
{
    gl_Position = lightSpaceMatrix * transform.model * vec4(aPos, 1.0);
}  
