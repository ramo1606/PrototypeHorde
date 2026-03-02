#version 330

// Input vertex attributes
in vec3 vertexPosition;

// Uniform for the light's MVP matrix
uniform mat4 mvp;

void main()
{
    // Simply transform the vertex position to clip space
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
