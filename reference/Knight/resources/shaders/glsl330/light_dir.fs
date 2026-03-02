#version 330

in vec3 worldNormal;      // Normal vector from the vertex shader
in vec2 texCoord;
in vec4 vcolor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 lightDirection; // Direction of the light source (normalized)
uniform vec3 lightColor;     // Color of the light source
uniform vec3 ambientColor;   // Ambient light color

out vec4 finalColor; // Output fragment color

void main() {
    // Normalize the normal vector
    vec3 normal = normalize(worldNormal);

    // Calculate diffuse lighting using Lambertian reflection
    float diffuseIntensity = max(dot(normal, -lightDirection), 0.0);

    // Combine ambient and diffuse lighting
    vec3 lighting = ambientColor + (lightColor * diffuseIntensity);

    // Set the fragment color
    finalColor = vec4(lighting, 1.0) * colDiffuse * vcolor * texture(texture0, texCoord); // Alpha is 1.0 (opaque)
}
