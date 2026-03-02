#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;

uniform mat4 matModel;      // Model matrix
uniform mat4 matView;       // View matrix
uniform mat4 matProjection; // Projection matrix

out vec3 fragPosition;   // World-space position to fragment shader
out vec3 fragNormal;     // Normal to fragment shader
out vec2 fragTexCoord;   // Texture coordinates to fragment shader

void main() {
    vec4 worldPosition = matModel * vec4(vertexPosition, 1.0); // Transform to world space
    fragPosition = worldPosition.xyz;                      // Pass world-space position
    fragNormal = mat3(transpose(inverse(matModel))) * vertexNormal; // Transform normal to world space
    fragTexCoord = vertexTexCoord;                         // Pass texture coordinates

    gl_Position = matProjection * matView * worldPosition;       // Transform to clip space
}
