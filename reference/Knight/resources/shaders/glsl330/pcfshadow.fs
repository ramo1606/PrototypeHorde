#version 330

in vec3 fragPos;
in vec4 fragPosLightSpace;

uniform sampler2D shadowMap;
uniform vec3 lightPos;
uniform float shadowBias;

out vec4 color;

// Helper function to compute shadow factor with PCF
float PCFShadow(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float shadow = 0.0;
    float texelSize = 1.0 / 1024.0;  // Size of a single texel in shadow map

    // Sample 3x3 grid around the current pixel
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z - shadowBias > pcfDepth ? 1.0 : 0.0;
        }
    }

    shadow /= 9.0;  // Average shadow factor
    return shadow;
}

void main() {
    // Ambient and diffuse light calculations
    vec3 colorAmbient = vec3(0.3);
    vec3 colorDiffuse = vec3(0.7);

    vec3 normal = normalize(fragPos);
    vec3 lightDir = normalize(lightPos - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * colorDiffuse;

    // Calculate soft shadow factor using PCF
    float shadow = PCFShadow(fragPosLightSpace);

    // Final color with ambient, diffuse, and shadow
    //vec3 lighting = (colorAmbient + (1.0 - shadow) * diffuse) * colorDiffuse;
    //color = vec4(lighting, 1.0);
    color = vec4(vec3(shadow, shadow, shadow),1);
}
