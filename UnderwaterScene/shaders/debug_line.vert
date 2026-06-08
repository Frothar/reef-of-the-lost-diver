#version 410 core
// debug_line.vert - prosty podglad linii (NED-01, podglad splajnu).
layout(location = 0) in vec3 aPos;

uniform mat4 viewProjection;

void main()
{
    gl_Position = viewProjection * vec4(aPos, 1.0);
}
