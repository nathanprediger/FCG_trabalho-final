#include "collisions.h"

Plane::Plane(const glm::vec3 &n, const glm::vec3 &p) : normal(n), point(p) {}

Sphere::Sphere(const glm::vec3 &c, float r) : center(c), radius(r) {}

bool Sphere::colideWithPoint(const glm::vec3 &ponto) const
{
    return glm::length(ponto - center) <= radius;
}

Cube::Cube(const glm::vec3 &m, const glm::vec3 &n) : max(m), min(n) {}
bool Cube::colideWithCube(const Cube &other, glm::vec4 pos_1, glm::vec4 pos_2) const
{
    return (max.x + pos_1.x >= other.min.x + pos_2.x && min.x + pos_1.x <= other.max.x + pos_2.x) &&
           (max.y + pos_1.y>= other.min.y + pos_2.y && min.y + pos_1.y <= other.max.y + pos_2.y) &&
           (max.z + pos_1.z >= other.min.z + pos_2.z && min.z + pos_1.z <= other.max.z + pos_2.z);
}
bool Cube::colideWithPlane(const Plane &Plane)
{
    glm::vec3 center = (max + min) / 2.0f;
    glm::vec3 extents = (max - min) / 2.0f;
    float d = glm::dot(Plane.normal, center - Plane.point);
    float r = glm::dot(glm::abs(Plane.normal), extents);
    return std::abs(d) <= r;
}