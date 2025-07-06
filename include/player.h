#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "collisions.h"

struct Player {
    glm::vec4 position;
    glm::vec4 direction;
    glm::vec4 deslocar;
    glm::vec4 view_frente;
    glm::vec4 view_lado;
    float speed;
    float health;
    bool alive;
    bool jumping;
    bool running;
    float y_vel;
    float stamina;
    float staminamax;
    Cube boundingBox;

    Player(glm::vec4 pos, glm::vec4 dir, float spd, float hp, Cube box);

    void updatePosition(float gravity, glm::vec4 diff_time);

    void move(glm::vec4 diff_time, char direction, float gravity);

    void takeDamage(float damage);

    void jump(float gravity, glm::vec4 diff_time, bool space_press);

    void run(glm::vec4 diff_time);
};