#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "collisions.h"
#include "bezier.h"
struct Enemie {
    Bezier_path* cur_path;
    glm::vec4 position;
    glm::vec4 prev_position;
    double time = 0.0f;
    float ang;
    glm::vec4 direction;
    float speed;
    float health;
    bool alive;
    bool aggressive;
    char type;
    Cube boundingBox;

    Enemie(glm::vec4 pos, glm::vec4 dir, float spd, float hp, char type, Cube box);

    void move(float delta_time);

    void takeDamage(float damage);

    void aggressive_direction(glm::vec4 player_pos);

    void set_aggressive(bool agro);

    void player_spot(glm::vec4 player_pos);
};