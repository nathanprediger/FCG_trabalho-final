#include <glm/vec4.hpp>
#include <cmath>

typedef struct bezier_curve_pair{
    glm::vec4 points[4]; // pontos da curva
    double arcsize; // para ter aproximadamente uma velocidade constante ao longo da curva
} Bezier_curve;

typedef struct bez_path{
    Bezier_curve cur_curve; // curva atual
    struct bez_path* next_curve; // proxima curva
    int curve_num; // quantas curvas faltam ate o fim, contando atual
}Bezier_path;


glm::vec4 cubic_bezier_curve(Bezier_curve bez_c, double *t, double speedmult, double timedif);
double approximate_curve_size(glm::vec4 points[4], int approx_precision);
Bezier_curve define_cubic_bezier(glm::vec4 points[4]);
Bezier_path* link_curve_to_path(Bezier_path* path, glm::vec4 curva[2]);
Bezier_path* create_path(Bezier_curve curva);
void clear_path(Bezier_path* path);
glm::vec4 move_along_bezier_path(Bezier_path* path, double *t, double speedmult, double timedif);