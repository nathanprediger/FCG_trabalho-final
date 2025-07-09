#include "bezier.h"
#include "matrices.h"



glm::vec4 cubic_bezier_curve(Bezier_curve bez_c, double *t, double speedmult, double timedif){
    glm::vec4 pos = glm::vec4(0.0f,0.0f,0.0f,1.0f);
    // definição dos coeficientes de bezier
    double tval = abs(*t) - floor(abs(*t)); // caso t > 1 ou < 0, deixamos t em um periodo [0,1]
    double tcube = pow(tval,3);
    double tsquare = pow(tval,2);
    double negtcube = pow(1-tval, 3);
    double negtsquare = pow(1-tval, 2);
    double coefs[4] = {negtcube, 3*negtsquare*tval, 3*tsquare*(1-tval), tcube};
    //calculamos posicao
    for(int i = 0; i < 4; i++)
        pos += bez_c.points[i] * (float)coefs[i];
    // atualizamos t de acordo com a velocidade definida pelo usuario, comprimento aproximado do arco, e tempo passado entre ultima "mexida"
    *t += (timedif*speedmult)/bez_c.arcsize;
    return pos;
}

Bezier_curve define_cubic_bezier(glm::vec4 points[4]){
    Bezier_curve ret;
    // aproxima tamanho para 1000 pontos e retorna tamanho da curva aproximado para definir "velocidade"
    ret.arcsize = approximate_curve_size(points, 1000);
    // cria curva
    for(int i = 0;i < 4; i++)
        ret.points[i] = points[i];
    return ret;
}

double approximate_curve_size(glm::vec4 points[4], int approx_precision){
    double tval, tcube, tsquare, negtcube, negtsquare;
    double sumofdist = 0;
    glm::vec4 prev_point = points[0];
    glm::vec4 cur_point = glm::vec4(0.0f,0.0f,0.0f,1.0f);
    for (int i = 1; i < approx_precision; i++){
        // simula um calculo de bezier, pegando pontos com intervalos de "progresso" constantes
        // i/quantidade de pontos, i iterando de 1 ate a quantidade de pontos
        tval = (double)i/approx_precision;
        tcube = pow(tval,3);
        tsquare = pow(tval,2);
        negtcube = pow(1-tval, 3);
        negtsquare = pow(1-tval, 2);
        double coefs[4] = {negtcube, 3*negtsquare*tval, 3*tsquare*(1-tval), tcube};
        for(int j = 0; j < 4; j++)
            cur_point += points[j] * (float)coefs[j];
        // calculado o ponto, fazemos a distancia dele e seu ponto anterior, ressaltando que o primeiro ponto da curva ja é dado;
        sumofdist += abs(norm(cur_point - prev_point));
        // atualizamos ponto anterior como o ponto atual para iterar pelo loop inteiro
        prev_point = cur_point;
        cur_point = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    // ultimo ponto é dado na entrada da funcao, por isso calculamos os pontos [1, x - 1], x sendo quantidade de ponts;
    sumofdist += abs(norm(points[3] - prev_point));
    return sumofdist;
}

Bezier_path* link_curve_to_path(Bezier_path* path, glm::vec4 curva[2]){
    Bezier_path* atual = path;
    while(atual->next_curve != NULL)
        atual = atual->next_curve;
    // faz a "oposicao" do penultimo ponto de uma e do segundo da outra em relacao ao que fica entre eles
    glm::vec4 last_two_points_path[2] = {atual->cur_curve.points[2], atual->cur_curve.points[3]};
    glm::vec4 first_point_curve = last_two_points_path[1] + (last_two_points_path[1] - last_two_points_path[0]);

    Bezier_path* novo_path = (Bezier_path*)malloc(sizeof(Bezier_path));
    glm::vec4 points[4] = {last_two_points_path[1], first_point_curve, curva[0], curva[1]};
    Bezier_curve nova_curva = define_cubic_bezier(points);
    novo_path->cur_curve = nova_curva;
    novo_path->next_curve = NULL;
    //incrementa "depth" do ultimo
    atual->next_curve = novo_path;
    return path;
}

// não utilizado ainda, mas da free na memoria do path
void clear_path(Bezier_path* path){
    // liberamos sempre a curva atual, mantendo a proxima em memoria para nao perder o caminho dos nodos
    Bezier_path* cur_pos = path;
    Bezier_path* next_pos = path->next_curve;
    while(next_pos!=NULL){
        free(cur_pos);
        cur_pos = next_pos;
        next_pos= cur_pos->next_curve;
    }
    // quando next_pos == NULL, quer dizer que a curva atual é a ultima, assim damos free nela
    free(cur_pos);
}

//criamos um path a partir de uma curva, caso temos uma curva só, pode-se evitar essa funcao
// em prol de calcular a curva de bezier direto na struct da curva
// POSSIVEL ATUALIZAÇÃO, DAR "TEMPO" PARA PATH, ASSIM QUANDO CALCULAR BEZIER PATH PODERIAMOS UTILIZAR O T DE CADA UMA DAS CURVAS ISOLADO.
// OTIMA IDEIA NA REAL DEPOIS IMPLEMENTO
Bezier_path* create_path(Bezier_curve curva){
    Bezier_path* novo_path = (Bezier_path*)malloc(sizeof(Bezier_path));
    novo_path->cur_curve = curva;
    novo_path->next_curve = NULL;
    return novo_path;
}

//FUNCAO PARA ANDAR DE FATO AO LONGO DO CAMINHO DEFINIDO PELAS CURVAS.
glm::vec4 move_along_bezier_path(Bezier_path* path, double *t, double speedmult, double timedif){
    Bezier_path* atual = path; // DEFINIMOS CURVA ATUAL
    int t_int = (int)floor(abs(*t)); //T_INT É USADO PARA ACHAR QUAL CURVA DEVEMOS MEXER
    int prof_curva = 0;
    while(t_int != prof_curva && atual->next_curve != NULL){
        prof_curva++;
        atual = atual->next_curve;
    }
    if(t_int != prof_curva)//Devemos fazer o retorno
        *t = (t_int-0.0001f)*-1.0f;

    return cubic_bezier_curve(atual->cur_curve, t, speedmult, timedif);
}

#define MAX_CURVES 3
Bezier_path *generateRandomBezierPath(glm::vec4 start, int max_distance_points, int min_distance_points)
{   
    Bezier_path *path;
    glm::vec4 offset;
    int max_diff = abs(min_distance_points) + abs(max_distance_points);
    glm::vec4 staring_curve[4];
    glm::vec4 curves[MAX_CURVES][2];
    for(int i = 0; i < MAX_CURVES; i++){
        if(i == 0){
            //offset de posição não pode ser nulo, coelhinho nao iria se mexer e direção angular poderia dar infinito ou nan
            staring_curve[0] = start;
            offset = glm::vec4(min_distance_points + (rand() % max_diff), 0.0f, min_distance_points + (rand() % max_diff), 0.0f);
            while(length(offset) < 0.1f)
                offset = glm::vec4(min_distance_points + (rand() % max_diff), 0.0f, min_distance_points + (rand() % max_diff), 0.0f);
            staring_curve[1] = staring_curve[0] + offset;
            offset = glm::vec4(min_distance_points + (rand() % max_diff), 0.0f, min_distance_points + (rand() % max_diff), 0.0f);
            while(length(offset) < 0.1f)
                offset = glm::vec4(min_distance_points + (rand() % max_diff), 0.0f, min_distance_points + (rand() % max_diff), 0.0f);
            staring_curve[2] = staring_curve[1] + offset;
            offset = glm::vec4(min_distance_points + (rand() % max_diff), 0.0f, min_distance_points + (rand() % max_diff), 0.0f);
            while(length(offset) < 0.1f)
                offset = glm::vec4(min_distance_points + (rand() % max_diff), 0.0f, min_distance_points + (rand() % max_diff), 0.0f);
            staring_curve[3] = staring_curve[2] + offset;
            Bezier_curve curve = define_cubic_bezier(staring_curve);
            path = create_path(curve);
            curves[0][0] = staring_curve[2];
            curves[0][1] = staring_curve[3];
        }
        else{
            offset = glm::vec4(min_distance_points + (rand() % max_diff), 0.0f, min_distance_points + (rand() % max_diff), 0.0f);
            while(length(offset) < 0.1f)
                offset = glm::vec4(min_distance_points + (rand() % max_diff), 0.0f, min_distance_points + (rand() % max_diff), 0.0f);
            curves[i][0] = curves[i-1][1] +  curves[i-1][1] - curves[i-1][0] + offset;
            offset = glm::vec4(min_distance_points + (rand() % max_diff), 0.0f, min_distance_points + (rand() % max_diff), 0.0f);
            while(length(offset) < 0.1f)
                offset = glm::vec4(min_distance_points + (rand() % max_diff), 0.0f, min_distance_points + (rand() % max_diff), 0.0f);
            curves[i][1] = curves[i][0] + glm::vec4(min_distance_points + (rand() % max_diff), 0.0f, min_distance_points + (rand() % max_diff), 0.0f);
            path = link_curve_to_path(path, curves[i]);
        }
    }
    return path;
}

double direction_angle(glm::vec4 prev_point, glm::vec4 cur_point)
{
    glm::vec3 dir_vec = (cur_point - prev_point);
    if (glm::length(dir_vec) > 0.0f)
        return atan2(dir_vec.x, dir_vec.z);
    else
        return 0.0f;
}