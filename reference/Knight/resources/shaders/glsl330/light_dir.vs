#version 330

layout(location = 0) in vec3 vertexPosition; // Vertex position
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal;   // Vertex normal
layout(location = 3) in vec4 vertexColor;

uniform mat4 matModel;       // Model transformation matrix
uniform mat4 matView;        // View transformation matrix
uniform mat4 matProjection;  // Projection matrix

out vec3 worldNormal;      // Pass the transformed world space normal to the fragment shader
out vec2 texCoord;
out vec4 vcolor;

void main() {
    texCoord = vertexUV;
    vcolor = vertexColor;

    // Transform the vertex position into world space
    vec4 worldPosition = matModel * vec4(vertexPosition, 1.0);

    // Transform the normal vector
    worldNormal = mat3(transpose(inverse(matModel))) * vertexNormal;

    // Calculate final vertex position in clip space
    gl_Position = matProjection * matView * worldPosition;
}
