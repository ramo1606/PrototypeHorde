#version 330

// Input attributes
layout(location = 0) in vec3 vertexPosition;
layout(location = 2) in vec3 vertexNormal;
layout(location = 1) in vec2 vertexTexCoord;

// Uniforms
uniform mat4 matModel;
uniform mat4 matView;
uniform mat4 matProjection;

out vec3 fragPosition; // Position in world space
out vec3 fragNormal;   // Normal in world space

void main() {
    // Transform vertex position to world space
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));

    // Transform normal to world space and normalize
    fragNormal = mat3(transpose(inverse(matModel))) * vertexNormal;

    // Final position in screen space
    gl_Position = matProjection * matView * vec4(fragPosition, 1.0);
}
