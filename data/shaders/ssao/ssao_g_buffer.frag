#version 300 es
precision highp float;

layout (location = 0) out vec3 gPosition; // pas obligatoire car on peut chopper la pos depuis la depth map.
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 fragPos;
in vec3 normal;
in vec2 texCoords;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
};

uniform Material material;

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = fragPos;

    // also store the per-fragment normals into the gbuffer
    gNormal = normalize(normal);

    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = vec3(0.95); // hard coded for the ssao exemple.
}