// kn_lit.vs - Vertex Shader for simple lighting in Kniht Framework
#version 330

// vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// These are default uniforms from Raylib shader system
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

// Output vertex attributes (to fragment shader)
out vec3 worldPos;
out vec2 texUV;
out vec4 vtxColor;
out vec3 vtxNormal;

// NOTE: Add here your custom variables

void main()
{
    // Send vertex attributes to fragment shader
    worldPos = vec3(matModel*vec4(vertexPosition, 1.0));
    texUV = vertexTexCoord;
    vtxColor = vertexColor;
    vtxNormal = normalize(vec3(matNormal*vec4(vertexNormal, 1.0)));

    // Calculate final vertex position
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}

//end of kn_lit.vs
