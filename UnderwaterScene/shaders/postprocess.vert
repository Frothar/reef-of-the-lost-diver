#version 410 core
//
// postprocess.vert - OLE-07: fullscreen quad do post-processingu.
// Pozycje w NDC [-1,1], UV obliczone z pozycji.
//

layout(location = 0) in vec2 aPos;

out vec2 uv;

void main()
{
    uv = aPos * 0.5 + 0.5;  // [-1,1] -> [0,1]
    gl_Position = vec4(aPos, 0.0, 1.0);
}
