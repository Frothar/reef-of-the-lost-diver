#version 410 core
//
// shadow_depth.vert - OLE-04: minimalny vertex shader dla przebiegu cieni.
//
// Renderujemy geometrie z perspektywy swiatla (lightSpaceMatrix = ortho * view).
// Shader nie produkuje koloru - pisze tylko glebie do tekstury GL_DEPTH_COMPONENT.
//

layout(location = 0) in vec3 vertexPosition;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(vertexPosition, 1.0);
}
