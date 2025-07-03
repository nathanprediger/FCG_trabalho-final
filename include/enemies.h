#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>

struct Enemie {
    glm::vec3 position;
    glm::vec3 direction;
    float speed;
    float health;
    bool alive;

    Enemie(glm::vec3 pos, glm::vec3 dir, float spd, float hp);

    void move(float delta_time);

    void takeDamage(float damage);
};