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
    while(atual->next_curve != NULL){
        //incrementa "depth"
        atual->curve_num++;
        atual = atual->next_curve;
    }
    // faz a "oposicao" do penultimo ponto de uma e do segundo da outra em relacao ao que fica entre eles
    glm::vec4 last_two_points_path[2] = {atual->cur_curve.points[2], atual->cur_curve.points[3]};
    glm::vec4 first_point_curve = last_two_points_path[1] + (last_two_points_path[1] - last_two_points_path[0]);

    Bezier_path* novo_path = (Bezier_path*)malloc(sizeof(Bezier_path));
    glm::vec4 points[4] = {last_two_points_path[1], first_point_curve, curva[0], curva[1]};
    Bezier_curve nova_curva = define_cubic_bezier(points);
    novo_path->cur_curve = nova_curva;
    novo_path->curve_num = 1;
    novo_path->next_curve = NULL;
    //incrementa "depth" do ultimo
    atual->curve_num++;
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
    novo_path->curve_num = 1; // ultimo nodo do path, depois sera incrementado
    return novo_path;
}

//FUNCAO PARA ANDAR DE FATO AO LONGO DO CAMINHO DEFINIDO PELAS CURVAS.
glm::vec4 move_along_bezier_path(Bezier_path* path, double *t, double speedmult, double timedif){
    Bezier_path atual = *path; // DEFINIMOS CURVA ATUAL
    int t_int = (int)floor(abs(*t)); //T_INT É USADO PARA ACHAR QUAL CURVA DEVEMOS MEXER,
    // PODE SER TROCADO DEPOIS PARA REFLETIR MUDANÇAS PROPOSTAS NOS COMENTARIOS ACIMA
    while(t_int != (path->curve_num - atual.curve_num) && atual.next_curve != NULL)
        atual = *atual.next_curve; //Percorremos a lista de curvas até chegarmos ou na ultima curva ou na correspondente ao t atual
    if(t_int != (path->curve_num - atual.curve_num)) // caso estejamos na ultima curva mas t indica que deveriamos estar na proxima (não existe)
    //NULL INVERTE t para -t_int, assim já que utilizamos abs na funcao cubic_bezier_curve, iremos, por exemplo, de 1.01
    // que esta fora do range [0, 1] para -0.999, que apos o abs fica 0.999 então dentro do range [0, 1], agora o incremento em t faz o caminho de retorno
        *t = (t_int - 0.001)*-1;
    return cubic_bezier_curve(atual.cur_curve, t, speedmult, timedif);

}