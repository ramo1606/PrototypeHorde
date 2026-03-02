#version 330

in vec2 fragTexCoord;

uniform sampler2D texture0;

uniform sampler2D gPosition;  // G-buffer position
uniform sampler2D gNormal;    // G-buffer normal
uniform sampler2D gAlbedo;    // G-buffer albedo
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform vec3 viewPosition;

out vec4 fragColor;

void main() {
    // Retrieve data from G-buffer
    //vec3 fragPosition = texture(gPosition, fragTexCoord).rgb;
    //vec3 fragNormal = normalize(texture(gNormal, fragTexCoord).rgb);
    //vec4 fragAlbedo = texture(gAlbedo, fragTexCoord);

    // Lighting calculations
   // vec3 lightDir = normalize(lightPosition - fragPosition);
    //float diff = max(dot(fragNormal, lightDir), 0.0);

    //vec3 diffuse = diff * lightColor * fragAlbedo.rgb;

    //vec3 viewDir = normalize(viewPosition - fragPosition);
    //vec3 reflectDir = reflect(-lightDir, fragNormal);
    //float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
   // vec3 specular = spec * lightColor;

    //vec3 finalColor = diffuse + specular;
    //fragColor = vec4(finalColor, 1.0);

    //fragColor = vec4(fragAlbedo.rgb,1);
    fragColor = texture(texture0, fragTexCoord);
}
