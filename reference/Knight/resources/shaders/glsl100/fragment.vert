#version 330

in float lifetime;       // Normalized lifetime from vertex shader
out vec4 finalColor;

void main() {
    // Set color based on lifetime (fade out as it reaches end of life)
    vec3 color = mix(vec3(1.0, 0.5, 0.0), vec3(1.0, 0.0, 0.0), lifetime);  // Color shift
    float alpha = 1.0 - lifetime;  // Fade out
    finalColor = vec4(color, alpha);
}
