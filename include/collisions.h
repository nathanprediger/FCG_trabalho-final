#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "matrices.h"

// Estrutura para um plano
struct Plane {
    glm::vec3 normal;
    glm::vec3 point;

    Plane(const glm::vec3& n, const glm::vec3& p);
};
extern Plane boundaries[4];
// Estrutura para uma esfera
struct Sphere {
    glm::vec3 center;
    float radius;

    Sphere(const glm::vec3& c, float r);

    bool colideWithPlane(const Plane& plane, glm::vec3 cur_pos) const;
    bool colideWithPoint(const glm::vec3 deslocamentocentro, glm::vec3 ponto) const;
};

// Estrutura para um cubo (AABB)
struct Cube {
    glm::vec3 max;
    glm::vec3 min;

    Cube(const glm::vec3& m, const glm::vec3& n);
    bool colideWithPoint(glm::vec4 cubepos, glm::vec4 point);
    bool colideWithCube(const Cube &other, glm::vec4 pos_1, glm::vec4 pos_2) const;
    bool colideWithPlane(const Plane& plane, glm::vec3 cur_pos) const;
    bool colideWithRay(glm::vec4 ray, glm::vec4 origin, glm::vec4 cube_org);
};