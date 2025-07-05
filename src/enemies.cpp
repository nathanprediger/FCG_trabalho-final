#include "enemies.h"

Enemie::Enemie(glm::vec4 pos, glm::vec4 dir, float spd, float hp, Cube box)
    : position(pos), direction(dir), speed(spd), health(hp), alive(true), aggressive(false), boundingBox(box) {}

void Enemie::move(float delta_time)
{
    if (alive)
    {
        position += direction * speed * delta_time;
    }
}

void Enemie::takeDamage(float damage)
{
    if (alive)
    {
        health -= damage;
        if (health <= 0.0f)
        {
            alive = false;
        }
    }
}

void Enemie::set_aggressive(bool agro)
{
    aggressive = agro;
}

void Enemie::aggressive_direction(glm::vec4 player_pos)
{
    direction = aggressive ? normalize(player_pos - position) : direction;
}

void Enemie::player_spot(glm::vec4 player_pos){
    if(aggressive)
    {
        if(length(player_pos - position) > 10){
            direction = glm::vec4(0.0f,0.0f,0.0f,0.0f);
            set_aggressive(false);
        }
    }
    else{
        if(length(player_pos - position) < 5)
            set_aggressive(true);
    }
}