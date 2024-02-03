#version 300 es
precision highp float;

out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D image;

uniform bool horizontal;
const float weight[5] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

void main()
{             
     vec2 tex_offset;
     ivec2 texture_size = textureSize(image, 0);
     tex_offset.x = 1.0 / float(texture_size.x);
     tex_offset.y = 1.0 / float(texture_size.y);

     vec3 result = texture(image, texCoords).rgb * weight[0];
     if(horizontal)
     {
         for(int i = 1; i < 5; ++i)
         {
            result += texture(image, texCoords + vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
            result += texture(image, texCoords - vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
         }
     }
     else
     {
         for(int i = 1; i < 5; ++i)
         {
             result += texture(image, texCoords + vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
             result += texture(image, texCoords - vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
         }
     }

     fragColor = vec4(result, 1.0);
}