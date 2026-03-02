#version 330

in vec3 fragPosition; // World-space position from vertex shader
in vec3 fragNormal;   // Normal from vertex shader
in vec2 fragTexCoord; // Texture coordinates from vertex shader

uniform sampler2D texture0; // Albedo texture

//layout(location = 0) out vec3 gPosition; // G-buffer position
//layout(location = 1) out vec3 gNormal;   // G-buffer normal
//layout(location = 2) out vec4 gAlbedo;   // G-buffer albedo

out vec3 gPosition; // G-buffer position
out vec3 gNormal;   // G-buffer normal
out vec4 gAlbedo;   // G-buffer albedo

void main() {
    gPosition = fragPosition;               // Write world-space position to G-buffer
    gNormal = normalize(fragNormal);        // Write normalized normal to G-buffer
    gAlbedo = texture(texture0, fragTexCoord); // Write albedo to G-buffer
}
