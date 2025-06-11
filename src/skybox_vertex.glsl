#version 330 core

layout(location = 0) in vec4 position;

out vec3 texcoords;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main(){
    mat4 rotacao = mat4(mat3(view));
    texcoords = vec3(model*position);
    vec4 pos = projection* rotacao*position;
    texcoords = normalize(texcoords);
    gl_Position = pos;  
    gl_Position.z = pos.w;
    // z como 1 para evitar overdraw        =|:^)<=
}