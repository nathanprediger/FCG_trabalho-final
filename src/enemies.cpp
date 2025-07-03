#include "enemies.h"
#include "collisions.h"
struct Enemie {
    glm::vec3 position;
    glm::vec3 direction;
    float speed;
    float health;
    bool alive;
    Cube boundingBox; // Caixa delimitadora para colisões

    Enemie(glm::vec3 pos, glm::vec3 dir, float spd, float hp, Cube box)
        : position(pos), direction(dir), speed(spd), health(hp), alive(true),boundingBox(box) {}

    void move(float delta_time) {
        if (alive) {
            position += direction * speed * delta_time;
        }
    }

    void takeDamage(float damage) {
        if (alive) {
            health -= damage;
            if (health <= 0.0f) {
                alive = false;
            }
        }
    }
};