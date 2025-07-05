#include "collisions.h"

Plane::Plane(const glm::vec3 &n, const glm::vec3 &p) : normal(n), point(p) {}

Sphere::Sphere(const glm::vec3 &c, float r) : center(c), radius(r) {}

bool Sphere::colideWithPoint(const glm::vec3 &ponto) const
{
    return glm::length(ponto - center) <= radius;
}

Cube::Cube(const glm::vec3 &m, const glm::vec3 &n) : max(m), min(n) {}
bool Cube::colideWithCube(const Cube &other) const
{
    return (max.x >= other.min.x && min.x <= other.max.x) &&
           (max.y >= other.min.y && min.y <= other.max.y) &&
           (max.z >= other.min.z && min.z <= other.max.z);
}
bool Cube::colideWithPlane(const Plane &Plane)
{
    glm::vec3 center = (max + min) / 2.0f;
    glm::vec3 extents = (max - min) / 2.0f;
    float d = glm::dot(Plane.normal, center - Plane.point);
    float r = glm::dot(glm::abs(Plane.normal), extents);
    return std::abs(d) <= r;
}