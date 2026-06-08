#version 410 core

in vec3 texDir;

uniform samplerCube skybox;
uniform vec3 waterTint;   // multiplied onto the cubemap for an underwater mood

out vec4 fragColor;

void main()
{
    vec3 color = texture(skybox, texDir).rgb * waterTint;
    fragColor = vec4(color, 1.0);
}
