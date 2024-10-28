#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float intensity;

void main() {
    vec2 texelSize = 1.0 / resolution;
    vec4 originalColor = texture(texture0, fragTexCoord);

    // Blur parameters
    float blur = intensity;
    float total = 0.0;
    vec4 bloomColor = vec4(0.0);

    // Perform blur
    for (float x = -4.0; x <= 4.0; x++) {
        for (float y = -4.0; y <= 4.0; y++) {
            vec2 offset = vec2(x, y) * texelSize * blur;
            float weight = 1.0 - length(offset) * 0.25;
            if (weight <= 0.0) continue;

            bloomColor += texture(texture0, fragTexCoord + offset) * weight;
            total += weight;
        }
    }

    bloomColor /= total;

    // Blend original color with bloom
    finalColor = mix(originalColor, bloomColor, 0.5) * fragColor;
}
