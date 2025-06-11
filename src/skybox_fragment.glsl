#version 330 core
out vec4 skycolor;

in vec3 texcoords;

uniform sampler2D skybox;
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923


void main(){
    float theta = atan(texcoords.x, texcoords.z);
    float phi = asin(texcoords.y);
    float U = (theta + M_PI) / (2 * M_PI);
    float V = (phi == M_PI_2) ? 1 : (phi + M_PI_2) / M_PI;
    vec3 color = texture(skybox, vec2(U,V)).rgb;
    skycolor = vec4(color, 1.0);
}   
