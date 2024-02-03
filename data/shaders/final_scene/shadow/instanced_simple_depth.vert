#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;
// location 1 to 4 are already taken by other vertex inputs.
layout (location = 5) in mat4 aModelMatrix;

struct Transform {
    mat4 model;
    // mat4 view;
    // mat4 projection;
};

uniform mat4 lightSpaceMatrix;
//uniform Transform transform;

void main()
{
    gl_Position = lightSpaceMatrix * aModelMatrix * vec4(aPos, 1.0);
}  
