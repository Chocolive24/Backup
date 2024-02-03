#version 300 es
precision highp float;

out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D hdrBuffer;
uniform bool hdr;
uniform float exposure;

void main()
{             
    const float gamma = 2.2;
    vec3 hdrColor = texture(hdrBuffer, texCoords).rgb;
    if(hdr)
    {
        // reinhard
        // vec3 result = hdrColor / (hdrColor + vec3(1.0));
        // exposure
        //vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
        // also gamma correct while we're at it       
        //result = pow(result, vec3(1.0 / gamma));
        //fragColor = vec4(result, 1.0);

        // Narkowicz ACES tone mapping
        hdrColor *= 0.6;
        vec3 mapped = (hdrColor * (2.51f * hdrColor + 0.03f)) /
                      (hdrColor * (2.43f * hdrColor + 0.59f) + 0.14f);
        mapped = clamp(mapped, vec3(0.0), vec3(1.0));

        // gamma correction
        mapped = pow(mapped, vec3(1.0 / gamma));
        fragColor = vec4(mapped, 1.0);
    }
    else
    {
        vec3 result = pow(hdrColor, vec3(1.0 / gamma));
        fragColor = vec4(result, 1.0);
    }
}