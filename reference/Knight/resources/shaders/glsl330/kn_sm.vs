#version 330

// vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform (values from raylib)
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

// NEW: Light-space matrices for each light
uniform mat4 lightSpaceMatrices[4];

// Output vertex attributes (to fragment shader)
out vec3 worldPos;
out vec2 texUV;
out vec4 vtxColor;
out vec3 vtxNormal;

// New: Fragment position in light's clip space
out vec4 fragPositionLightSpace[4]; 

void main()
{
    // Pass vertex attributes to fragment shader
    worldPos = vec3(matModel*vec4(vertexPosition, 1.0));
    texUV = vertexTexCoord;
    vtxColor = vertexColor;
    vtxNormal = normalize(vec3(matNormal*vec4(vertexNormal, 1.0)));

    // NEW: Calculate fragment position in each light's clip space
    for (int i = 0; i < 4; ++i) {
        fragPositionLightSpace[i] = lightSpaceMatrices[i] * vec4(vertexPosition, 1.0);
    }

    // Calculate final vertex position
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}

