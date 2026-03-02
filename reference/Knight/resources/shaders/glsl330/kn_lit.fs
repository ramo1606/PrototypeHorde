// kn_lit.fs Fragment shader program for rendering object with maximum 4 lightings and texture
#version 330

// Input vertex attributes (from vertex shader)
in vec3 worldPos;
in vec2 texUV;
in vec4 vtxColor;
in vec3 vtxNormal;  //assume the normal is already normalized

// These are default uniforms provided by Raylib shader system
uniform sampler2D texture0;    //texture sampler unit 0
uniform vec4 colDiffuse;    // color diffuse (base tint color, multiplied by texture color)
uniform int alphaTest;    // alpha test value (0 = disabled, 1 = enabled)

// Output fragment color
out vec4 outColor;

#define     MAX_LIGHTS              4
#define     LIGHT_DIRECTIONAL       0
#define     LIGHT_POINT             1

struct Light {
    int enabled;
    int type;
    vec3 position;
    vec3 target;
    vec4 color;
};

// Input lighting values
uniform Light lights[MAX_LIGHTS];
uniform vec4 ambient;
uniform vec3 viewPos;
uniform float materialShininess; // Material shininess factor

void main()
{

    vec4 texel = texture(texture0, texUV) * vtxColor;

    if (alphaTest == 1 && texel.a < 0.5)
        discard; // Apply alpha test if enabled    

    vec3 viewD = normalize(viewPos - worldPos);
    vec3 lightSum = vec3(0.0);
    vec3 specularSum = vec3(0.0);

    //Loop through all enabled lights to calculate lighting
    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights[i].enabled == 1) // Check if the light is enabled
        {
            vec3 light = vec3(0.0);

            if (lights[i].type == LIGHT_DIRECTIONAL)
            {
                light = -normalize(lights[i].target - lights[i].position);
            }

            if (lights[i].type == LIGHT_POINT)
            {
                light = normalize(lights[i].position - worldPos);
            }

           // Calculate diffuse component (Lambertian)
           // NdotL is max(0, dot(Normal, LightDirection))
            float NdotL = max(dot(vtxNormal, light), 0.0);
            lightSum += lights[i].color.rgb * NdotL;

            // Calculate specular component (Blinn-Phong)
            float specCo = 0.0;
            if (NdotL > 0.0) // Only calculate specular if diffuse light is present
            {
                // Blinn-Phong half-vector: normalized vector exactly halfway between view direction and light direction
                // This is different from the Phong model which uses the reflection vector in default raylib light shader
                vec3 halfVector = normalize(light + viewD);

                // NdotH is max(0, dot(Normal, HalfVector))
                float NdotH = max(dot(vtxNormal, halfVector), 0.0);

                // Raise NdotH to the power of the material's shininess
                specCo = pow(NdotH, materialShininess);
            }
            specularSum += specCo; // Accumulate specular contribution
        }
    }
    
    outColor = (texel*((colDiffuse + vec4(specularSum, 1.0))*vec4(lightSum, 1.0))) + ambient*colDiffuse;
    outColor = pow(outColor, vec4(1.0/1.2));      // Gamma correction

}
