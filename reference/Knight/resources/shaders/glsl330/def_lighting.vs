#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord; // Input texture coordinates

out vec2 fragTexCoord; // Output texture coordinates for the fragment shader

void main() {
    fragTexCoord = vertexTexCoord; // Pass texture coordinates to the fragment shader
    gl_Position = vec4(vertexPosition, 1.0); // Fullscreen quad vertex position
}
