#version 330

// Input from vertex shader
in vec3 fragPosition;
in vec3 fragNormal;

// Uniforms
uniform vec3 lightPosition; // Position of the point light in world space
uniform vec3 lightColor;    // Color of the point light
uniform vec4 colDiffuse;     // Base color of the object

// Output color
out vec4 fragColor;

void main() {
    // Calculate direction from the fragment to the light source
    vec3 lightDir = normalize(lightPosition - fragPosition);

    // Calculate diffuse shading (Lambertian reflection)
    float diff = max(dot(fragNormal, lightDir), 0.0);

    // Calculate attenuation (inverse square law)
    float distance = length(lightPosition - fragPosition);
    float attenuation = 1.0 / (0.5 * distance + 0.25f * (distance * distance));

    // Combine light color with attenuation and diffuse component
    vec3 lighting = lightColor * diff ;

    // Apply lighting to the base color
    fragColor = vec4(colDiffuse.rgb * lighting, colDiffuse.a);
}
