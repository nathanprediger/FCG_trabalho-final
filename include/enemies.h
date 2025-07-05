#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "collisions.h"

struct Enemie {
    glm::vec4 position;
    glm::vec4 direction;
    float speed;
    float health;
    bool alive;
    bool aggressive;
    Cube boundingBox;

    Enemie(glm::vec4 pos, glm::vec4 dir, float spd, float hp, Cube box);

    void move(float delta_time);

    void takeDamage(float damage);

    void aggressive_direction(glm::vec4 player_pos);

    void set_aggressive(bool agro);

    void player_spot(glm::vec4 player_pos);
};