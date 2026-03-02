#version 330

in vec3 vertexPosition;
in vec3 vertexNormal;

uniform mat4 mvp;
uniform mat4 lightViewProj;

out vec3 fragPos;
out vec4 fragPosLightSpace;

void main() {
    fragPos = vertexPosition;
    fragPosLightSpace = lightViewProj * vec4(vertexPosition, 1.0);
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
