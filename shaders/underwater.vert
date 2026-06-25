#version 410 core

// Vertex attribute layout matches Core::RenderContext::initFromAssimpMesh
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexTangent;
layout(location = 4) in vec3 vertexBitangent;

uniform mat4 transformation; // projection * view * model
uniform mat4 modelMatrix;

out vec3 worldPos;
out vec3 worldNormal;
out vec2 texCoord;

void main()
{
    worldPos    = vec3(modelMatrix * vec4(vertexPosition, 1.0));
    worldNormal = normalize(mat3(modelMatrix) * vertexNormal);
    texCoord    = vertexTexCoord;
    gl_Position = transformation * vec4(vertexPosition, 1.0);
}
