//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   LABORATÓRIO 5
//
//      Alunos: Bernardo Hummes
//              Guilherme Bazzo
//

#include <cmath>
#include <cstdio>
#include <cstdlib>

#define WIDTH 800
#define HEIGHT 600
#define WINDOW_TITLE "FCG Tour"

#define FREE_CAMERA 1
#define LOOK_AT_CAMERA 2

#define ERRO_COLISAO 0.1f

#define M_PI   3.14159265358979323846

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

#include <iostream>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

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

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj".
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    ObjModel(const char* filename, const char* basepath = "../../data/", bool triangulate = true)
    {
        printf("Carregando modelo \"%s\"... ", filename);

        char filepath[100];
        strcpy(filepath, filename);
        strcat(filepath, ".obj");

        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filepath, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        printf("OK.\n");
    }
};


// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

template <typename T> int sgn(T val);
bool check_inside_museum(float x, float z);
float F_p1_p2(glm::vec3 v, glm::vec3 a, float x, float z);

float absolute_float(float v);
glm::vec4 bezier(float t, glm::vec4 p1, glm::vec4 p2, glm::vec4 p3, glm::vec4 p4);
bool interseccao_caixa_caixa(struct box_obj obj1, struct box_obj obj2);
bool interseccao_esfera_esfera(struct sphere_obj obj1, struct sphere_obj obj2);
bool interseccao_caixa_esfera(struct box_obj caixa, struct sphere_obj esfera);
bool interseccao_caixa_plano(struct box_obj caixa, struct plane_obj plano);
bool interseccao_esfera_plano(struct sphere_obj esfera, struct plane_obj plano);
bool check_colision(float x, float z);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

//estande 2
float g_Angle_Stand2 = 0.0f;
float g_aux_Stand2 = 0.0f;

// definições das informações do objeto no estande 5
float g_AngleX_5 = 0.0f;
float g_AngleY_5 = 0.0f;
float g_AngleZ_5 = 0.0f;

float g_posX_5 = 0.0f;
float g_posY_5 = 0.0f;
float g_posZ_5 = 0.0f;

float g_scaleX_5 = 0.5f;
float g_scaleY_5 = 0.5f;
float g_scaleZ_5 = 0.5f;

// definições dos pontos para a curva do estande 9
float p1X_9 = 0.1f;
float p1Y_9 = 0.1f;
float p1Z_9 = 0.1f;
float p2X_9 = 0.5f;
float p2Y_9 = 0.5f;
float p2Z_9 = 0.1f;
float p3X_9 = 1.3f;
float p3Y_9 = 1.3f;
float p3Z_9 = 0.1f;
float p4X_9 = 1.4f;
float p4Y_9 = 1.4f;
float p4Z_9 = 0.1f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 1.58f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.2f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.0f; // Distância da câmera para a origem

// Variáveis que salvarão o estado atual da FREE CAMERA, assim, caso mudemos
// para a LOOK_AT_CAMERA podemos voltar para onde estávamos no espaço com o
// o mesmo ponto de visão
float CameraDistance_save = g_CameraDistance;
float CameraPhi_FC_save = g_CameraPhi;
float CameraTheta_FC_save = g_CameraTheta;

// Inicializa distância da câmera ao ponto (0,0,0) global
float g_CamDistanceX = -1.0f;
float g_CamDistanceY = 1.0f;
float g_CamDistanceZ = 0.0f;

// flags para teclas walkaround
int pressedW =0, pressedS=0, pressedA=0, pressedD=0;

// Tipo de camera
int camera_view_ID = FREE_CAMERA;

float y;
float z;
float x;

#define QUANT_ESTANDE 20
std::vector<glm::vec4> posicoes_estandes;
int estande_atual = 0;

int opcao_estande1 = 0;

int cor_lampada = 1;

int direcao_textura_plana = 1;

int obj_atual_stand18 = 1;
float cai_obj1 = 0.0f;
float cai_obj2 = 0.0f;
float cai_obj3 = 0.0f;
float cai_obj4 = 0.0f;
float cai_obj5 = 0.0f;

struct square_bbox{
    glm::vec3   p1;
    glm::vec3   p2;
    glm::vec3   p3;
    glm::vec3   p4;
};

struct box_obj {
    glm::vec3   c; // center
    float       x_size; // distance from center to x
    float       y_size; // distance from center to y
    float       z_size; // distance from center to z
};

struct sphere_obj {
    glm::vec3   c; // center
    float       r; // radius
};

struct plane_obj {
    glm::vec3   c; // center
    float       x_size; // distance from center to x
    float       z_size; // distance from center to z
};


struct square_bbox Museu;
std::vector<square_bbox> estandes_bbox;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint program_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint projection_uniform;
GLint object_id_uniform;
GLint bbox_min_uniform;
GLint bbox_max_uniform;
GLint estande_shader;
GLint acerto_ou_erro_est1;
GLint cor_lampada_shader;
GLint direcao_textura_plana_shader;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

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
    GLFWwindow* window;
    window = glfwCreateWindow(WIDTH, HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

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
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 217-219 do documento "Aula_03_Rendering_Pipeline_Grafico.pdf".
    //
    LoadShadersFromFiles();

    std::vector<const char*> object_names = {"museu", "estande", "triceratop", "triangulo", "cow", "esfera", "cubo", "rosquinha_1", "rosquinha_2", "lampada", "chaleira", "plano_gc_real", "vetor", "plano"};
    std::vector<const char*>::iterator iterator_obj_names ;

    const char* basepath = "../../data/";

    for (iterator_obj_names = object_names.begin(); iterator_obj_names != object_names.end(); iterator_obj_names++){
        char filepath[100];
        strcpy(filepath, basepath);
        strcat(filepath, *iterator_obj_names);

        LoadTextureImage(filepath);

        if(*iterator_obj_names == "estande"){
            LoadTextureImage("../../data/estande_acerto");
            LoadTextureImage("../../data/estande_erro");
        } else if(*iterator_obj_names == "lampada"){
            LoadTextureImage("../../data/vermelho");
            LoadTextureImage("../../data/azul");
            LoadTextureImage("../../data/verde");
            LoadTextureImage("../../data/rosa");
            LoadTextureImage("../../data/amarelo");
        }

        ObjModel obj_model(filepath, basepath);
        ComputeNormals(&obj_model);
        BuildTrianglesAndAddToVirtualScene(&obj_model);
    }


    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slide 108 do documento "Aula_09_Projecoes.pdf".
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 22-34 do documento "Aula_13_Clipping_and_Culling.pdf".
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Variáveis auxiliares utilizadas para chamada à função
    // TextRendering_ShowModelViewProjection(), armazenando matrizes 4x4.
    glm::mat4 the_projection;
    glm::mat4 the_model;
    glm::mat4 the_view;

    // Valor inicial do tempo
    double time_prev = glfwGetTime();
    double time_now;
    double passo_tempo;

    // Ficamos em loop, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operações de renderização

        // Controle do tempo no movimento para reposicionamento da câmera com WASD keys
        time_now = glfwGetTime();
        passo_tempo = (time_now - time_prev);
        double passo_camera = passo_tempo*4.0f;
        time_prev = time_now;

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
        glUseProgram(program_id);

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().
        float r = g_CameraDistance;

        // Cálculos trigonométricos salvos para economizar recursos computacionais
        float sin_g_CameraPhi = sin(g_CameraPhi);
        float cos_g_CameraPhi = cos(g_CameraPhi);
        float sin_g_CameraTheta = sin(g_CameraTheta);
        float cos_g_CameraTheta = cos(g_CameraTheta);

        // Coordenadas do view vector
        y = r*sin_g_CameraPhi;
        z = r*cos_g_CameraPhi*cos_g_CameraTheta;
        x = r*cos_g_CameraPhi*sin_g_CameraTheta;

        glm::vec4 camera_position_c;
        glm::vec4 camera_lookat_l;
        glm::vec4 camera_view_vector;
        glm::vec4 camera_up_vector;

        // FREE CAMERA
        if(camera_view_ID == FREE_CAMERA){

            CameraDistance_save = g_CameraDistance;
            CameraPhi_FC_save = g_CameraPhi;
            CameraTheta_FC_save = g_CameraTheta;

            float new_z = 0.0f;
            float new_x = 0.0f;

            if (pressedW)
            {
                //g_CamDistanceY -= (passo_camera*sin_g_CameraPhi);
                new_z = g_CamDistanceZ - (passo_camera * cos_g_CameraTheta);
                new_x = g_CamDistanceX - (passo_camera * sin_g_CameraTheta);
                if (check_colision(new_x, new_z)){
                    g_CamDistanceZ = new_z;
                    g_CamDistanceX = new_x;
                }
            }

            if (pressedS)
            {
                //g_CamDistanceY += (passo_camera*sin_g_CameraPhi);
                new_z = g_CamDistanceZ + (passo_camera * cos_g_CameraTheta);
                new_x = g_CamDistanceX + (passo_camera * sin_g_CameraTheta);
                if (check_colision(new_x, new_z)){
                    g_CamDistanceZ = new_z;
                    g_CamDistanceX = new_x;
                }
            }

            if (pressedA)
            {
                new_x = g_CamDistanceX - (passo_camera * cos_g_CameraTheta);
                new_z = g_CamDistanceZ + (passo_camera * sin_g_CameraTheta);
                if (check_colision(new_x, new_z)){
                    g_CamDistanceZ = new_z;
                    g_CamDistanceX = new_x;
                }
            }

            if (pressedD)
            {
                new_x = g_CamDistanceX + (passo_camera * cos_g_CameraTheta);
                new_z = g_CamDistanceZ - (passo_camera * sin_g_CameraTheta);
                if (check_colision(new_x, new_z)){
                    g_CamDistanceZ = new_z;
                    g_CamDistanceX = new_x;
                }
            }

            camera_position_c  = glm::vec4(g_CamDistanceX,g_CamDistanceY,g_CamDistanceZ,1.0f);
            //printf("x: %f; z: %f\n", g_CamDistanceX, g_CamDistanceZ);
            camera_view_vector = glm::vec4(-x, -y, -z, 0.0);

        // LOOK AT CAMERA
        } else {
            // Ponto "c", centro da câmera
            camera_position_c  = glm::vec4(posicoes_estandes[estande_atual].x, y + 0.5f, posicoes_estandes[estande_atual].z - (2.2f*sgn(posicoes_estandes[estande_atual].z)),1.0f);
            // Ponto "l", onde câmera está olhando
            camera_lookat_l    = glm::vec4(posicoes_estandes[estande_atual].x, posicoes_estandes[estande_atual].y + 3.2f, posicoes_estandes[estande_atual].z, 1.0f);
            // Vetor "view", sentido para onde a câmera está virada
            camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
            //printf("x = %f, y = %f, z = %f\n", posicoes_estandes[estande_atual].x,posicoes_estandes[estande_atual].y,posicoes_estandes[estande_atual].z );
        }

        camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)

        // Computamos a matriz "View" utilizando os parâmetros da câmera para definir o sistema de coordenadas da câmera.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far estão no sentido negativo!
        float nearplane = -0.05f;  // Posição do "near plane"
        float farplane  = -60.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slide 227 do documento "Aula_09_Projecoes.pdf".
            float field_of_view = M_PI / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slide 236 do documento "Aula_09_Projecoes.pdf".
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));


        glUniform1i(estande_shader, estande_atual);
        glUniform1i(acerto_ou_erro_est1, opcao_estande1);
        glUniform1i(cor_lampada_shader, cor_lampada);
        glUniform1i(direcao_textura_plana_shader, direcao_textura_plana);


        #define MUSEU 0
        #define ESTANDE 1
        #define DINOSSAURO 2
        #define TRIANGULO 3
        #define VACA 4
        #define ESFERA 5
        #define CUBO 6
        #define ROSQUINHA_1 7
        #define ROSQUINHA_2 8
        #define LAMPADA 9
        #define CHALEIRA_PLANA 10
        #define CHALEIRA_CUBICA 11
        #define CHALEIRA_ESFERICA 12
        #define CHALEIRA_CILINDRICA 13
        #define PLANO_GC_REAL 14
        #define VETOR_ESTATICO 15
        #define VETOR_MOVE 16
        #define VETOR_RESULTANTE 17
        #define PLANO 18

        glm::vec3 obj_min;
        glm::vec3 obj_max;
        glm::vec4 obj_min_vec4;
        glm::vec4 obj_max_vec4;
        glm::vec4 posMin;
        glm::vec4 posMax;


        model = Matrix_Translate(-22.0f, 1.0f, 0.0f)
              * Matrix_Scale(25.0f, 6.0f, 12.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, MUSEU);
        DrawVirtualObject("museu");

        glm::vec3 museu_min = g_VirtualScene["museu"].bbox_min;
        glm::vec3 museu_max = g_VirtualScene["museu"].bbox_max;
        glm::vec4 museu_min_vec4 = glm::vec4(museu_min.x, museu_min.y, museu_min.z, 1.0f);
        glm::vec4 museu_max_vec4 = glm::vec4(museu_max.x, museu_max.y, museu_max.z, 1.0f);

        posMin = model * museu_min_vec4;
        posMax = model * museu_max_vec4;

        Museu.p1 = glm::vec3(posMin.x + ERRO_COLISAO, 1.0f, posMin.z + ERRO_COLISAO);
        Museu.p2 = glm::vec3(posMax.x - ERRO_COLISAO, 1.0f, posMin.z + ERRO_COLISAO);
        Museu.p3 = glm::vec3(posMax.x - ERRO_COLISAO, 1.0f, posMax.z - ERRO_COLISAO);
        Museu.p4 = glm::vec3(posMin.x + ERRO_COLISAO, 1.0f, posMax.z - ERRO_COLISAO);


        for (float estandes = 0; estandes<10*4; estandes+=4){
            model = Matrix_Translate(-1.2f*estandes, -4.8f, -11.0f)
                  * Matrix_Scale(0.95f, 1.2f, 0.95f);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(object_id_uniform, ESTANDE);
            DrawVirtualObject("estande");

            posicoes_estandes.push_back(glm::vec4(-1.2f*estandes, -4.8f, -11.0f, 1.0f));

            glm::vec3 estande_min = g_VirtualScene["estande"].bbox_min;
            glm::vec3 estande_max = g_VirtualScene["estande"].bbox_max;
            glm::vec4 estande_min_vec4 = glm::vec4(estande_min.x, estande_min.y, estande_min.z, 1.0f);
            glm::vec4 estande_max_vec4 = glm::vec4(estande_max.x, estande_max.y, estande_max.z, 1.0f);

            glm::vec4 posMin = model * estande_min_vec4;
            glm::vec4 posMax = model * estande_max_vec4;

            struct square_bbox estande;

            estande.p1 = glm::vec3(posMin.x - ERRO_COLISAO, 1.0f, posMin.z - ERRO_COLISAO);
            estande.p2 = glm::vec3(posMax.x + ERRO_COLISAO, 1.0f, posMin.z - ERRO_COLISAO);
            estande.p3 = glm::vec3(posMax.x + ERRO_COLISAO, 1.0f, posMax.z + ERRO_COLISAO);
            estande.p4 = glm::vec3(posMin.x - ERRO_COLISAO, 1.0f, posMax.z + ERRO_COLISAO);

            estandes_bbox.push_back(estande);
        }

        for (float estandes = 0; estandes<10*4; estandes+=4){
            model = Matrix_Translate(-1.2f*estandes, -4.8f, 11.0f)
                  * Matrix_Scale(0.95f, 1.2f, 0.95f)
                  * Matrix_Rotate_Y(M_PI);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(object_id_uniform, ESTANDE);
            DrawVirtualObject("estande");

            posicoes_estandes.push_back(glm::vec4(-1.2f*estandes, -4.8f, 11.0f, 1.0f));

            glm::vec3 estande_min = g_VirtualScene["estande"].bbox_min;
            glm::vec3 estande_max = g_VirtualScene["estande"].bbox_max;
            glm::vec4 estande_min_vec4 = glm::vec4(estande_min.x, estande_min.y, estande_min.z, 1.0f);
            glm::vec4 estande_max_vec4 = glm::vec4(estande_max.x, estande_max.y, estande_max.z, 1.0f);

            glm::vec4 posMin = model * estande_min_vec4;
            glm::vec4 posMax = model * estande_max_vec4;

            struct square_bbox estande;

            estande.p1 = glm::vec3(posMin.x - ERRO_COLISAO, 1.0f, posMin.z - ERRO_COLISAO);
            estande.p2 = glm::vec3(posMax.x + ERRO_COLISAO, 1.0f, posMin.z - ERRO_COLISAO);
            estande.p3 = glm::vec3(posMax.x + ERRO_COLISAO, 1.0f, posMax.z + ERRO_COLISAO);
            estande.p4 = glm::vec3(posMin.x - ERRO_COLISAO, 1.0f, posMax.z + ERRO_COLISAO);

            estandes_bbox.push_back(estande);
        }

        model = Matrix_Translate(-22.0f, -5.0f, 1.0f)
              * Matrix_Scale(2.0f, 2.0f, 2.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, DINOSSAURO);
        DrawVirtualObject("triceratop");

        // estande 1
        model = Matrix_Translate(posicoes_estandes[1-1].x, posicoes_estandes[1-1].y + 3.68f, posicoes_estandes[1-1].z)
              * Matrix_Scale(0.6f, 0.6f, 0.6f)
              * Matrix_Rotate_X(0.4f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, PLANO_GC_REAL);
        DrawVirtualObject("plano_gc_real");

        // estante 2
        model = Matrix_Translate(posicoes_estandes[2-1].x - 0.45f, posicoes_estandes[2-1].y + 3.82f, posicoes_estandes[2-1].z - 0.2f)
              * Matrix_Scale(0.30f, 0.2f, 0.3f)
              * Matrix_Rotate_X(0.4f)
              * Matrix_Rotate_Y(-M_PI/2);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, VETOR_ESTATICO);
        DrawVirtualObject("vetor");

        model = Matrix_Translate(posicoes_estandes[2-1].x - 0.3f, posicoes_estandes[2-1].y + 3.84f, posicoes_estandes[2-1].z + 0.0f - g_Angle_Stand2*0.15f)
              * Matrix_Scale(0.30f, 0.2f, 0.3f)
              * Matrix_Rotate_X(0.4f)
              * Matrix_Rotate_Y(M_PI + g_Angle_Stand2);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, VETOR_MOVE);
        DrawVirtualObject("vetor");

        model = Matrix_Translate(posicoes_estandes[2-1].x - 0.33f, posicoes_estandes[2-1].y + 3.9f, posicoes_estandes[2-1].z - 0.05f - g_Angle_Stand2*0.085f)
              * Matrix_Scale(0.3f + (g_aux_Stand2*0.011), 0.4f + (g_aux_Stand2*0.015), 0.3f + (g_aux_Stand2*0.011))
              * Matrix_Rotate_X(0.2f)
              * Matrix_Rotate_Y(-(27*M_PI)/36 + (g_aux_Stand2*0.035));
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, VETOR_RESULTANTE);
        DrawVirtualObject("vetor");

        // estande 4
        model = Matrix_Translate(posicoes_estandes[4-1].x, posicoes_estandes[4-1].y + 4.2f, posicoes_estandes[4-1].z)
              * Matrix_Scale(0.2f, 0.2f, 0.2f)
              * Matrix_Rotate_X((float)glfwGetTime() * 1.5f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, TRIANGULO);
        DrawVirtualObject("triangulo");

        // estande 5
        model = Matrix_Translate(posicoes_estandes[5-1].x, posicoes_estandes[5-1].y + 4.0f, posicoes_estandes[5-1].z)
              * Matrix_Scale(g_scaleX_5, g_scaleY_5, g_scaleZ_5)
              * Matrix_Rotate_X(g_AngleX_5)
              * Matrix_Rotate_Y(g_AngleY_5)
              * Matrix_Rotate_Z(g_AngleZ_5);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, VACA);
        DrawVirtualObject("cow");

        // estande 6
        model = Matrix_Translate(posicoes_estandes[6-1].x, posicoes_estandes[6-1].y + 4.2f, posicoes_estandes[6-1].z)
              * Matrix_Scale(0.3f, 0.3f, 0.3f)
              * Matrix_Rotate_Z(g_AngleZ)
              * Matrix_Rotate_Y(g_AngleY)
              * Matrix_Rotate_X(g_AngleX);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, CUBO);
        DrawVirtualObject("cubo");


        // estande 7
        model = Matrix_Translate(posicoes_estandes[7-1].x + 0.2f, posicoes_estandes[7-1].y + 4.0f, posicoes_estandes[7-1].z + 0.3f)
              * Matrix_Scale(0.25f, 0.25f, 0.25f)
              * Matrix_Rotate_X(-1.5f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, CUBO);
        DrawVirtualObject("cubo");

        model = Matrix_Translate(posicoes_estandes[7-1].x - 0.1f, posicoes_estandes[7-1].y + 4.2f, posicoes_estandes[7-1].z - 0.5f)
              * Matrix_Scale(0.25f, 0.25f, 0.25f)
              * Matrix_Rotate_X(-1.5f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, CUBO);
        DrawVirtualObject("cubo");

        // estande 8
        model = Matrix_Translate(posicoes_estandes[8-1].x, posicoes_estandes[8-1].y + 4.2f, posicoes_estandes[8-1].z)
              * Matrix_Scale(0.4f, 0.4f, 0.4f)
              * Matrix_Rotate_X((float)glfwGetTime() * 0.3f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, ROSQUINHA_1);
        DrawVirtualObject("rosquinha_1");
        model = Matrix_Translate(posicoes_estandes[8-1].x, posicoes_estandes[8-1].y + 4.2f, posicoes_estandes[8-1].z)
              * Matrix_Scale(0.4f, 0.4f, 0.4002f)
              * Matrix_Rotate_X((float)glfwGetTime() * 0.3f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, ROSQUINHA_2);
        DrawVirtualObject("rosquinha_2");


        // estande 9
        // normalizando valores que inicialmente iam de [-1, 1] (como faz o cosseno)
        //      para [0,1]
        float t_bezier = ( cos(time_now) - (-1) )/(1 - (-1)) ;

         // necessário ajustar valores
        p1X_9 = posicoes_estandes[9-1].x + 0.6f;
        p1Y_9 = posicoes_estandes[9-1].y + 4.0f;
        p1Z_9 = posicoes_estandes[9-1].z + 0.5f;

        p2X_9 = posicoes_estandes[9-1].x + 0.2f;
        p2Y_9 = posicoes_estandes[9-1].y + 4.5f;
        p2Z_9 = posicoes_estandes[9-1].z + 0.2f;

        p3X_9 = posicoes_estandes[9-1].x - 0.2f;
        p3Y_9 = posicoes_estandes[9-1].y + 4.7f;
        p3Z_9 = posicoes_estandes[9-1].z - 0.3f;


        p4X_9 = posicoes_estandes[9-1].x - 0.5f;
        p4Y_9 = posicoes_estandes[9-1].y + 4.2f;
        p4Z_9 = posicoes_estandes[9-1].z - 0.5f;

        glm::vec4 p1 (p1X_9, p1Y_9, p1Z_9, 1.0f);
        glm::vec4 p2 (p2X_9, p2Y_9, p2Z_9, 1.0f);
        glm::vec4 p3 (p3X_9, p3Y_9, p3Z_9, 1.0f);
        glm::vec4 p4 (p4X_9, p4Y_9, p4Z_9, 1.0f);

        glm::vec4 deslocamento_9 = bezier(t_bezier, p1, p2, p3, p4);

        model = Matrix_Translate(deslocamento_9.x, deslocamento_9.y, deslocamento_9.z)
              * Matrix_Scale(0.3f, 0.3f, 0.3f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, VACA);
        DrawVirtualObject("cow");


        // estande 10
        model = Matrix_Translate(posicoes_estandes[10-1].x, posicoes_estandes[10-1].y + 3.65f, posicoes_estandes[10-1].z + 0.2f)
              * Matrix_Scale(2.6f, 2.6f, 2.6f)
              * Matrix_Rotate_X(-1.2f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, LAMPADA);
        DrawVirtualObject("lampada");

        // estande 13
        model = Matrix_Translate(posicoes_estandes[13-1].x, posicoes_estandes[13-1].y + 4.2f, posicoes_estandes[13-1].z)
              * Matrix_Scale(0.5f, 0.5f, 0.5f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, ESFERA);
        DrawVirtualObject("esfera");

        // estande 14
        model = Matrix_Translate(posicoes_estandes[14-1].x, posicoes_estandes[14-1].y + 3.8f, posicoes_estandes[14-1].z)
              * Matrix_Scale(3.5f, 3.5f, 3.5f)
              * Matrix_Rotate_Y((float)glfwGetTime() * 0.25f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, CHALEIRA_PLANA);
        DrawVirtualObject("chaleira");

        // estande 15
        model = Matrix_Translate(posicoes_estandes[15-1].x, posicoes_estandes[15-1].y + 3.8f, posicoes_estandes[15-1].z)
              * Matrix_Scale(3.5f, 3.5f, 3.5f)
              * Matrix_Rotate_Y((float)glfwGetTime() * 0.25f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, CHALEIRA_CUBICA);
        DrawVirtualObject("chaleira");

        // estande 16
        model = Matrix_Translate(posicoes_estandes[16-1].x, posicoes_estandes[16-1].y + 3.8f, posicoes_estandes[16-1].z)
              * Matrix_Scale(3.5f, 3.5f, 3.5f)
              * Matrix_Rotate_Y((float)glfwGetTime() * 0.25f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, CHALEIRA_ESFERICA);
        DrawVirtualObject("chaleira");

        // estande 17
        model = Matrix_Translate(posicoes_estandes[17-1].x, posicoes_estandes[17-1].y + 3.8f, posicoes_estandes[17-1].z)
              * Matrix_Scale(3.5f, 3.5f, 3.5f)
              * Matrix_Rotate_Y((float)glfwGetTime() * 0.25f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, CHALEIRA_CILINDRICA);
        DrawVirtualObject("chaleira");


        // estande 18

        // plano
        model = Matrix_Translate(posicoes_estandes[18-1].x, posicoes_estandes[18-1].y + 3.8f, posicoes_estandes[18-1].z - 0.3f)
              * Matrix_Scale(0.65f, 0.6f, 0.46f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, PLANO);
        DrawVirtualObject("plano");

        obj_min = g_VirtualScene["plano"].bbox_min;
        obj_max = g_VirtualScene["plano"].bbox_max;
        obj_min_vec4 = glm::vec4(obj_min.x, obj_min.y, obj_min.z, 1.0f);
        obj_max_vec4 = glm::vec4(obj_max.x, obj_max.y, obj_max.z, 1.0f);

        posMin = model * obj_min_vec4;
        posMax = model * obj_max_vec4;

        struct plane_obj obj_plano;
        obj_plano.c = glm::vec3( (posMin.x + posMax.x)/2.0f, (posMin.y + posMax.y)/2.0f, (posMin.z + posMax.z)/2.0f );
        obj_plano.x_size = absolute_float(posMax.x - obj_plano.c.x);
        obj_plano.z_size = absolute_float(posMax.z - obj_plano.c.z);


        // cubo 1
        if (obj_atual_stand18 >= 1){
            model = Matrix_Translate(posicoes_estandes[18-1].x, posicoes_estandes[18-1].y + 5.0f - cai_obj1, posicoes_estandes[18-1].z - 0.3f)
                * Matrix_Scale(0.08f, 0.08f, 0.08f);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(object_id_uniform, CUBO);
            DrawVirtualObject("cubo");

            obj_min = g_VirtualScene["cubo"].bbox_min;
            obj_max = g_VirtualScene["cubo"].bbox_max;
            obj_min_vec4 = glm::vec4(obj_min.x, obj_min.y, obj_min.z, 1.0f);
            obj_max_vec4 = glm::vec4(obj_max.x, obj_max.y, obj_max.z, 1.0f);

            posMin = model * obj_min_vec4;
            posMax = model * obj_max_vec4;

            struct box_obj obj_caixa1;
            obj_caixa1.c = glm::vec3( (posMin.x + posMax.x)/2.0f , (posMin.y + posMax.y)/2.0f, (posMin.z + posMax.z)/2.0f );
            obj_caixa1.x_size = absolute_float(posMax.x - obj_caixa1.c.x);
            obj_caixa1.y_size = absolute_float(posMax.y - obj_caixa1.c.y);
            obj_caixa1.z_size = absolute_float(posMax.z - obj_caixa1.c.z);

            if (!interseccao_caixa_plano(obj_caixa1, obj_plano)){
                cai_obj1 += passo_tempo/2.0f;
            } else {
                obj_atual_stand18 = 2;
            }


        }

        // cubo 2
        if (obj_atual_stand18 >=2){
            model = Matrix_Translate(posicoes_estandes[18-1].x + 0.1f, posicoes_estandes[18-1].y + 5.0f /*+ object_fall*/, posicoes_estandes[18-1].z - 0.3f)
                * Matrix_Scale(0.08f, 0.08f, 0.08f);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(object_id_uniform, CUBO);
            DrawVirtualObject("cubo");

            obj_min = g_VirtualScene["cubo"].bbox_min;
            obj_max = g_VirtualScene["cubo"].bbox_max;
            obj_min_vec4 = glm::vec4(obj_min.x, obj_min.y, obj_min.z, 1.0f);
            obj_max_vec4 = glm::vec4(obj_max.x, obj_max.y, obj_max.z, 1.0f);

            posMin = model * obj_min_vec4;
            posMax = model * obj_max_vec4;

            struct box_obj obj_caixa2;
            obj_caixa2.c = glm::vec3( (posMin.x + posMax.x)/2.0f , (posMin.y + posMax.y)/2.0f, (posMin.z + posMax.z)/2.0f );
            obj_caixa2.x_size = absolute_float(posMax.x - obj_caixa2.c.x);
            obj_caixa2.y_size = absolute_float(posMax.y - obj_caixa2.c.y);
            obj_caixa2.z_size = absolute_float(posMax.z - obj_caixa2.c.z);
        }

        // esfera 1
        if (obj_atual_stand18 >= 3){
            model = Matrix_Translate(posicoes_estandes[18-1].x + 0.5, posicoes_estandes[18-1].y + 5.0f, posicoes_estandes[18-1].z - 0.3f)
                * Matrix_Scale(0.12f, 0.12f, 0.12f);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(object_id_uniform, ESFERA);
            DrawVirtualObject("esfera");

            obj_min = g_VirtualScene["esfera"].bbox_min;
            obj_max = g_VirtualScene["esfera"].bbox_max;
            obj_min_vec4 = glm::vec4(obj_min.x, obj_min.y, obj_min.z, 1.0f);
            obj_max_vec4 = glm::vec4(obj_max.x, obj_max.y, obj_max.z, 1.0f);

            posMin = model * obj_min_vec4;
            posMax = model * obj_max_vec4;

            struct sphere_obj obj_esfera1;
            obj_esfera1.c = glm::vec3( (posMin.x + posMax.x)/2.0f , (posMin.y + posMax.y)/2.0f, (posMin.z + posMax.z)/2.0f );
            obj_esfera1.r = absolute_float(posMax.x - obj_esfera1.c.x);
        }

        //esfera 2
        if (obj_atual_stand18 >= 4){
            model = Matrix_Translate(posicoes_estandes[18-1].x + 0.5, posicoes_estandes[18-1].y + 4.9f, posicoes_estandes[18-1].z - 0.3f)
                * Matrix_Scale(0.12f, 0.12f, 0.12f);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(object_id_uniform, ESFERA);
            DrawVirtualObject("esfera");

            obj_min = g_VirtualScene["esfera"].bbox_min;
            obj_max = g_VirtualScene["esfera"].bbox_max;
            obj_min_vec4 = glm::vec4(obj_min.x, obj_min.y, obj_min.z, 1.0f);
            obj_max_vec4 = glm::vec4(obj_max.x, obj_max.y, obj_max.z, 1.0f);

            posMin = model * obj_min_vec4;
            posMax = model * obj_max_vec4;

            struct sphere_obj obj4;
            obj4.c = glm::vec3( (posMin.x + posMax.x)/2.0f , (posMin.y + posMax.y)/2.0f, (posMin.z + posMax.z)/2.0f );
            obj4.r = absolute_float(posMax.x - obj4.c.x);
        }



        // Pegamos um vértice com coordenadas de modelo (0.5, 0.5, 0.5, 1) e o
        // passamos por todos os sistemas de coordenadas armazenados nas
        // matrizes the_model, the_view, e the_projection; e escrevemos na tela
        // as matrizes e pontos resultantes dessas transformações.
        //glm::vec4 p_model(0.5f, 0.5f, 0.5f, 1.0f);
        //TextRendering_ShowModelViewProjection(window, projection, view, model, p_model);

        // Imprimimos na tela os ângulos de Euler que controlam a rotação do
        // terceiro cubo.
        //TextRendering_ShowEulerAngles(window);

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        //TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        //TextRendering_ShowFramesPerSecond(window);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
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

float absolute_float(float v){
    if (v < 0){
        return -v;
    }
    return v;
}

glm::vec4 bezier(float t, glm::vec4 p1, glm::vec4 p2, glm::vec4 p3, glm::vec4 p4)
{

    glm::vec4 c12 = p1 + t*(p2-p1);
    glm::vec4 c23 = p2 + t*(p3-p2);
    glm::vec4 c34 = p3 + t*(p4-p3);
    glm::vec4 c123 = c12 + t*(c23-c12);
    glm::vec4 c234 = c23 + t*(c34-c23);
    glm::vec4 c = c123 + t*(c234-c123);

    return c;
}

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

void load_free_camera(){
    camera_view_ID = FREE_CAMERA;
    g_CameraDistance = CameraDistance_save;
    g_CameraPhi = CameraPhi_FC_save;
    g_CameraTheta = CameraTheta_FC_save;
    g_UsePerspectiveProjection = true;
}

void load_look_at_camera(){
    camera_view_ID = LOOK_AT_CAMERA;
}

bool interseccao_caixa_caixa(struct box_obj obj1, struct box_obj obj2){
    glm::vec3 distancia_centros = (obj1.c - obj2.c);

    bool x_dist = absolute_float(distancia_centros.x) <= (obj1.x_size + obj2.x_size);
    bool y_dist = absolute_float(distancia_centros.y) <= (obj1.y_size + obj2.y_size);
    bool z_dist = absolute_float(distancia_centros.z) <= (obj1.z_size + obj2.z_size);

    return (x_dist && y_dist && z_dist);
}

bool interseccao_esfera_esfera(struct sphere_obj obj1, struct sphere_obj obj2){
    glm::vec3 vetor_dist = obj1.c - obj2.c;
    float distancia_centros = sqrt( pow(vetor_dist.x,2) + pow(vetor_dist.y,2) + pow(vetor_dist.z,2) );

    return distancia_centros <= (obj1.r + obj2.r);
}

bool interseccao_caixa_esfera(struct box_obj caixa, struct sphere_obj esfera){
    glm::vec3 distancia_centros = (caixa.c - esfera.c);

    bool x_dist = absolute_float(distancia_centros.x) <= (caixa.x_size + esfera.r);
    bool y_dist = absolute_float(distancia_centros.y) <= (caixa.y_size + esfera.r);
    bool z_dist = absolute_float(distancia_centros.z) <= (caixa.z_size + esfera.r);

    return (x_dist && y_dist && z_dist);
}

bool interseccao_caixa_plano(struct box_obj caixa, struct plane_obj plano){
    glm::vec3 distancia_centros = (caixa.c - plano.c);

    bool y_dist = absolute_float(distancia_centros.y) <= (caixa.y_size);
    bool x_dist = absolute_float(distancia_centros.x) <= (caixa.x_size + plano.x_size);
    bool z_dist = absolute_float(distancia_centros.z) <= (caixa.z_size + plano.z_size);

    return (y_dist && x_dist && z_dist);
}

bool interseccao_esfera_plano(struct sphere_obj esfera, struct plane_obj plano){
    glm::vec3 distancia_centros = (esfera.c - plano.c);

    bool y_dist = absolute_float(distancia_centros.y) <= (esfera.r);
    bool x_dist = absolute_float(distancia_centros.x) <= (esfera.r + plano.x_size);
    bool z_dist = absolute_float(distancia_centros.z) <= (esfera.r + plano.z_size);

    return (y_dist && x_dist && z_dist);
}

/// return true if inside museum
bool check_inside_museum(float x,  float z){
    glm::vec3 p_41 = Museu.p4 - Museu.p1;
    glm::vec3 p_24 = Museu.p2 - Museu.p4;
    glm::vec3 p_12 = Museu.p1 - Museu.p2;

    glm::vec3 p_23 = Museu.p2 - Museu.p3;
    glm::vec3 p_42 = Museu.p4 - Museu.p2;
    glm::vec3 p_34 = Museu.p3 - Museu.p4;

    bool t1 = F_p1_p2(p_41, Museu.p4, x, z) > 0 && F_p1_p2(p_24, Museu.p2, x, z) > 0 && F_p1_p2(p_12, Museu.p1, x, z) > 0;
    bool t2 = F_p1_p2(p_23, Museu.p2, x, z) > 0 && F_p1_p2(p_42, Museu.p4, x, z) > 0 && F_p1_p2(p_34, Museu.p3, x, z) > 0;

    if (t1 || t2) {
        return true;
    }
    return false;
}

/// return true if outside estande
bool check_estande(struct square_bbox estande, float x, float z){
    glm::vec3 p_41 = estande.p4 - estande.p1;
    glm::vec3 p_24 = estande.p2 - estande.p4;
    glm::vec3 p_12 = estande.p1 - estande.p2;

    glm::vec3 p_23 = estande.p2 - estande.p3;
    glm::vec3 p_42 = estande.p4 - estande.p2;
    glm::vec3 p_34 = estande.p3 - estande.p4;

    bool t1 = F_p1_p2(p_41, estande.p4, x, z) >= 0 && F_p1_p2(p_24, estande.p2, x, z) >= 0 && F_p1_p2(p_12, estande.p1, x, z) >= 0;
    bool t2 = F_p1_p2(p_23, estande.p2, x, z) >= 0 && F_p1_p2(p_42, estande.p4, x, z) >= 0 && F_p1_p2(p_34, estande.p3, x, z) >= 0;

    if (t1 || t2) {
        return false;
    }
    return true;
}

bool check_estandes(float x, float z){
    bool colisoes_estandes = true;
    for (int i=0; i != QUANT_ESTANDE; i++){
        colisoes_estandes = colisoes_estandes && check_estande(estandes_bbox[i], x, z);
    }
    return colisoes_estandes;
}

float F_p1_p2(glm::vec3 v, glm::vec3 a, float x, float z){
    return v.z*x - v.x*z - v.z*a.x + v.x*a.z;
}

bool check_colision(float x, float z){
    return check_inside_museum(x, z) && check_estandes(x, z); //check_dinosaur(x, z);
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    char filepath[100];
    strcpy(filepath, filename);
    strcat(filepath, ".png");

    printf("Carregando imagem \"%s\"... ", filepath);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filepath, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filepath);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slide 100 do documento "Aula_20_e_21_Mapeamento_de_Texturas.pdf"
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura. Falaremos sobre eles em uma próxima aula.
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
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 217-219 do documento "Aula_03_Rendering_Pipeline_Grafico.pdf".
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
    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( program_id != 0 )
        glDeleteProgram(program_id);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    model_uniform           = glGetUniformLocation(program_id, "model"); // Variável da matriz "model"
    view_uniform            = glGetUniformLocation(program_id, "view"); // Variável da matriz "view" em shader_vertex.glsl
    projection_uniform      = glGetUniformLocation(program_id, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    object_id_uniform       = glGetUniformLocation(program_id, "object_id"); // Variável "object_id" em shader_fragment.glsl
    bbox_min_uniform        = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform        = glGetUniformLocation(program_id, "bbox_max");

    estande_shader = glGetUniformLocation(program_id, "estande_atual");
    acerto_ou_erro_est1 = glGetUniformLocation(program_id, "acerto_ou_erro_est1");
    cor_lampada_shader = glGetUniformLocation(program_id, "cor_lampada");
    direcao_textura_plana_shader = glGetUniformLocation(program_id, "direcao_planar");


    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage5"), 5);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage6"), 6);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage7"), 7);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage8"), 8);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage9"), 9);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage10"), 10);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage11"), 11);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage12"), 12);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage13"), 13);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage14"), 14);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage15"), 15);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage16"), 16);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage17"), 17);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage18"), 18);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage19"), 19);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage20"), 20);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage21"), 21);


    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
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
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

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

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
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
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
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
GLuint LoadShader_Vertex(const char* filename)
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
GLuint LoadShader_Fragment(const char* filename)
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
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

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
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
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
    delete [] log;
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
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

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
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula (slides 33-44 do documento "Aula_07_Transformacoes_Geometricas_3D.pdf").
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slide 227 do documento "Aula_09_Projecoes.pdf".
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
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
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
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    if (g_LeftMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da câmera com os deslocamentos
        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   += 0.01f*dy;

        // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = M_PI/4;

        float phimin = -M_PI/3;

        if (camera_view_ID == LOOK_AT_CAMERA){
            phimin = -M_PI/15;
        }

        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;

        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_RightMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da antebraço com os deslocamentos
        //g_ForearmAngleZ -= 0.01f*dx;
        //g_ForearmAngleX += 0.01f*dy;

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
        //g_TorsoPositionX += 0.01f*dx;
        //g_TorsoPositionY -= 0.01f*dy;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        //g_LastCursorPosX = xpos;
        //g_LastCursorPosY = ypos;
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // ==============
    // Não modifique este loop! Ele é utilizando para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    // ==============

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        g_AngleX_5 = 0.0f;
        g_AngleY_5 = 0.0f;
        g_AngleZ_5 = 0.0f;
        g_posX_5 = 0.0f;
        g_posY_5 = 0.0f;
        g_posZ_5 = 0.0f;
        g_scaleX_5 = 0.5f;
        g_scaleY_5 = 0.5f;
        g_scaleZ_5 = 0.5f;

        if (camera_view_ID == LOOK_AT_CAMERA)
            load_free_camera();
        else
            glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS){
        load_look_at_camera();
    }

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = M_PI / 16; // 22.5 graus, em radianos.

    if (key == GLFW_KEY_X && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 6-1)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 6-1)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    //Set gimbal lock
    if (key == GLFW_KEY_G && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 6-1)
    {
        g_AngleY = M_PI/2;
    }

    if (key == GLFW_KEY_Z && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 6-1)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Se o usuário apertar a tecla espaço, resetamos os ângulos de Euler para zero.
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 6-1 )
    {
        g_AngleX = 0.0f;
        g_AngleY = 0.0f;
        g_AngleZ = 0.0f;
        //g_ForearmAngleX = 0.0f;
        //g_ForearmAngleZ = 0.0f;
        //g_TorsoPositionX = 0.0f;
        //g_TorsoPositionY = 0.0f;
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 7-1)
    {
        g_UsePerspectiveProjection = true;
    }
    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 7-1)
    {
        g_UsePerspectiveProjection = false;
    }
    if(camera_view_ID == LOOK_AT_CAMERA && estande_atual != 7-1){
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }



    // Atual controle sobre qual camera usar
    if (key == GLFW_KEY_1 && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 1-1){
        opcao_estande1 = 1;
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 1-1){
        opcao_estande1 = 2;
    }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 10-1){
        cor_lampada = 1;
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 10-1){
        cor_lampada = 2;
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 10-1){
        cor_lampada = 3;
    }
    if (key == GLFW_KEY_4 && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 10-1){
        cor_lampada = 4;
    }
    if (key == GLFW_KEY_5 && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 10-1){
        cor_lampada = 5;
    }
    if (key == GLFW_KEY_6 && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 10-1){
        cor_lampada = 6;
    }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 14-1){
        direcao_textura_plana = 1;
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 14-1){
        direcao_textura_plana = 2;
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 14-1){
        direcao_textura_plana = 3;
    }




    // TECLAS W/S/D/A
    if (key == GLFW_KEY_W)
        pressedW = (action == GLFW_RELEASE) ? 0 : 1;

    if (key == GLFW_KEY_S)
        pressedS = (action == GLFW_RELEASE) ? 0 : 1;

    if (key == GLFW_KEY_A)
        pressedA = (action == GLFW_RELEASE) ? 0 : 1;

    if (key == GLFW_KEY_D)
        pressedD = (action == GLFW_RELEASE) ? 0 : 1;

    if (key == GLFW_KEY_UP && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 2-1){
        if (g_Angle_Stand2 < 0.5f){
            g_Angle_Stand2 += 0.05f;
            g_aux_Stand2 += 1.0f;
        }
    }
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS && camera_view_ID == LOOK_AT_CAMERA && estande_atual == 2-1){
        if (g_Angle_Stand2 > -0.5f){
            g_Angle_Stand2 -= 0.05f;
            g_aux_Stand2 -= 1.0f;
        }
    }

    // Andar entre os estandes
    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS && camera_view_ID==2) {
        if ( estande_atual == QUANT_ESTANDE-1 ){
            estande_atual = 0;
        } else {
            estande_atual += 1;
        }
    }
    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS && camera_view_ID) {
        if ( estande_atual == 0 ){
            estande_atual = QUANT_ESTANDE-1;
        } else {
            estande_atual -= 1;
        }
    }

    // Ações específicas para cada estande
    // se estande 5
    if (estande_atual == 5-1 && camera_view_ID == 2)
    {
        if (key == GLFW_KEY_X && action == GLFW_PRESS)
        {
            g_AngleX_5 += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
        }
        if (key == GLFW_KEY_C && action == GLFW_PRESS)
        {
            g_AngleY_5 += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
        }
        if (key == GLFW_KEY_Z && action == GLFW_PRESS)
        {
            g_AngleZ_5 += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
        }

        if (key == GLFW_KEY_Q && action == GLFW_PRESS)
        {
            g_posX_5 += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
        }
        if (key == GLFW_KEY_W && action == GLFW_PRESS)
        {
            g_posY_5 += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
        }
        if (key == GLFW_KEY_E && action == GLFW_PRESS)
        {
            g_posZ_5 += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
        }

        if (key == GLFW_KEY_A && action == GLFW_PRESS)
        {
            g_scaleX_5 += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
        }
        if (key == GLFW_KEY_S && action == GLFW_PRESS)
        {
            g_scaleY_5 += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
        }
        if (key == GLFW_KEY_D && action == GLFW_PRESS)
        {
            g_scaleZ_5 += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
        }

        if ((key == GLFW_KEY_SPACE || key == GLFW_KEY_ESCAPE) && action == GLFW_PRESS)
        {
            g_AngleX_5 = 0.0f;
            g_AngleY_5 = 0.0f;
            g_AngleZ_5 = 0.0f;

            g_posX_5 = 0.0f;
            g_posY_5 = 0.0f;
            g_posZ_5 = 0.0f;

            g_scaleX_5 = 0.5f;
            g_scaleY_5 = 0.5f;
            g_scaleZ_5 = 0.5f;
        }
    }

    float delta_9 = 0.5f;
        // se estande 9
    if (estande_atual == 9-1 && camera_view_ID == 2)
    {
        // P1
        if (key == GLFW_KEY_Q && action == GLFW_PRESS)
        {
            p1X_9 += (mod & GLFW_MOD_SHIFT) ? -delta_9 : delta_9;
        }
        if (key == GLFW_KEY_A && action == GLFW_PRESS)
        {
            p1Y_9 += (mod & GLFW_MOD_SHIFT) ? -delta_9 : delta_9;
        }
        // P2
        if (key == GLFW_KEY_W && action == GLFW_PRESS)
        {
            p2X_9 += (mod & GLFW_MOD_SHIFT) ? -delta_9 : delta_9;
        }
        if (key == GLFW_KEY_S && action == GLFW_PRESS)
        {
            p2Y_9 += (mod & GLFW_MOD_SHIFT) ? -delta_9 : delta_9;
        }
        // P3
        if (key == GLFW_KEY_E && action == GLFW_PRESS)
        {
            p3X_9 += (mod & GLFW_MOD_SHIFT) ? -delta_9 : delta_9;
        }
        if (key == GLFW_KEY_D && action == GLFW_PRESS)
        {
            p3Y_9 += (mod & GLFW_MOD_SHIFT) ? -delta_9 : delta_9;
        }
        // P4
        if (key == GLFW_KEY_R && action == GLFW_PRESS)
        {
            p4X_9 += (mod & GLFW_MOD_SHIFT) ? -delta_9 : delta_9;
        }
        if (key == GLFW_KEY_F && action == GLFW_PRESS)
        {
            p4Y_9 += (mod & GLFW_MOD_SHIFT) ? -delta_9 : delta_9;
        }


        if ((key == GLFW_KEY_SPACE || key == GLFW_KEY_ESCAPE) && action == GLFW_PRESS)
        {
            g_AngleX_5 = 0.0f;
            g_AngleY_5 = 0.0f;
            g_AngleZ_5 = 0.0f;

            g_posX_5 = 0.0f;
            g_posY_5 = 0.0f;
            g_posZ_5 = 0.0f;

            g_scaleX_5 = 0.5f;
            g_scaleY_5 = 0.5f;
            g_scaleZ_5 = 0.5f;
        }
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
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
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
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
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
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

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :

