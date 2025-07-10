#include "player.h"
#include "collisions.h"


#define MAP_MAX 49.5f
#define RUNNING_SPEED 3.0f
Player::Player(glm::vec4 pos, glm::vec4 dir, float spd, float hp, Cube box)
    : position(pos), direction(dir), deslocar(glm::vec4(0.0f,0.0f,0.0f,0.0f)), view_frente(glm::vec4(0.0f,0.0f,0.0f,0.0f)), 
      view_lado(glm::vec4(0.0f,0.0f,0.0f,0.0f)), speed(spd), health(hp), alive(true),
      jumping(false), running(false), y_vel(0.0f), stamina(100.0f), staminamax(100.0f), shooting(false),
      boundingBox(box), reloading(false), ammo(7) {}

void Player::updatePosition(float gravity, glm::vec4 diff_time) {
    position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    position.x += deslocar.x;
    position.y += deslocar.y;
    position.z += deslocar.z;
    deslocar.y -= gravity*diff_time.y;
    if(deslocar.y < 0.0f)
                deslocar.y = 0.0f;
}
void Player::move(glm::vec4 diff_time, char direction, float gravity) {
    float speed_multiplier = 1.0f;
    if(running) speed_multiplier = RUNNING_SPEED;
    glm::vec4 deslocar_anterior = deslocar;
    switch (direction){
        case 'F': deslocar -= speed * speed_multiplier * view_frente * diff_time; break;
        case 'B': deslocar += speed * speed_multiplier * view_frente * diff_time; break;
        case 'L': deslocar -= speed * speed_multiplier * view_lado * diff_time; break;
        case 'R': deslocar += speed * speed_multiplier * view_lado * diff_time; break;
        default: break;
    }
    glm::vec3 future_pos = glm::vec3(deslocar.x, deslocar.y, deslocar.z);
    if(boundingBox.colideWithPlane(boundaries[0], future_pos) || 
       boundingBox.colideWithPlane(boundaries[1], future_pos) ||
       boundingBox.colideWithPlane(boundaries[2], future_pos) ||
       boundingBox.colideWithPlane(boundaries[3], future_pos)) {
        deslocar = deslocar_anterior;
    }
}
void Player::takeDamage(float damage){
    if (alive) {
        health -= damage;
        if (health <= 0.0f) {
            alive = false;
        }
    }
}

void Player::jump(float gravity, glm::vec4 diff_time, bool space_press) {
    if(deslocar.y == 0.0f && stamina >= 10.0f && space_press == true){
        jumping = true;
        y_vel = 15.0f; 
        stamina -= 10.0f;
    }
    if(jumping){
        deslocar.y += y_vel * diff_time.y; 
        y_vel -= gravity*diff_time.y;
    }
    if(deslocar.y <= 0.0f){
        jumping = false;
    }
}

void Player::run (glm::vec4 diff_time) {
    if(running){
        if(stamina >0.05f) stamina -= diff_time.x * RUNNING_SPEED * 5.0f;
        else {
            running = false;
            stamina = 0.05f;
        }
    } 
    else {
        stamina += diff_time.x * 10.0f;
        if(stamina > staminamax) stamina = staminamax;
    }
}

