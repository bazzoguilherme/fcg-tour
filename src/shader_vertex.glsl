#version 330 core

// Atributos de vértice recebidos como entrada ("in") pelo Vertex Shader.
// Veja a função BuildTrianglesAndAddToVirtualScene() em "main.cpp".
layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#define ESFERA_GOURAUD 20
uniform int object_id;

// Atributos de vértice que serão gerados como saída ("out") pelo Vertex Shader.
// ** Estes serão interpolados pelo rasterizador! ** gerando, assim, valores
// para cada fragmento, os quais serão recebidos como entrada pelo Fragment
// Shader. Veja o arquivo "shader_fragment.glsl".
out vec4 position_world;
out vec4 position_model;
out vec4 normal;
out vec2 texcoords;
out vec3 cor_v;

void main()
{
    // A variável gl_Position define a posição final de cada vértice
    // OBRIGATORIAMENTE em "normalized device coordinates" (NDC), onde cada
    // coeficiente estará entre -1 e 1 após divisão por w.
    // Veja slides 144 e 150 do documento "Aula_09_Projecoes.pdf".
    //
    // O código em "main.cpp" define os vértices dos modelos em coordenadas
    // locais de cada modelo (array model_coefficients). Abaixo, utilizamos
    // operações de modelagem, definição da câmera, e projeção, para computar
    // as coordenadas finais em NDC (variável gl_Position). Após a execução
    // deste Vertex Shader, a placa de vídeo (GPU) fará a divisão por W. Veja
    // slide 189 do documento "Aula_09_Projecoes.pdf".

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
    // Veja slide 107 do documento "Aula_07_Transformacoes_Geometricas_3D.pdf".
    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;

    // Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
    texcoords = texture_coefficients;


cor_v = vec3(0.0f, 0.0f, 0.0f);

    // GOURAUD SHADING
    if ( object_id == ESFERA_GOURAUD )
    {
        float q = 40;
        vec3 Kd = vec3(1.0, 0.843, 0.0);
        vec3 Ka = vec3(1.000000, 1.000000, 1.000000);
        vec3 Ks = vec3(0.8,0.8,0.8);

        vec4 spotlightPosition = vec4(-22.0,4,0.0,1.0);
        vec4 spotlightDirection = vec4(0.0,-1.0,0.0,0.0);

        vec4 l = normalize(spotlightPosition - position_world);

        vec4 n = normalize(normal);

        // Espectro da fonte de iluminação
        vec3 I = vec3(1.0, 1.0, 1.0);

        // Espectro da luz ambiente
        vec3 Ia = vec3(0.1, 0.1, 0.1);

         // Termo difuso utilizando a lei dos cossenos de Lambert
        vec3 lambert_diffuse_term = Kd*I*max(0.0,dot(n,l));

        // Termo ambiente
        vec3 ambient_term = Ka*Ia;



        float lambert = max(0,dot(n,l));

        vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
        vec4 camera_position = inverse(view) * origin;

        vec4 v = normalize(camera_position - position_world);

        vec4 r = -l + 2*n*(dot(n, l));

        float phong = pow(max(0.0, dot(r, v)), q);

        if (dot((normalize(position_world - spotlightPosition)), normalize(spotlightDirection)) < cos(3.141529/2.5))
            cor_v = Kd * (lambert + 0.01);
        else
            cor_v = Kd * I * (lambert + 0.01) + ambient_term + Ks * I * phong;

    }


}

