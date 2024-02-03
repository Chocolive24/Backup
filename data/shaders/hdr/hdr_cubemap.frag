#version 300 es
precision highp float;

layout (location = 0) out vec4 fragColor;

in vec3 localFragPos;

uniform samplerCube environmentMap;

void main()
{
    const float gamma = 2.2;
    vec3 envColor = textureLod(environmentMap, localFragPos, 0.0).rgb;

    // Narkowicz ACES tone mapping
    envColor *= 0.6;
    vec3 mapped = (envColor * (2.51f * envColor + 0.03f)) /
                  (envColor * (2.43f * envColor + 0.59f) + 0.14f);
    mapped = clamp(mapped, vec3(0.0), vec3(1.0));

    // gamma correction
    mapped = pow(mapped, vec3(1.0 / gamma));
    fragColor = vec4(mapped, 1.0);
}