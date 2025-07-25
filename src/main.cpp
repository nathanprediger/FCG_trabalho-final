//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   LABORATÓRIO 5
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#define MINIAUDIO_IMPLEMENTATION
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <set>
#include <iostream>
#include <cstdlib>
#include <ctime>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>  // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h> // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "collisions.h"
#include "enemies.h"
#include "bezier.h"
#include "player.h"
#include "miniaudio.h"

#define BUNNYBOX Sphere(glm::vec3(0.0f, 0.0f, 0.0f), 0.8f);
#define HUMANOIDBOX Cube(glm::vec3(0.35f, 2.0f, 0.35f), glm::vec3(-0.35f, 0.0f, -0.35f))
#define TREEBOX Cube(glm::vec3(0.6f, 3.0f, 0.6f), glm::vec3(-0.6f, 0.0f, -0.6f))
#define LOGBOX Cube(glm::vec3(1.0f, 2.0f, 1.0f), glm::vec3(-1.0f, 0.0f, -1.0f))
#define MAP_LENGTH 100
#define X_MIN 5
#define X_MAX 48
#define Z_MIN 5
#define Z_MAX 48
#define NUM_ENEMIES 15
#define NUM_TREES 25
#define LEON_SPEED 2.0f
#define LEON_HP 10.0f
#define LEON_AMMO 7
#define INITIAL_POS glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)  // Posição inicial do Leon
#define INITIAL_VIEW glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) // Vetor de visão inicial do Leon
#define NUM_BUNNIES 5
#define NUM_LOGS 10
#define BUNNY_MAX_SPEED 800
#define TIME_LIMIT 180

typedef struct BUNNY
{
    Bezier_path *bunny_path;
    glm::vec4 cur_position;
    glm::vec4 prev_position;
    double bunny_time = 0.0f;
    double bunny_speed;
    float ang;
    bool taken = false;
} Bunny;
#define MAX_CURVES 3

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char *filename, const char *basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretório dos arquivos OBJ.
        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i + 1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                        filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};

// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4 &M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel *);                                                                                      // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel *model);                                                                                                     // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles();                                                                                                              // Carrega os shaders de vértice e fragmento, criando um programa de GPU
GLuint LoadTextureImage(const char *filename, int type);                                                                                  // Função que carrega imagens de textura
std::map<std::string, GLuint> LoadTexturesFromObjModel(ObjModel *model, std::string base_path);                                           // Carrega texturas de um modelo obj
void DrawVirtualObject(const char *object_name);                                                                                          // Desenha um objeto armazenado em g_VirtualScene
void DrawVirtualObjectMtl(char obj_list[], int arrsize, ObjModel *model, std::map<std::string, GLuint> textures_name_to_id, int obj_num); // desenha com mtl
GLuint LoadShader_Vertex(const char *filename);                                                                                           // Carrega um vertex shader
GLuint LoadShader_Fragment(const char *filename);                                                                                         // Carrega um fragment shader
void LoadShader(const char *filename, GLuint shader_id);                                                                                  // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);                                                              // Cria um programa de GPU
void PrintObjModelInfo(ObjModel *);                                                                                                       // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow *window);
float TextRendering_CharWidth(GLFWwindow *window);
void TextRendering_PrintString(GLFWwindow *window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow *window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow *window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow *window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow *window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow *window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow *window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowStamina(GLFWwindow *window, float stamina, float maxstamina);
void TextRendering_ShowAmmo(GLFWwindow *window, int ammo, int maxammo);
void TextRendering_Crosshair(GLFWwindow *window);
void TextRendering_ShowProjection(GLFWwindow *window);
void TextRendering_ShowFramesPerSecond(GLFWwindow *window);
void TextRendering_ShowTimer(GLFWwindow *window);
void TextRendering_ShowStartObjective(GLFWwindow *window);
void TextRendering_ShowObjectiveProgress(GLFWwindow *window, int coelhinhos);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
void ErrorCallback(int error, const char *description);
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow *window, double xpos, double ypos);
void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
//Funcoes de geração de objetos
std::vector<Enemie> generateEnemies(int map_occupation[MAP_LENGTH][MAP_LENGTH]);
std::vector<std::pair<float, float>> generateStaticObj(int map_occupation[MAP_LENGTH][MAP_LENGTH], int object_quant);
std::vector<Bunny> generateBunnies(int map_occupation[MAP_LENGTH][MAP_LENGTH]);
// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string name;              // Nome do objeto
    size_t first_index;            // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t num_indices;            // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum rendering_mode;         // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3 bbox_min;            // Axis-Aligned Bounding Box do objeto
    glm::vec3 bbox_max;
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4> g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;
#define M_PI 3.14159265358979323846

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false;  // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f;    // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;      // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;
GLint g_has_texture_uniform;
GLint g_shading_model_uniform;
GLint g_point_light_pos_uniform;
GLint g_point_light_color_uniform;
GLint g_point_light_active_uniform;
// SKYBOX SHADERS VAR
GLuint g_GpuProgramSkyboxID = 0;
GLint g_skybox_model_uniform;
GLint g_skybox_view_uniform;
GLint g_skybox_projection_uniform;
// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;
bool a_press = false;
bool s_press = false;
bool d_press = false;
bool w_press = false;
bool space_press = false;
bool shift_press = false;
bool mouse_move = false;
bool paused = false;
bool first_person_view = true;
float gravity = 9.8f;
bool g_muzzleFlashActive = false;
float g_muzzleFlashTimer = 0.0f;
double sensitivity = 0.5f;

Plane boundaries[4] = {
        Plane(glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, -MAP_LENGTH/2, 1.0f)), // Plane XZ at Z_MIN
        Plane(glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, MAP_LENGTH/2, 1.0f)), // Plane XZ at Z_MAX
        Plane(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::vec4(-MAP_LENGTH/2, 0.0f, 0.0f, 1.0f)), // Plane YZ at X_MIN
        Plane(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::vec4(MAP_LENGTH/2, 0.0f, 0.0f, 1.0f)) // Plane YZ at X_MAX
 };
int main(int argc, char *argv[])
{

    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    ma_result result;
    ma_engine engine;
    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS)
    {
        printf("Erro no miniaudio\n");
        std::exit(EXIT_FAILURE);
    }
    ma_sound deagleshot[LEON_AMMO];
    ma_sound deaglereload;
    for(int i = 0; i < LEON_AMMO; i++){
        result = ma_sound_init_from_file(&engine, "../../data/audios/deagleshoot.mp3", 0, NULL, NULL, &deagleshot[i]);
        if (result != MA_SUCCESS)
        {
            printf("Erro no miniaudio\n");
            std::exit(EXIT_FAILURE);
        }
    }
    result = ma_sound_init_from_file(&engine, "../../data/audios/deaglereload.mp3", 0, NULL, NULL, &deaglereload);
    if (result != MA_SUCCESS)
    {
        printf("Erro no miniaudio\n");
        std::exit(EXIT_FAILURE);
    }
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }
    unsigned seed = time(0);
    srand(seed);
    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow *window;
    window = glfwCreateWindow(800, 600, "Mal Residente -1", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *glversion = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    GLuint earth_id = LoadTextureImage("../../data/tc-earth_daymap_surface.jpg", 0);                             // TextureImage0
    GLuint earth_night_id = LoadTextureImage("../../data/tc-earth_nightmap_citylights.gif", 0);                  // TextureImage1
    GLuint rocky_terrain_id = LoadTextureImage("../../data/rocky_terrain_02_diff_1k.jpg", 1);                    // TextureImage2
    GLuint skybox_id = LoadTextureImage("../../data/skyboxes/satara_night_no_lamps_2k.hdr", 0);                  // TextureImage3
    GLuint tree_mask_id = LoadTextureImage("../../data/tree/A_5e6233bf8b6646988a1a6e8dc4697172_ped-tga.png", 0); // TextureImage4

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel planemodel("../../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel bunnymodel("../../data/bunny.obj");
    ComputeNormals(&bunnymodel);
    BuildTrianglesAndAddToVirtualScene(&bunnymodel);

    ObjModel spheremodel("../../data/skyboxes/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel leonmodel("../../data/leon/JD3L9JZFR7UB7NMGULPCJC51T.obj");
    ComputeNormals(&leonmodel);
    BuildTrianglesAndAddToVirtualScene(&leonmodel);
    std::map<std::string, GLuint> leon_textures = LoadTexturesFromObjModel(&leonmodel, "../../data/leon/");

    ObjModel malezombiemodel("../../data/malezombie/F00DQG8JS2X7AFI95CFGV3WQT.obj");
    ComputeNormals(&malezombiemodel);
    BuildTrianglesAndAddToVirtualScene(&malezombiemodel);
    std::map<std::string, GLuint> malezombie_textures = LoadTexturesFromObjModel(&malezombiemodel, "../../data/malezombie/");

    ObjModel femalezombiemodel("../../data/femalezombie/00MGC5O1PDBT3MS4X048REERB.obj");
    ComputeNormals(&femalezombiemodel);
    BuildTrianglesAndAddToVirtualScene(&femalezombiemodel);
    std::map<std::string, GLuint> femalezombie_textures = LoadTexturesFromObjModel(&femalezombiemodel, "../../data/femalezombie/");

    ObjModel woodmodel("../../data/tronco/NL9JQ9SO03UR1QS6G2RU80S4N.obj");
    ComputeNormals(&woodmodel);
    BuildTrianglesAndAddToVirtualScene(&woodmodel);
    std::map<std::string, GLuint> wood_textures = LoadTexturesFromObjModel(&woodmodel, "../../data/tronco/");

    ObjModel chainfencemodel("../../data/chainfence/fencelinda.obj");
    ComputeNormals(&chainfencemodel);
    BuildTrianglesAndAddToVirtualScene(&chainfencemodel);
    std::map<std::string, GLuint> chainfence_textures = LoadTexturesFromObjModel(&chainfencemodel, "../../data/chainfence/");

    ObjModel gunmodel("../../data/GUN/VOUQ5MT40MFG4TEJ3DSCA3A7K.obj");
    ComputeNormals(&gunmodel);
    BuildTrianglesAndAddToVirtualScene(&gunmodel);
    std::map<std::string, GLuint> gun_textures = LoadTexturesFromObjModel(&gunmodel, "../../data/GUN/");

    ObjModel treemodel("../../data/tree/model.obj");
    ComputeNormals(&treemodel);
    BuildTrianglesAndAddToVirtualScene(&treemodel);
    std::map<std::string, GLuint> tree_textures = LoadTexturesFromObjModel(&treemodel, "../../data/tree/");

    //modelo do gastal gerado com IA
    ObjModel gastalmodel("../../data/gastal/gastal.obj");
    ComputeNormals(&gastalmodel);
    BuildTrianglesAndAddToVirtualScene(&gastalmodel);
    std::map<std::string, GLuint> gastal_textures = LoadTexturesFromObjModel(&gastalmodel, "../../data/gastal/");

    if (argc > 1)
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    bool lastpaused = paused;
    int amountbunniestaken = 0;
    double timeprev = glfwGetTime();
    glm::vec4 timeprevec = glm::vec4(timeprev, timeprev, timeprev, 0.0f);



    //generating random object positions
int map_ocupation[MAP_LENGTH][MAP_LENGTH] = {0};

    std::vector<Enemie> enemies = generateEnemies(map_ocupation);
    std::vector<std::pair<float, float>> tree_positions = generateStaticObj(map_ocupation, NUM_TREES);
    std::vector<Bunny> bunnies = generateBunnies(map_ocupation);
    std::vector<std::pair<float, float> > log_positions = generateStaticObj(map_ocupation, NUM_LOGS);
    Cube tree_BBOX = TREEBOX;
    Sphere bunny_BBOX = BUNNYBOX;
    Cube log_BBOX = LOGBOX;
    struct Player Leon(INITIAL_POS, INITIAL_VIEW, LEON_SPEED, LEON_HP, HUMANOIDBOX);
    double shootinganim = 0.0f;
    double reloadinganim = 0.0f;


    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().
        float r = g_CameraDistance * 1.5;
        float y = r * sin(g_CameraPhi);
        float z = r * cos(g_CameraPhi) * cos(g_CameraTheta);
        float x = r * cos(g_CameraPhi) * sin(g_CameraTheta);
        // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
        // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::vec4 camera_position_c;
        glm::vec4 camera_view_vector; // Vetor "view", sentido para onde a câmera está virada
        // printa o vetor deslocar
        // printf("Deslocar: (%f, %f, %f)\n", deslocar.x, deslocar.y, deslocar.z);
        Leon.direction = glm::vec4(x, -y, z, 0.0f);                     // Vetor "view", sentido para onde o player está virado
        glm::vec4 camera_up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)
        double timenow = glfwGetTime();
        if (first_person_view == false)
        {
            camera_position_c = Leon.position + glm::vec4(0.0f, 1.5f, 0.0f, 0.0f) - Leon.direction;
            if (camera_position_c.y <= 0.05f)
                camera_position_c.y = 0.05f;
            camera_view_vector = normalize(Leon.position + glm::vec4(0.0f, 1.5f, 0.0f, 0.0f) - camera_position_c);
        }
        else
        {
            camera_position_c = Leon.position + glm::vec4(0.0f, 2.0f, 0.0f, 0.0f); // Posição da câmera em primeira pessoa
            camera_view_vector = Leon.direction;
        }
        Leon.view_frente = -camera_view_vector / norm(camera_view_vector);
        Leon.view_lado = crossproduct(camera_up_vector, Leon.view_frente) / norm(crossproduct(camera_up_vector, Leon.view_frente));
        Leon.view_frente.y = 0.0f;                                    // Forçamos o vetor "view_frente" a ser paralelo ao plano XZ
        Leon.view_frente = Leon.view_frente / norm(Leon.view_frente); // Normalizamos o vetor "view_frente"
        glm::vec4 timenowvec = glm::vec4(timenow, timenow, timenow, 0.0f);
        double timedif = timenowvec.x - timeprevec.x;
        if (g_muzzleFlashActive)
        {
            g_muzzleFlashTimer -= timedif;
            if (g_muzzleFlashTimer <= 0.0f)
            {
                g_muzzleFlashActive = false;
            }
        }
        if (!paused)
        {
            if (g_LeftMouseButtonPressed && !Leon.shooting && shootinganim <= 0.1f && !Leon.reloading)
            {
                if (Leon.ammo == 0){
                    Leon.reloading = true;
                    ma_sound_start(&deaglereload);
                }
                else
                {
                    glm::vec4 shotdir = Leon.direction;
                    Leon.ammo--;
                    for (int i = 0; i < NUM_ENEMIES; i++)
                    {
                        if (enemies[i].alive)
                        {
                            if (enemies[i].boundingBox.colideWithRay(shotdir, Leon.position, enemies[i].position))
                            {
                                enemies[i].health -= 3;
                                if(enemies[i].health <= 0)
                                    enemies[i].alive = false;
                            }
                        }
                    }
                    ma_sound_start(&deagleshot[Leon.ammo]);
                    Leon.shooting = true;
                    g_LeftMouseButtonPressed = false;
                }
                g_muzzleFlashActive = true;
                g_muzzleFlashTimer = 0.1f;
            }
            if (Leon.shooting)
            {
                shootinganim += 20 * timedif;
                if (shootinganim >= 1.0f)
                    Leon.shooting = false;
            }
            if (Leon.reloading)
            {
                reloadinganim += 10 * timedif;
                if (reloadinganim >= 3.0)
                {
                    Leon.reloading = false;
                    Leon.ammo = 7;
                }
            }
            shootinganim = (shootinganim >= 0.1f) ? shootinganim - (timedif * 10) : 0.0f;
            reloadinganim = (reloadinganim >= 0.1f) ? reloadinganim - (timedif * 5) : 0.0f;
            if (w_press)
                Leon.move((timenowvec - timeprevec), 'F', gravity);
            if (a_press)
                Leon.move((timenowvec - timeprevec), 'L', gravity);
            if (s_press)
                Leon.move((timenowvec - timeprevec), 'B', gravity);
            if (d_press)
                Leon.move((timenowvec - timeprevec), 'R', gravity);
            Leon.jump(gravity, (timenowvec - timeprevec), space_press);
            space_press = false;
            if (shift_press)
            {
                Leon.running = true;
            }
            else
            {
                Leon.running = false;
            }
            Leon.run((timenowvec - timeprevec));

            for (int i = 0; i < NUM_ENEMIES; i++)
            {
                if (enemies[i].alive)
                {
                    enemies[i].player_spot(Leon.position);
                    enemies[i].aggressive_direction(Leon.position);
                    bool canmove = true;
                    for (int j = 0; j < NUM_TREES; j++)
                    {
                        if (enemies[i].boundingBox.colideWithCube(TREEBOX, enemies[i].position + enemies[i].direction, glm::vec4(tree_positions[j].first, 0.0f, tree_positions[j].second, 1.0f)))
                            canmove = false;
                    }
                    for (int j = 0; j < NUM_LOGS; j++)
                    {
                        if (enemies[i].boundingBox.colideWithCube(LOGBOX, enemies[i].position + enemies[i].direction, glm::vec4(log_positions[j].first, 0.0f, log_positions[j].second, 1.0f)))
                            canmove = false;
                    }
                    if (canmove)
                        enemies[i].move(timedif);
                    if (enemies[i].boundingBox.colideWithCube(HUMANOIDBOX, enemies[i].position, Leon.position))
                    {
                        printf("AIAIAIAIAIAI ME MORDEU\n");
                        glfwSetWindowShouldClose(window, GL_TRUE);
                    }
                }
            }
            for(int i = 0; i < NUM_BUNNIES; i++){
                if(!bunnies[i].taken){
                    if(bunny_BBOX.colideWithPoint(bunnies[i].cur_position, Leon.position))
                    {
                        printf("Pegou Coelho %d!\n", i);
                        bunnies[i].taken = true;
                        amountbunniestaken++;
                    }
                }
            }
            if(amountbunniestaken == NUM_BUNNIES){
                printf("PARABENS, VOCE PEGOU TODOS COELHOS! GASTAL ESTA ORGULHOSO!\n");
                glfwSetWindowShouldClose(window, GL_TRUE);
            }
            bool collision[3] = {false, false, false};
            for (int i = 0; i < NUM_TREES; i++)
            {
                glm::vec4 treepos = glm::vec4(tree_positions[i].first, 0.0f, tree_positions[i].second, 1.0f);
                if (tree_BBOX.colideWithPoint(treepos, glm::vec4(Leon.deslocar.x, Leon.position.y, Leon.position.z, 1.0f)))
                    collision[0] = true;
                if (tree_BBOX.colideWithPoint(treepos, glm::vec4(Leon.position.x, Leon.deslocar.y, Leon.position.z, 1.0f)))
                    collision[1] = true;
                if (tree_BBOX.colideWithPoint(treepos, glm::vec4(Leon.position.x, Leon.position.y, Leon.deslocar.z, 1.0f)))
                    collision[2] = true;
            }
            for (int i = 0; i < NUM_LOGS; i++)
            {
                glm::vec4 logpos = glm::vec4(log_positions[i].first, 0.0f, log_positions[i].second, 1.0f);
                if (log_BBOX.colideWithPoint(logpos, glm::vec4(Leon.deslocar.x, Leon.position.y, Leon.position.z, 1.0f)))
                    collision[0] = true;
                if (log_BBOX.colideWithPoint(logpos, glm::vec4(Leon.position.x, Leon.deslocar.y, Leon.position.z, 1.0f)))
                    collision[1] = true;
                if (log_BBOX.colideWithPoint(logpos, glm::vec4(Leon.position.x, Leon.position.y, Leon.deslocar.z, 1.0f)))
                    collision[2] = true;
            }

            Leon.deslocar.x = (collision[0]) ? Leon.position.x : Leon.deslocar.x;
            Leon.deslocar.y = (collision[1]) ? Leon.position.y : Leon.deslocar.y;
            Leon.deslocar.z = (collision[2]) ? Leon.position.z : Leon.deslocar.z;
            Leon.updatePosition(gravity, (timenowvec - timeprevec));
        }
        if (lastpaused != paused)
        {
            if (paused)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                printf("Pausado.\n");
            }
            else
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                printf("Despausado.\n");
            }
            lastpaused = paused;
        }
        timeprevec = timenowvec;

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;
        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane = -50.0f; // Posição do "far plane"
        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f * g_CameraDistance / 2.5f;
            float b = -t;
            float r = t * g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));
#define GOURAUD 0
#define PHONG 1

#define SPHERE 0
#define BUNNY 1
#define PLANE 2
#define WINE 3
#define GUN 4
#define LEON 5
#define MALEZOMBIE 6
#define FEMALEZOMBIE 7
#define WOOD 8
#define TREE 9
#define HOUSE 10
#define FENCE 11


        glUniform1i(g_shading_model_uniform, PHONG);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, rocky_terrain_id);
        // Desenhamos o plano do chão
        model = Matrix_Translate(0.0f, 0.0f, 0.0f) * Matrix_Scale(100.0f, 1.0f, 100.0f); // Translação e escala do plano
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PLANE);
        DrawVirtualObject("the_plane");

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, earth_id);

        for (int i = 0; i < NUM_BUNNIES; i++)
        {
            if (!bunnies[i].taken)
            {
                if (!paused)
                {
                    bunnies[i].prev_position = bunnies[i].cur_position;
                    bunnies[i].cur_position = move_along_bezier_path(bunnies[i].bunny_path, &(bunnies[i].bunny_time), bunnies[i].bunny_speed, timedif);
                    glm::vec3 bunny_pos = glm::vec3(bunnies[i].cur_position.x, bunnies[i].cur_position.y, bunnies[i].cur_position.z);
                    if(bunny_BBOX.colideWithPlane(boundaries[0], bunny_pos) || bunny_BBOX.colideWithPlane(boundaries[1], bunny_pos) ||
                       bunny_BBOX.colideWithPlane(boundaries[2], bunny_pos) || bunny_BBOX.colideWithPlane(boundaries[3], bunny_pos))
                    {
                        // Colisão detectada!
                        // 1. Reverte a posição do coelho para a última localização segura.
                        bunnies[i].cur_position = bunnies[i].prev_position;

                        // 2. Limpa a trajetória antiga para evitar vazamento de memória.
                        clear_path(bunnies[i].bunny_path);

                        // 3. Gera uma nova trajetória aleatória a partir da posição atual.
                        bunnies[i].bunny_path = generateRandomBezierPath(bunnies[i].cur_position, 3, -3);

                        // 4. Reinicia o parâmetro de tempo para a nova trajetória.
                        bunnies[i].bunny_time = 0.0f;
                    }
                    
                }
                bunnies[i].ang = direction_angle(bunnies[i].prev_position, bunnies[i].cur_position);
                model = Matrix_Translate(bunnies[i].cur_position.x, 0.3f, bunnies[i].cur_position.z) *Matrix_Scale(0.3f, 0.3f, 0.3f) * Matrix_Rotate_Y(bunnies[i].ang + M_PI / 2);
                glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                glUniform1i(g_object_id_uniform, BUNNY);
                DrawVirtualObject("the_bunny");
            }
        }

        for (int i = 0; i < NUM_ENEMIES; i++)
        {
            char zomb[3] = {1, 1, 1};
            if (enemies[i].alive)
            {
                if (enemies[i].type == 'F')
                {
                    // FEMALE ZOMBIE
                    model = Matrix_Translate(enemies[i].position.x, enemies[i].position.y, enemies[i].position.z) * Matrix_Rotate_X(-M_PI / 2.0f) * Matrix_Rotate_Z(enemies[i].ang);
                    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                    DrawVirtualObjectMtl(zomb, sizeof(zomb), &femalezombiemodel, femalezombie_textures, FEMALEZOMBIE);
                }
                else if (enemies[i].type == 'M')
                {
                    // MALE ZOMBIE
                    model = Matrix_Translate(enemies[i].position.x, enemies[i].position.y, enemies[i].position.z) * Matrix_Rotate_X(-M_PI / 2.0f) * Matrix_Rotate_Z(enemies[i].ang);
                    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                    DrawVirtualObjectMtl(zomb, sizeof(zomb), &malezombiemodel, malezombie_textures, MALEZOMBIE);
                }
            }
        }

        for(int i = 0; i < NUM_LOGS; i++){
            glUniform1i(g_shading_model_uniform, GOURAUD);
            char woodraw[1] = {1};
            model = Matrix_Translate(log_positions[i].first, 1.0f, log_positions[i].second);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glDisable(GL_CULL_FACE);
            DrawVirtualObjectMtl(woodraw, sizeof(woodraw), &woodmodel, wood_textures, WOOD);
            glEnable(GL_CULL_FACE);
        }

        glUniform1i(g_shading_model_uniform, PHONG);
        for (int i = 0; i < NUM_TREES; i++)
        {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, tree_mask_id);
            char treedraw[1] = {1};
            model = Matrix_Translate(tree_positions[i].first, 0.0f, tree_positions[i].second) * Matrix_Scale(0.01f, 0.01f, 0.01f) * Matrix_Rotate_X(-M_PI / 2.0f);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glDisable(GL_CULL_FACE);
            DrawVirtualObjectMtl(treedraw, sizeof(treedraw), &treemodel, tree_textures, TREE);
            glEnable(GL_CULL_FACE);
        }

        char gastaldraw[1] = {1};
        model = Matrix_Scale(10.0f, 10.0f, 10.0f) * Matrix_Translate(6.0f, 1.0f, 0.0f)*Matrix_Rotate_Y(-M_PI/2);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        DrawVirtualObjectMtl(gastaldraw, sizeof(gastaldraw), &gastalmodel, gastal_textures, LEON);
        char chaindraw[3] = {1, 1, 1};
        for (int j = 0; j < 2; j++)
        {
            int variacao_x = (j % 2) ? 0 : 1;
            int variacao_z = (j % 2) ? 1 : 0;
            for (int i = 1 - j; i < 51 - j; i++)
            {
                model = Matrix_Scale(1.0f, 1.0f, 1.0f) * Matrix_Translate(50.0f - (i * variacao_x) * 2, 0.0f, 50.0f - (i * variacao_z) * 2) * Matrix_Rotate_Y(j * M_PI / 2 + M_PI);
                glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                DrawVirtualObjectMtl(chaindraw, sizeof(chaindraw), &chainfencemodel, chainfence_textures, FENCE);
            }
            for (int i = 1 - j; i < 51 - j; i++)
            {
                model = Matrix_Scale(1.0f, 1.0f, 1.0f) * Matrix_Translate(-50.0f + (i * variacao_x) * 2, 0.0f, -50.0f + (i * variacao_z) * 2) * Matrix_Rotate_Y(j * M_PI / 2);
                glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                DrawVirtualObjectMtl(chaindraw, sizeof(chaindraw), &chainfencemodel, chainfence_textures, FENCE);
            }
        }

        glUseProgram(g_GpuProgramSkyboxID);
        glDepthFunc(GL_LEQUAL);
        glCullFace(GL_FRONT);
        model = Matrix_Translate(0.0f, 0.0f, 0.0f) * Matrix_Scale(100.0f, 100.0f, 100.0f);
        glUniformMatrix4fv(g_skybox_view_uniform, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(g_skybox_projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(g_skybox_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        DrawVirtualObject("Sphere_Sphere");
        glUseProgram(g_GpuProgramID);
        glCullFace(GL_BACK);
        glDepthFunc(GL_LESS);

        if (!first_person_view)
        {
            // LEON
            // Arma, faca, resto do corpo, ultimo = oculos
            glActiveTexture(GL_TEXTURE0);
            // printf("Leon Position: %f %f %f\n", Leon.position.x, Leon.position.y, Leon.position.z);
            model = Matrix_Translate(Leon.position.x, Leon.position.y, Leon.position.z) * Matrix_Rotate_X(-M_PI / 2.0f) * Matrix_Rotate_Z(g_CameraTheta);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            char leon_mask[8] = {0, 0, 1, 1, 1, 1, 1, 1};
            DrawVirtualObjectMtl(leon_mask, sizeof(leon_mask), &leonmodel, leon_textures, LEON);
        }
        else
        {
            char gundraw[1] = {1};
            glClear(GL_DEPTH_BUFFER_BIT);
            glm::mat4 gun_view_matrix = Matrix_Identity();
            glUniformMatrix4fv(g_view_uniform, 1, GL_FALSE, glm::value_ptr(gun_view_matrix));
            model = Matrix_Translate(0.8f, -0.6f, -1.5f + reloadinganim) * Matrix_Rotate_Y(M_PI) * Matrix_Scale(0.4f, 0.4f, 0.4f) * Matrix_Rotate_X(-shootinganim + reloadinganim);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            DrawVirtualObjectMtl(gundraw, sizeof(gundraw), &gunmodel, gun_textures, GUN);
        }
        glUniform1i(g_point_light_active_uniform, g_muzzleFlashActive && Leon.shooting == true);
        if (g_muzzleFlashActive)
        {
            // Posição da luz: um pouco à frente da câmera.
            // Isso é uma aproximação. O ideal seria calcular a posição exata da ponta da arma.
            glm::vec4 light_pos_world = Leon.position + glm::vec4(0.0f, 2.5f, 0.0f, 0.0f);
            glUniform3f(g_point_light_pos_uniform, light_pos_world.x, light_pos_world.y, light_pos_world.z);

            // Cor da luz: um amarelo/laranja forte
            glUniform3f(g_point_light_color_uniform, 1.0f, 0.7f, 0.7f);
        }

        // Imprimimos na tela os ângulos de Euler que controlam a rotação do
        // terceiro cubo.
        TextRendering_ShowStamina(window, Leon.stamina, Leon.staminamax);
        TextRendering_ShowAmmo(window, Leon.ammo, 7);
        if (first_person_view)
        {
            TextRendering_Crosshair(window);
        }
        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        TextRendering_ShowTimer(window);
        if(glfwGetTime() < 5.0f)
            TextRendering_ShowStartObjective(window);
        else if(glfwGetTime() >= TIME_LIMIT)
        {
            printf("DEMOROU DEMAIS LERDAO, OS COELHOS FUGIRAM\n");
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        else
            TextRendering_ShowObjectiveProgress(window, amountbunniestaken);
        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
GLuint LoadTextureImage(const char *filename, int type)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if (data == NULL)
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    if (type == 0)
    {
        glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    else if (type == 1)
    {
        glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;

    return texture_id;
}

std::map<std::string, GLuint> LoadTexturesFromObjModel(ObjModel *model, std::string base_path)
{
    std::map<std::string, GLuint> texture_name_to_id;

    for (const auto &material : model->materials)
    {
        if (!material.diffuse_texname.empty() && texture_name_to_id.count(material.diffuse_texname) == 0)
        {
            std::string texpath = base_path + material.diffuse_texname;

            GLuint loaded_texture_id = LoadTextureImage(texpath.c_str(), 1);
            texture_name_to_id[material.diffuse_texname] = loaded_texture_id;
        }
        if (!material.alpha_texname.empty() && texture_name_to_id.count(material.alpha_texname) == 0)
        {
            std::string texpath = base_path + material.alpha_texname;

            GLuint loaded_texture_id = LoadTextureImage(texpath.c_str(), 1);
            texture_name_to_id[material.alpha_texname] = loaded_texture_id;
        }
    }

    return texture_name_to_id;
}
// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char *object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void *)(g_VirtualScene[object_name].first_index * sizeof(GLuint)));

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

void DrawVirtualObjectMtl(char obj_list[], int arrsize, ObjModel *model, std::map<std::string, GLuint> textures_name_to_id, int obj_num)
{
    for (int i = 0; i < arrsize; i++)
    {
        if (obj_list[i] == 1)
        {
            const auto &shape = model->shapes[i];
            int material_id = shape.mesh.material_ids.empty() ? -1 : shape.mesh.material_ids[0];
            if (material_id < 0)
                continue;
            const auto &material = model->materials[material_id];
            // Ative a textura difusa do material, se existir
            if (!material.diffuse_texname.empty())
            {
                glActiveTexture(GL_TEXTURE0);
                // Diz ao shader que TEMOS uma textura para usar
                glUniform1i(g_has_texture_uniform, 1);

                GLuint texture_id = textures_name_to_id[material.diffuse_texname];
                glBindTexture(GL_TEXTURE_2D, texture_id);
            }
            if (!material.alpha_texname.empty())
            {
                glUniform1i(g_has_texture_uniform, 2);
                glActiveTexture(GL_TEXTURE1);

                GLuint texture_id = textures_name_to_id[material.alpha_texname];
                glBindTexture(GL_TEXTURE_2D, texture_id);
            }
            if (material.alpha_texname.empty() && material.diffuse_texname.empty())
            {
                // Diz ao shader que NÃO TEMOS uma textura, então ele deve usar apenas a cor
                glUniform1i(g_has_texture_uniform, 0);
            }
            // Envie parâmetros do material para o shader
            glUniform3fv(glGetUniformLocation(g_GpuProgramID, "material_Ka"), 1, material.ambient);
            glUniform3fv(glGetUniformLocation(g_GpuProgramID, "material_Kd"), 1, material.diffuse);
            glUniform3fv(glGetUniformLocation(g_GpuProgramID, "material_Ks"), 1, material.specular);
            glUniform1f(glGetUniformLocation(g_GpuProgramID, "material_q"), material.shininess);
            glUniform1f(glGetUniformLocation(g_GpuProgramID, "material_opacity"), material.dissolve);

            // Defina o object_id para LEON
            glUniform1i(g_object_id_uniform, obj_num);

            // Desenhe o shape
            DrawVirtualObject(shape.name.c_str());
        }
    }
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");
    GLuint skybox_vertex_shader_id = LoadShader_Vertex("../../src/skybox_vertex.glsl");
    GLuint skybox_fragment_shader_id = LoadShader_Fragment("../../src/skybox_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if (g_GpuProgramID != 0)
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    if (g_GpuProgramSkyboxID != 0)
        glDeleteProgram(g_GpuProgramSkyboxID);

    g_GpuProgramSkyboxID = CreateGpuProgram(skybox_vertex_shader_id, skybox_fragment_shader_id);
    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform = glGetUniformLocation(g_GpuProgramID, "model");           // Variável da matriz "model"
    g_view_uniform = glGetUniformLocation(g_GpuProgramID, "view");             // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform = glGetUniformLocation(g_GpuProgramID, "object_id");   // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform = glGetUniformLocation(g_GpuProgramID, "bbox_max");
    g_has_texture_uniform = glGetUniformLocation(g_GpuProgramID, "u_has_texture");
    g_shading_model_uniform = glGetUniformLocation(g_GpuProgramID, "shading_model");
    g_point_light_pos_uniform = glGetUniformLocation(g_GpuProgramID, "u_point_light_pos");
    g_point_light_color_uniform = glGetUniformLocation(g_GpuProgramID, "u_point_light_color");
    g_point_light_active_uniform = glGetUniformLocation(g_GpuProgramID, "u_point_light_active");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);

    glUseProgram(0);
    // Skybox dados placa de video e variaveis
    g_skybox_model_uniform = glGetUniformLocation(g_GpuProgramSkyboxID, "model");
    g_skybox_view_uniform = glGetUniformLocation(g_GpuProgramSkyboxID, "view");
    g_skybox_projection_uniform = glGetUniformLocation(g_GpuProgramSkyboxID, "projection");
    glUseProgram(g_GpuProgramSkyboxID);
    glUniform1i(glGetUniformLocation(g_GpuProgramSkyboxID, "skybox"), 3); // Variável "skybox" em shader_skybox_fragment.glsl
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4 &M)
{
    if (g_MatrixStack.empty())
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel *model)
{
    if (!model->attrib.normals.empty())
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4 vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];
                const float vx = model->attrib.vertices[3 * idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3 * idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3 * idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx, vy, vz, 1.0);
            }

            const glm::vec4 a = vertices[0];
            const glm::vec4 b = vertices[1];
            const glm::vec4 c = vertices[2];

            const glm::vec4 n = crossproduct(b - a, c - a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3 * triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize(3 * num_vertices);

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3 * i + 0] = n.x;
        model->attrib.normals[3 * i + 1] = n.y;
        model->attrib.normals[3 * i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel *model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float> model_coefficients;
    std::vector<float> normal_coefficients;
    std::vector<float> texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval, maxval, maxval);
        glm::vec3 bbox_max = glm::vec3(minval, minval, minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];

                indices.push_back(first_index + 3 * triangle + vertex);

                const float vx = model->attrib.vertices[3 * idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3 * idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3 * idx.vertex_index + 2];
                // printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back(vx);   // X
                model_coefficients.push_back(vy);   // Y
                model_coefficients.push_back(vz);   // Z
                model_coefficients.push_back(1.0f); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if (idx.normal_index != -1)
                {
                    const float nx = model->attrib.normals[3 * idx.normal_index + 0];
                    const float ny = model->attrib.normals[3 * idx.normal_index + 1];
                    const float nz = model->attrib.normals[3 * idx.normal_index + 2];
                    normal_coefficients.push_back(nx);   // X
                    normal_coefficients.push_back(ny);   // Y
                    normal_coefficients.push_back(nz);   // Z
                    normal_coefficients.push_back(0.0f); // W
                }

                if (idx.texcoord_index != -1)
                {
                    const float u = model->attrib.texcoords[2 * idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2 * idx.texcoord_index + 1];
                    texture_coefficients.push_back(u);
                    texture_coefficients.push_back(v);
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name = model->shapes[shape].name;
        theobject.first_index = first_index;                  // Primeiro índice
        theobject.num_indices = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;              // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0;            // "(location = 0)" em "shader_vertex.glsl"
    GLint number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!normal_coefficients.empty())
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1;             // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (!texture_coefficients.empty())
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2;             // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char *filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char *filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char *filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try
    {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    }
    catch (std::exception &e)
    {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar *shader_string = str.c_str();
    const GLint shader_string_length = static_cast<GLint>(str.length());

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar *log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if (log_length != 0)
    {
        std::string output;

        if (!compiled_ok)
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete[] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if (linked_ok == GL_FALSE)
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar *log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete[] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
    // posição atual do cursor nas variáveis g_LastCursorPosX e
    // g_LastCursorPosY.  Também, setamos a variável
    // g_LeftMouseButtonPressed como true, para saber que o usuário está
    // com o botão esquerdo pressionado.
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
    if (!paused)
    {
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da câmera com os deslocamentos
        g_CameraTheta -= 0.005f * dx* sensitivity;
        g_CameraPhi += 0.005f * dy * sensitivity;

        // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = 3.141592f / 2;
        float phimin = -phimax;

        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;

        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
    else
    {
        // Se o jogo estiver pausado, não atualizamos a posição do cursor
        // para evitar que o usuário perca o controle da câmera.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
    }

    if (g_RightMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_ForearmAngleZ -= 0.01f * dx;
        g_ForearmAngleX += 0.01f * dy;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_MiddleMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_TorsoPositionX += 0.01f * dx;
        g_TorsoPositionY -= 0.01f * dy;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f * yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = 0.5;
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mod)
{
    // ======================
    // Não modifique este loop! Ele é utilizando para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    // for (int i = 0; i < 10; ++i)
    //    if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
    //        std::exit(100 + i);
    // ======================

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_W)
    {
        if (action == GLFW_PRESS)
            w_press = true;
        else if (action == GLFW_RELEASE)
            w_press = false;
    }
    if (key == GLFW_KEY_A)
    {
        if (action == GLFW_PRESS)
            a_press = true;
        else if (action == GLFW_RELEASE)
            a_press = false;
    }
    if (key == GLFW_KEY_S)
    {
        if (action == GLFW_PRESS)
            s_press = true;
        else if (action == GLFW_RELEASE)
            s_press = false;
    }
    if (key == GLFW_KEY_D)
    {
        if (action == GLFW_PRESS)
            d_press = true;
        else if (action == GLFW_RELEASE)
            d_press = false;
    }
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
    {
        paused = !paused;
    }
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        space_press = true;
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    else if (key == GLFW_KEY_P && action == GLFW_PRESS)
        g_UsePerspectiveProjection = true;
    else if (key == GLFW_KEY_O && action == GLFW_PRESS)
        g_UsePerspectiveProjection = false;
    else if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        // Recarrega os shaders, veja a função LoadShadersFromFiles() acima.
        LoadShadersFromFiles();
        fprintf(stdout, "Shaders recarregados!\n");
        fflush(stdout);
    }
    else if ((key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT))
    {
        if (action == GLFW_PRESS)
            shift_press = true;
        else if (action == GLFW_RELEASE)
            shift_press = false;
    }
    else if ((key == GLFW_KEY_V && action == GLFW_PRESS))
    {
        first_person_view = !first_person_view;
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char *description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow *window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model)
{
    if (!g_ShowInfoText)
        return;

    glm::vec4 p_world = model * p_model;
    glm::vec4 p_camera = view * p_world;
    glm::vec4 p_clip = projection * p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f - pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f - 2 * pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f - 6 * pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f - 7 * pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f - 8 * pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f - 9 * pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f - 10 * pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f - 14 * pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f - 15 * pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f - 16 * pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f - 17 * pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f - 18 * pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2(0, 0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x) / (b.x - a.x), 0.0f, 0.0f, (b.x * p.x - a.x * q.x) / (b.x - a.x),
        0.0f, (q.y - p.y) / (b.y - a.y), 0.0f, (b.y * p.y - a.y * q.y) / (b.y - a.y),
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f - 22 * pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f - 23 * pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f - 24 * pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f - 25 * pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f - 26 * pad, 1.0f);
}


// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow *window)
{
    if (!g_ShowInfoText)
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if (g_UsePerspectiveProjection)
        TextRendering_PrintString(window, "Perspective", 1.0f - 13 * charwidth, -1.0f + 2 * lineheight / 10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f - 13 * charwidth, -1.0f + 2 * lineheight / 10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow *window)
{
    if (!g_ShowInfoText)
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int ellapsed_frames = 0;
    static char buffer[20] = "?? fps";
    static int numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if (ellapsed_seconds > 1.0f)
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f - (numchars + 1) * charwidth, 1.0f - lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel *model)
{
    const tinyobj::attrib_t &attrib = model->attrib;
    const std::vector<tinyobj::shape_t> &shapes = model->shapes;
    const std::vector<tinyobj::material_t> &materials = model->materials;

    printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
    printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
    printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
    printf("# of shapes    : %d\n", (int)shapes.size());
    printf("# of materials : %d\n", (int)materials.size());

    for (size_t v = 0; v < attrib.vertices.size() / 3; v++)
    {
        printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
               static_cast<const double>(attrib.vertices[3 * v + 0]),
               static_cast<const double>(attrib.vertices[3 * v + 1]),
               static_cast<const double>(attrib.vertices[3 * v + 2]));
    }

    for (size_t v = 0; v < attrib.normals.size() / 3; v++)
    {
        printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
               static_cast<const double>(attrib.normals[3 * v + 0]),
               static_cast<const double>(attrib.normals[3 * v + 1]),
               static_cast<const double>(attrib.normals[3 * v + 2]));
    }

    for (size_t v = 0; v < attrib.texcoords.size() / 2; v++)
    {
        printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
               static_cast<const double>(attrib.texcoords[2 * v + 0]),
               static_cast<const double>(attrib.texcoords[2 * v + 1]));
    }

    // For each shape
    for (size_t i = 0; i < shapes.size(); i++)
    {
        printf("shape[%ld].name = %s\n", static_cast<long>(i),
               shapes[i].name.c_str());
        printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
               static_cast<unsigned long>(shapes[i].mesh.indices.size()));

        size_t index_offset = 0;

        assert(shapes[i].mesh.num_face_vertices.size() ==
               shapes[i].mesh.material_ids.size());

        printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
               static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

        // For each face
        for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++)
        {
            size_t fnum = shapes[i].mesh.num_face_vertices[f];

            printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
                   static_cast<unsigned long>(fnum));

            // For each vertex in the face
            for (size_t v = 0; v < fnum; v++)
            {
                tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
                printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
                       static_cast<long>(v), idx.vertex_index, idx.normal_index,
                       idx.texcoord_index);
            }

            printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
                   shapes[i].mesh.material_ids[f]);

            index_offset += fnum;
        }

        printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
               static_cast<unsigned long>(shapes[i].mesh.tags.size()));
        for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++)
        {
            printf("  tag[%ld] = %s ", static_cast<long>(t),
                   shapes[i].mesh.tags[t].name.c_str());
            printf(" ints: [");
            for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j)
            {
                printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
                if (j < (shapes[i].mesh.tags[t].intValues.size() - 1))
                {
                    printf(", ");
                }
            }
            printf("]");

            printf(" floats: [");
            for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j)
            {
                printf("%f", static_cast<const double>(
                                 shapes[i].mesh.tags[t].floatValues[j]));
                if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1))
                {
                    printf(", ");
                }
            }
            printf("]");

            printf(" strings: [");
            for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j)
            {
                printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
                if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1))
                {
                    printf(", ");
                }
            }
            printf("]");
            printf("\n");
        }
    }

    for (size_t i = 0; i < materials.size(); i++)
    {
        printf("material[%ld].name = %s\n", static_cast<long>(i),
               materials[i].name.c_str());
        printf("  material.Ka = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].ambient[0]),
               static_cast<const double>(materials[i].ambient[1]),
               static_cast<const double>(materials[i].ambient[2]));
        printf("  material.Kd = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].diffuse[0]),
               static_cast<const double>(materials[i].diffuse[1]),
               static_cast<const double>(materials[i].diffuse[2]));
        printf("  material.Ks = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].specular[0]),
               static_cast<const double>(materials[i].specular[1]),
               static_cast<const double>(materials[i].specular[2]));
        printf("  material.Tr = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].transmittance[0]),
               static_cast<const double>(materials[i].transmittance[1]),
               static_cast<const double>(materials[i].transmittance[2]));
        printf("  material.Ke = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].emission[0]),
               static_cast<const double>(materials[i].emission[1]),
               static_cast<const double>(materials[i].emission[2]));
        printf("  material.Ns = %f\n",
               static_cast<const double>(materials[i].shininess));
        printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
        printf("  material.dissolve = %f\n",
               static_cast<const double>(materials[i].dissolve));
        printf("  material.illum = %d\n", materials[i].illum);
        printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
        printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
        printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
        printf("  material.map_Ns = %s\n",
               materials[i].specular_highlight_texname.c_str());
        printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
        printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
        printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
        printf("  <<PBR>>\n");
        printf("  material.Pr     = %f\n", materials[i].roughness);
        printf("  material.Pm     = %f\n", materials[i].metallic);
        printf("  material.Ps     = %f\n", materials[i].sheen);
        printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
        printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
        printf("  material.aniso  = %f\n", materials[i].anisotropy);
        printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
        printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
        printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
        printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
        printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
        printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
        std::map<std::string, std::string>::const_iterator it(
            materials[i].unknown_parameter.begin());
        std::map<std::string, std::string>::const_iterator itEnd(
            materials[i].unknown_parameter.end());

        for (; it != itEnd; it++)
        {
            printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
        }
        printf("\n");
    }
}

void TextRendering_ShowStamina(GLFWwindow *window, float stamina, float maxstamina)
{
    if (!g_ShowInfoText)
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Stamina: %.2f/%.2f\n", stamina, maxstamina);

    TextRendering_PrintString(window, buffer, -1.0f + pad / 10, -1.0f + 2 * pad / 10, 1.0f);
}

void TextRendering_ShowAmmo(GLFWwindow *window, int ammo, int maxammo)
{
    if (!g_ShowInfoText)
        return;

    float pad = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    char buffer[80];
    snprintf(buffer, 80, "Ammo: %d/%d\n", ammo, maxammo);

    TextRendering_PrintString(window, buffer, -1.0f + 25 * charwidth, -1.0f + 2 * pad / 10, 1.0f);
}

void TextRendering_Crosshair(GLFWwindow *window)
{
    float scale = 1.5f;

    float char_width = TextRendering_CharWidth(window) * scale;
    float char_height = TextRendering_LineHeight(window) * scale;

    TextRendering_PrintString(window, "+", -char_width / 2.0f, -char_height / 2.0f, scale);
}

void TextRendering_ShowTimer(GLFWwindow *window){

    if (!g_ShowInfoText)
        return;

    float charwidth = TextRendering_CharWidth(window);
    float pad = TextRendering_LineHeight(window);
    char buffer[50];
    snprintf(buffer, 50, "Tempo restante: %.2f\n", TIME_LIMIT - glfwGetTime());
    TextRendering_PrintString(window, buffer, 0.0f - (double)strlen(buffer)*charwidth/2, 1.0f - pad, 1.0f);

}

void TextRendering_ShowStartObjective(GLFWwindow *window){

    if (!g_ShowInfoText)
        return;

    float scale = 2.50f;
    float charwidth = TextRendering_CharWidth(window)*scale;
    float pad = TextRendering_LineHeight(window)*scale;
    char buffer[30];
    snprintf(buffer, 30, "Colete todos os coelinhos\n", glfwGetTime());
    TextRendering_PrintString(window, buffer, -(double)strlen(buffer)*charwidth/2, -pad/2, scale);
}

void TextRendering_ShowObjectiveProgress(GLFWwindow *window, int coelhinhos){
    if (!g_ShowInfoText)
    return;

    float charwidth = TextRendering_CharWidth(window);
    float pad = TextRendering_LineHeight(window);
    char buffer[30];
    snprintf(buffer, 30, "Coelinhos %d/%d\n", coelhinhos, NUM_BUNNIES);
    TextRendering_PrintString(window, buffer, -1.0f,1.0f - pad);
}
std::vector<Enemie> generateEnemies(int map_occupation[MAP_LENGTH][MAP_LENGTH]){
    #define HP 5.0f
    std::vector<struct Enemie> enemies;
    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        float posx = 0;
        float posz = 0;
        int j = 0;
        do
        {
            posx = ((rand() % (X_MAX - X_MIN + 1)) + X_MIN) * (rand() % 2 == 0 ? 1 : -1); // Gera um número aleatório entre X_MIN e X_MAX, podendo ser negativo
            posz = ((rand() % (Z_MAX - Z_MIN + 1)) + Z_MIN) * (rand() % 2 == 0 ? 1 : -1); // Gera um número aleatório entre Z_MIN e Z_MAX, podendo ser negativo
            j++;
        } while (map_occupation[(int)(posx + MAP_LENGTH / 2)][(int)(posz + MAP_LENGTH / 2)] != 0); // Verifica se a posição já está ocupada por outro objeto
        map_occupation[(int)(posx + MAP_LENGTH / 2)][(int)(posz + MAP_LENGTH / 2)] = 1; // Marca a posição como ocupada
        for (int x = -3; x <= 3; x++)
        {
            for (int z = -3; z <= 3; z++)
            {
                if (x != 0 || z != 0)
                {
                    int arr_posx = (int)posx + MAP_LENGTH / 2 + x;
                    int arr_posz = (int)posz + MAP_LENGTH / 2 + z;
                    if (arr_posx >= 0 && arr_posx < MAP_LENGTH && arr_posz >= 0 && arr_posz < MAP_LENGTH)
                        map_occupation[(int)(posx + MAP_LENGTH / 2) + x][(int)(posz + MAP_LENGTH / 2) + z] = 1; // Marca as posições ao redor como ocupadas
                }
            }
        }
        if (i % 2 == 0)
        {
            enemies.push_back(Enemie(glm::vec4(posx, 0.0f, posz, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), 1.0f + rand()%5, HP, 'M', HUMANOIDBOX));
        }
        else
        {
            enemies.push_back(Enemie(glm::vec4(posx, 0.0f, posz, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), 1.0f + rand()%5, HP, 'F', HUMANOIDBOX));
        }
        enemies[i].cur_path = generateRandomBezierPath(enemies[i].position, 3, -3);
        enemies[i].prev_position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        enemies[i].time = 0.0f;
        enemies[i].ang = 0.0;

    }
    return enemies;
}

std::vector<std::pair<float, float>> generateStaticObj(int map_occupation[MAP_LENGTH][MAP_LENGTH], int object_quant){
    std::vector<std::pair<float, float>> tree_positions;
    for (int i = 0; i < object_quant; i++)
    {
        float posx = 0;
        float posz = 0;
        int j = 0;
        do
        {
            posx = ((rand() % (X_MAX - X_MIN + 1)) + X_MIN) * (rand() % 2 == 0 ? 1 : -1); // Gera um número aleatório entre X_MIN e X_MAX, podendo ser negativo
            posz = ((rand() % (Z_MAX - Z_MIN + 1)) + Z_MIN) * (rand() % 2 == 0 ? 1 : -1); // Gera um número aleatório entre Z_MIN e Z_MAX, podendo ser negativo
            j++;
        } while (map_occupation[(int)(posx + MAP_LENGTH / 2)][(int)(posz + MAP_LENGTH / 2)] != 0); // Verifica se a posição já está ocupada por outro objeto
        // ocupa as posições envolta da arvore
        for (int x = -3; x <= 3; x++)
        {
            for (int z = -3; z <= 3; z++)
            {
                if (x != 0 || z != 0)
                {
                    int arr_posx = (int)posx + MAP_LENGTH / 2 + x;
                    int arr_posz = (int)posz + MAP_LENGTH / 2 + z;
                    if (arr_posx >= 0 && arr_posx < MAP_LENGTH && arr_posz >= 0 && arr_posz < MAP_LENGTH)
                        map_occupation[(int)(posx + MAP_LENGTH / 2) + x][(int)(posz + MAP_LENGTH / 2) + z] = 1; // Marca as posições ao redor como ocupadas
                }
            }
        }
        tree_positions.push_back(std::make_pair(posx, posz)); // Adiciona a posição da árvore na lista de posições
    }
    return tree_positions;
}

std::vector<Bunny> generateBunnies(int map_occupation[MAP_LENGTH][MAP_LENGTH]){ 
    std::vector<Bunny> bunnies;
    for (int i = 0; i < NUM_BUNNIES; i++)
    {
        float posx = 0;
        float posz = 0;
        int j = 0;
        do
        {
            posx = ((rand() % (X_MAX - X_MIN + 1)) + X_MIN) * (rand() % 2 == 0 ? 1 : -1); // Gera um número aleatório entre X_MIN e X_MAX, podendo ser negativo
            posz = ((rand() % (Z_MAX - Z_MIN + 1)) + Z_MIN) * (rand() % 2 == 0 ? 1 : -1); // Gera um número aleatório entre Z_MIN e Z_MAX, podendo ser negativo
            j++;
        } while (map_occupation[(int)(posx + MAP_LENGTH / 2)][(int)(posz + MAP_LENGTH / 2)] != 0); // Verifica se a posição já está ocupada por outro objeto

        for (int x = -3; x <= 3; x++)
        {
            for (int z = -3; z <= 3; z++)
            {
                if (x != 0 || z != 0)
                {
                    int arr_posx = (int)posx + MAP_LENGTH / 2 + x;
                    int arr_posz = (int)posz + MAP_LENGTH / 2 + z;
                    if (arr_posx >= 0 && arr_posx < MAP_LENGTH && arr_posz >= 0 && arr_posz < MAP_LENGTH)
                        map_occupation[(int)(posx + MAP_LENGTH / 2) + x][(int)(posz + MAP_LENGTH / 2) + z] = 1; // Marca as posições ao redor como ocupadas
                }
            }
        }
        Bunny cur_bunny;
        cur_bunny.taken = false;
        cur_bunny.cur_position = glm::vec4(posx, 0.0f, posz, 1.0f);
        cur_bunny.bunny_path = generateRandomBezierPath(cur_bunny.cur_position, 3, -3);
        cur_bunny.prev_position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        cur_bunny.bunny_time = 0.0f;
        cur_bunny.ang = 0.0f;
        cur_bunny.bunny_speed = (1 + rand() % BUNNY_MAX_SPEED) / 100.0f;
        bunnies.push_back(cur_bunny);
    }
    return bunnies;
}
