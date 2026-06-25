#version 410 core
// debug_line.frag - jednolity kolor linii (NED-01, podglad splajnu).
out vec4 FragColor;

uniform vec3 lineColor;

void main()
{
    FragColor = vec4(lineColor, 1.0);
}
