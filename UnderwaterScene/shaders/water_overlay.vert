#version 410 core
// water_overlay.vert - pelnoekranowy quad (NED, lekki efekt "jestem pod woda").
// Pozycje w NDC, bez zadnych macierzy. Przekazuje uv [0,1] do fragmentu.
layout(location = 0) in vec2 aPos; // [-1,1] x [-1,1]

out vec2 uv;

void main()
{
    uv = aPos * 0.5 + 0.5;          // [-1,1] -> [0,1]
    gl_Position = vec4(aPos, 0.0, 1.0);
}
