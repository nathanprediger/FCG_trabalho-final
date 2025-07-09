#include "enemies.h"


Enemie::Enemie(glm::vec4 pos, glm::vec4 dir, float spd, float hp, char t, Cube box)
    : position(pos), direction(dir), speed(spd), health(hp), alive(true), aggressive(false), type(t), boundingBox(box) {}

void Enemie::move(float delta_time)
{
    if (alive)
    {
        if(aggressive) {
            position += direction * speed * delta_time;
        }
        else{
            prev_position = position;
            position = move_along_bezier_path(cur_path, &time, speed, delta_time);
            if(boundingBox.colideWithPlane(boundaries[0], glm::vec3(position.x, position.y, position.z)) || 
               boundingBox.colideWithPlane(boundaries[1], glm::vec3(position.x, position.y, position.z)) ||
               boundingBox.colideWithPlane(boundaries[2], glm::vec3(position.x, position.y, position.z)) ||
               boundingBox.colideWithPlane(boundaries[3], glm::vec3(position.x, position.y, position.z)))
            {
                
                position = prev_position;

            
                clear_path(cur_path);

                // Gera uma nova trajetória aleatória a partir da posição atual.
                cur_path = generateRandomBezierPath(position, 3, -3);

                time = 0.0f;
            }
        }
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
            set_aggressive(false);
            clear_path(cur_path);
            cur_path = generateRandomBezierPath(position, 3, -3);   
            time = 0.0f;
        }
    }
    else{
        if(length(player_pos - position) < 5)
            set_aggressive(true);
    }
    
}