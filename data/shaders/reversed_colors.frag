#version 300 es
precision highp float;

out vec4 outColor;
  
in vec2 texCoords;

uniform sampler2D screenTexture;

void main()
{ 
    outColor = vec4(vec3(1.0 - texture(screenTexture, texCoords)), 1.0);
}
