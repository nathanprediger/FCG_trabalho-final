#version 330 core

// Atributos de vértice
layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;

// Matrizes
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// ====================================================================
// NOVO UNIFORM PARA CONTROLE
// ====================================================================
uniform int shading_model; // 0 = Gouraud, 1 = Phong
// ====================================================================

// Saídas para o Fragment Shader
out vec4 position_world;
out vec4 position_model;
out vec4 normal;
out vec2 texcoords;

// ====================================================================
// NOVA SAÍDA (APENAS PARA O CAMINHO GOURAUD)
// ====================================================================
out vec3 gouraud_color;
// ====================================================================

// --- Constantes e Uniforms para o caminho Gouraud ---
#define GOURAUD 0
#define PHONG 1

// Precisamos de todos os uniforms aqui para o cálculo Gouraud
uniform int object_id;
uniform vec4 bbox_min;
uniform vec4 bbox_max;
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform vec3 material_Ka;
uniform vec3 material_Kd;
uniform vec3 material_Ks;
uniform float material_q;
uniform int u_has_texture;
vec4 color;
// Object IDs
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

#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

#define BLINN_PHONG 0 
#define LAMBERT 1

void main()
{
    // A variável gl_Position define a posição final de cada vértice
    // OBRIGATORIAMENTE em "normalized device coordinates" (NDC), onde cada
    // coeficiente estará entre -1 e 1 após divisão por w.
    // Veja {+NDC2+}.
    //
    // O código em "main.cpp" define os vértices dos modelos em coordenadas
    // locais de cada modelo (array model_coefficients). Abaixo, utilizamos
    // operações de modelagem, definição da câmera, e projeção, para computar
    // as coordenadas finais em NDC (variável gl_Position). Após a execução
    // deste Vertex Shader, a placa de vídeo (GPU) fará a divisão por W. Veja
    // slides 41-67 e 69-86 do documento Aula_09_Projecoes.pdf.

    gl_Position = projection * view * model * model_coefficients;

    // Como as variáveis acima  (tipo vec4) são vetores com 4 coeficientes,
    // também é possível acessar e modificar cada coeficiente de maneira
    // independente. Esses são indexados pelos nomes x, y, z, e w (nessa
    // ordem, isto é, 'x' é o primeiro coeficiente, 'y' é o segundo, ...):
    //
    //     gl_Position.x = model_coefficients.x;
    //     gl_Position.y = model_coefficients.y;
    //     gl_Position.z = model_coefficients.z;
    //     gl_Position.w = model_coefficients.w;
    //

    // Agora definimos outros atributos dos vértices que serão interpolados pelo
    // rasterizador para gerar atributos únicos para cada fragmento gerado.

    // Posição do vértice atual no sistema de coordenadas global (World).
    position_world = model * model_coefficients;

    // Posição do vértice atual no sistema de coordenadas local do modelo.
    position_model = model_coefficients;

    // Normal do vértice atual no sistema de coordenadas global (World).
    // Veja slides 123-151 do documento Aula_07_Transformacoes_Geometricas_3D.pdf.
    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;

    // Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
    texcoords = texture_coefficients;

    if(shading_model == GOURAUD){
    vec3 final_color_rgb;
        
        vec4 camera_position = inverse(view) * vec4(0.0, 0.0, 0.0, 1.0);
        vec4 p = position_world;
        vec4 n = normalize(normal); 
        vec4 l = normalize(vec4(-0.5, 1.0, -0.5, 0.0));
        vec4 v = normalize(camera_position - p);
        vec4 h = normalize(v + l);

        vec3 I = vec3(0.2, 0.2, 0.2);
        vec3 Ia = vec3(0.2, 0.2, 0.2);
        vec3 Kd0;

        if (u_has_texture > 0) {
            Kd0 = material_Kd * texture(TextureImage0, texcoords).rgb;
        } else {
            Kd0 = material_Kd;
        }

        vec3 lambert_term = I * max(0, dot(n, l));
        vec3 ambient_term = material_Ka * Ia;
        vec3 blinn_phong_specular_term = material_Ks * I * pow(max(0.0, dot(n.xyz, h.xyz)), material_q);

        final_color_rgb = Kd0 * lambert_term + ambient_term + blinn_phong_specular_term;

        gouraud_color = pow(final_color_rgb, vec3(1.0)/2.2);
    }
    else if(shading_model == PHONG){
        gouraud_color = vec3(0.0, 0.0, 0.0);
    }
}

