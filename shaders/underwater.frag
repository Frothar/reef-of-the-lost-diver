#version 410 core

// Minimal lit + underwater-fog starter shader.
// Replace / extend with the PBR pipeline (OLE-01..05) as the project grows.

in vec3 worldPos;
in vec3 worldNormal;
in vec2 texCoord;

uniform vec3 cameraPos;
uniform vec3 lightDir;    // direction TO the sun (normalized), e.g. light from above
uniform vec3 lightColor;
uniform vec3 objectColor;

// Underwater atmosphere
uniform vec3  fogColor;   // teal/deep-blue water tint
uniform float fogDensity; // exponential fog strength

out vec4 fragColor;

void main()
{
    vec3 N = normalize(worldNormal);
    vec3 L = normalize(lightDir);
    vec3 V = normalize(cameraPos - worldPos);

    // Simple Lambert + Blinn-Phong specular
    float diff = max(dot(N, L), 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 64.0);

    vec3 ambient = 0.18 * fogColor;
    vec3 color = objectColor * (ambient + diff * lightColor) + 0.25 * spec * lightColor;

    // Exponential distance fog toward the water color (depth cue)
    float dist = length(cameraPos - worldPos);
    float fog = 1.0 - exp(-fogDensity * dist);
    color = mix(color, fogColor, clamp(fog, 0.0, 1.0));

    fragColor = vec4(color, 1.0);
}
