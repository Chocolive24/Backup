#version 300 es
precision highp float;

in vec2 texCoords;

out vec4 FragColor;

struct Material{
  sampler2D texture_diffuse1;
  sampler2D texture_specular1;
};


uniform Material material;

void main()
{    
    FragColor = texture(material.texture_diffuse1, texCoords);
}