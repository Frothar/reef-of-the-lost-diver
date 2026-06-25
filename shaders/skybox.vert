#version 410 core

layout(location = 0) in vec3 aPos;

uniform mat4 viewProjection; // projection * mat4(mat3(view))  -- translation stripped

out vec3 texDir;

void main()
{
    texDir = aPos;
    vec4 pos = viewProjection * vec4(aPos, 1.0);
    // Force depth to the far plane (z = w) so the skybox is always behind everything.
    gl_Position = pos.xyww;
}
