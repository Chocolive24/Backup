#version 300 es
precision highp float;

out vec4 outColor;
  
in vec2 texCoords;

uniform sampler2D screenTexture;

void main()
{ 
    outColor = texture(screenTexture, texCoords);
    // Humans eyes tends to be more sensitive to green and least to blue so the everage
    // is calculated like this for a more physically accurate result.
    float average = 0.2126 * outColor.r + 0.7152 * outColor.g + 0.0722 * outColor.b;
    outColor = vec4(average, average, average, 1.0);
}
