#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 texCoord;

out vec2 fragTexCoord;

// Uniform for the light's MVP matrix
uniform mat4 mvp;

void main()
{
    fragTexCoord = texCoord;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
