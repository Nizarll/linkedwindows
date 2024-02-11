#version 330

in vec2 fragCoord;

out vec4 fragColor;

uniform sampler2D texture; // Input texture

void main()
{
    // Bloom parameters
    const float threshold = 0.7; // Brightness threshold
    const float intensity = 8.0; // Bloom intensity

    vec4 color = texture2D(texture, fragCoord);

    // Calculate the brightness of the pixel
    float brightness = (color.r + color.g + color.b) / 3.0;

    // Apply bloom effect if brightness exceeds the threshold
    if (brightness > threshold)
    {
        // Increase the brightness of the pixel
        vec3 bloomColor = color.rgb * intensity;

        // Add the bloom color to the original color
        color.rgb += bloomColor;
    }

    fragColor = color;
}
