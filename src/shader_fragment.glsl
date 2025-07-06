#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define SPHERE 0
#define BUNNY  1
#define PLANE  2
#define WINE 3
#define GUN 4
#define LEON 5
#define MALEZOMBIE 6
#define FEMALEZOMBIE 7
#define WOOD 8
#define TREE 9
#define HOUSE 10
#define FENCE 11
uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;
uniform int leon_part;
// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;

uniform vec3 material_Ka;
uniform vec3 material_Kd;
uniform vec3 material_Ks;
uniform float material_q;
uniform float material_opacity;
uniform int u_has_texture; 


// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

#define BLINN_PHONG 0 
#define LAMBERT 1

void main()
{
    
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal); 

    vec4 Lp = vec4(10.0,100.0,10.0,1.0);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    //vec4 l = normalize(Lp-p);
    vec4 l = normalize(vec4(-0.5,1.0,-0.5, 0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Vetor que define o sentido da reflexão especular ideal.
    vec4 r = normalize(-l+2*n*(dot(n,l))); // PREENCHA AQUI o vetor de reflexão especular ideal

    vec4 w = normalize(vec4(0.0,-1.0,0.0,0.0));

    vec4 h = normalize(v+l);

    vec3 Ks; // Refletância especular
    vec3 Ka; // Refletância ambiente
    float q;
    float alfa = 1.0;

    int illumination_type = 0;
    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;
    vec3 Kd0;
    if ( object_id == SPHERE )
    {
        // PREENCHA AQUI as coordenadas de textura da esfera, computadas com
        // projeção esférica EM COORDENADAS DO MODELO. Utilize como referência
        // o slides 134-150 do documento Aula_20_Mapeamento_de_Texturas.pdf.
        // A esfera que define a projeção deve estar centrada na posição
        // "bbox_center" definida abaixo.

        // Você deve utilizar:
        //   função 'length( )' : comprimento Euclidiano de um vetor
        //   função 'atan( , )' : arcotangente. Veja https://en.wikipedia.org/wiki/Atan2.
        //   função 'asin( )'   : seno inverso.
        //   constante M_PI
        //   variável position_model

        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

        U = 0.0;
        V = 0.0;
    }
    else if ( object_id == BUNNY )
    {
        // PREENCHA AQUI as coordenadas de textura do coelho, computadas com
        // projeção planar XY em COORDENADAS DO MODELO. Utilize como referência
        // o slides 99-104 do documento Aula_20_Mapeamento_de_Texturas.pdf,
        // e também use as variáveis min*/max* definidas abaixo para normalizar
        // as coordenadas de textura U e V dentro do intervalo [0,1]. Para
        // tanto, veja por exemplo o mapeamento da variável 'p_v' utilizando
        // 'h' no slides 158-160 do documento Aula_20_Mapeamento_de_Texturas.pdf.
        // Veja também a Questão 4 do Questionário 4 no Moodle.
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x - minx)/(maxx - minx);
        V = (position_model.y - miny)/(maxy -miny);

        illumination_type = BLINN_PHONG;

        Ka = vec3(0.04, 0.2, 0.4);
        Ks = vec3(0.8, 0.8, 0.8);
        Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
        q = 32;
    }
    else if ( object_id == PLANE )
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;
        Kd0 = texture(TextureImage2, vec2(U,V)).rgb;

        Ks = vec3(0.3, 0.3, 0.3);
        Ka = vec3(0.0,0.0,0.0);
        q = 20.0;

        illumination_type = LAMBERT;
    }
    else if(object_id == TREE){
        Ka = material_Ka;
        Ks = material_Ks;
        q = material_q;
        alfa = material_opacity;
        U = texcoords.x;
        V = texcoords.y;
        illumination_type = BLINN_PHONG;
        Kd0 = material_Kd * texture(TextureImage0, fract(vec2(U,V))).rgb;
        float alpha = texture(TextureImage1, texcoords).r;
        if (alpha < 0.5) discard;
    }
    else if (object_id == LEON || object_id == MALEZOMBIE || object_id == FEMALEZOMBIE || object_id == WOOD || 
    object_id == GUN || object_id == TREE || object_id == HOUSE || object_id == FENCE) {
        Ka = material_Ka;
        Ks = material_Ks;
        q = material_q;
        alfa = material_opacity;
        U = texcoords.x;
        V = texcoords.y;
        illumination_type = BLINN_PHONG;

        
        // Lógica condicional para a cor difusa
        if (u_has_texture == 1) {
            // Se tem textura, multiplica a cor do material pela cor da textura
            Kd0 = material_Kd * texture(TextureImage0, fract(vec2(U,V))).rgb;
        }
        else if (u_has_texture == 2){
            // Se tem textura, multiplica a cor do material pela cor da textura
            Kd0 = material_Kd * texture(TextureImage0, fract(vec2(U,V))).rgb;
            float alpha = texture(TextureImage1, texcoords).r;
            if (alpha < 0.5) discard;
        }
        else {
            // Se NÃO tem textura, usa apenas a cor do material
            Kd0 = material_Kd;
        }
        
    }
    // Espectro da fonte de iluminação
    vec3 I = vec3(0.2, 0.2, 0.2); // PREENCH AQUI o espectro da fonte de luz

    // Espectro da luz ambiente
    vec3 Ia = vec3(0.2, 0.2, 0.2); // PREENCHA AQUI o espectro da luz ambiente

    // Equação de Iluminação
    vec3 lambert = I*max(0,dot(n,l));

    // Termo ambiente
    vec3 ambient_term = Ka*Ia;

    // Termo especular utilizando o modelo de iluminação de Blinn-Phong
    vec3 blinn_phong_specular_term = Ks*I*pow(max(0.0, dot(n.xyz,h.xyz)), q);

   // if(dot(w.xyz,(normalize(p-Lp)).xyz)>=cos(radians(30.0))){
        if(illumination_type == LAMBERT){
        color.rgb = Kd0 * lambert + ambient_term;
        }
        else if (illumination_type == BLINN_PHONG) {
        color.rgb = Kd0 * (lambert) + ambient_term + blinn_phong_specular_term;
        }
    //}
    //else
    //    color.rgb = Kd0 * (lambert + vec3(0.005)) + ambient_term;
    

    // NOTE: Se você quiser fazer o rendering de objetos transparentes, é
    // necessário:
    // 1) Habilitar a operação de "blending" de OpenGL logo antes de realizar o
    //    desenho dos objetos transparentes, com os comandos abaixo no código C++:
    //      glEnable(GL_BLEND);
    //      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // 2) Realizar o desenho de todos objetos transparentes *após* ter desenhado
    //    todos os objetos opacos; e
    // 3) Realizar o desenho de objetos transparentes ordenados de acordo com
    //    suas distâncias para a câmera (desenhando primeiro objetos
    //    transparentes que estão mais longe da câmera).
    // Alpha default = 1 = 100% opaco = 0% transparente
    color.a = alfa;

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
} 

