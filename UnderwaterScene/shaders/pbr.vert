#version 410 core

// Vertex layout matches Core::RenderContext::initFromAssimpMesh
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexTangent;
layout(location = 4) in vec3 vertexBitangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 worldPos;
out vec3 worldNormal;
out vec2 texCoord;
out mat3 TBN;

void main()
{
    worldPos    = vec3(model * vec4(vertexPosition, 1.0));
    worldNormal = normalize(mat3(transpose(inverse(model))) * vertexNormal);
    texCoord    = vertexTexCoord;

    vec3 T = normalize(mat3(model) * vertexTangent);
    vec3 B = normalize(mat3(model) * vertexBitangent);
    vec3 N = worldNormal;
    TBN = mat3(T, B, N);

    gl_Position = projection * view * vec4(worldPos, 1.0);
}
