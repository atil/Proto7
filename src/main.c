// - start from here: 
// - [assets] handle comments correctly when computing mtl_count
// - [infra] move_dir pdb file to bin and make sure remedybg/raddbg works
// - [infra] be able to click exe. get rid of path errors
// - [infra] figure out sensitivity difference between machines
// - [ui] dynamic-content-fixed-size text

#pragma warning(disable:5045) // Spectre thing
#pragma warning(disable:4820) // Padding

#pragma warning(push)
#pragma warning(disable:4255)
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#define GLEW_STATIC // Also need to include opengl32lib for this to work
#include <GL/glew.h>
#include <glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_TRUETYPE_IMPLEMENTATION 
#include <stb_truetype.h>
#pragma warning(pop)

typedef size_t u64;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint8_t u8;

#define DEG2RAD 0.0174533f
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define FONT_ATLAS_WIDTH 512
#define FONT_ATLAS_HEIGHT 512
#define FONT_CHAR_COUNT 96
#define FONT_TEXT_HEIGHT_PIXELS 64 // In pixels
#define MTL_MAX_COUNT 16 // in a .mtl file
#define MTL_NAME_LEN 32 // name of sections inside a .mtl file
#define MTL_FILENAME_LEN 64 // .mtl file itself
#define MTL_TEXTURE_FILENAME_LEN 64

typedef struct {
    u32 vao;
    u32 vbo;
    u32 vertex_count;
    u32 tex_handle;
} gameobject_t;

typedef struct {
    float* vertex_data;
    u32 vertex_count;
    char texture_name[MTL_TEXTURE_FILENAME_LEN];
} mesh_t; // Render-ready data

typedef struct {
    char name[MTL_NAME_LEN];
    char texture_name[MTL_TEXTURE_FILENAME_LEN];
    // emission etc here
} mtldata_t; // Single material data

typedef struct {
    mtldata_t materials[MTL_MAX_COUNT];
    u32 mtl_count;
} mtlasset_t;

typedef struct {
    char mtl_name[MTL_NAME_LEN]; // usemtl argument
    u32* face_data; // Array of [v/u/n v/u/n v/u/n]
    u32 face_count; // Number of face rows
} objsubdata_t;

typedef struct {
    float* positions;
    float* uvs;
    float* normals;
    objsubdata_t* subs;
    u32 mtl_count;
} objasset_t;

typedef struct {
    stbtt_bakedchar font_char_data[FONT_CHAR_COUNT];
    float text_scale;
    u32 shader;
    u32 font_bitmap_handle;
} ui_t;

typedef struct {
    u32 vao;
    u32 vbo;
    u32 vertex_count;
} uitext_t;

#include "geom.c"
#include "assets.c"

u32 create_shader(char* vert_shader_filename, char* frag_shader_filename) {
    char* vert_shader_source = read_entire_file(vert_shader_filename);
    char* frag_shader_source = read_entire_file(frag_shader_filename);

    int success;
    char info_log[512];

    // Vert
    u32 vert_shader_handle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_shader_handle, 1, &vert_shader_source, NULL);
    glCompileShader(vert_shader_handle);

    glGetShaderiv(vert_shader_handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vert_shader_handle, 512, NULL, info_log);
        printf("vertex shader compilation error: %s\n", info_log);
    }

    // Frag
    u32 frag_shader_handle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader_handle, 1, &frag_shader_source, NULL);
    glCompileShader(frag_shader_handle);

    glGetShaderiv(frag_shader_handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(frag_shader_handle, 512, NULL, info_log);
        printf("fragment shader compilation error: %s\n", info_log);
    }
    
    // Link
    u32 shader_program = glCreateProgram();
    glAttachShader(shader_program, vert_shader_handle);
    glAttachShader(shader_program, frag_shader_handle);
    glLinkProgram(shader_program);

    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, NULL, info_log);
        printf("shader link error: %s\n", info_log);
    }

    free(vert_shader_source);
    free(frag_shader_source);
    glDeleteShader(vert_shader_handle);
    glDeleteShader(frag_shader_handle);

    return shader_program;
}

void render_create_buffer(gameobject_t* p_go, mesh_t* p_mesh) {
    p_go->vertex_count = p_mesh->vertex_count;
    glGenVertexArrays(1, &(p_go->vao));
    glGenBuffers(1, &(p_go->vbo));

    glBindVertexArray(p_go->vao);
    glBindBuffer(GL_ARRAY_BUFFER, p_go->vbo);
    glBufferData(GL_ARRAY_BUFFER, p_mesh->vertex_count * 8 * sizeof(float), p_mesh->vertex_data, GL_STATIC_DRAW); 

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenTextures(1, &(p_go->tex_handle));
    glBindTexture(GL_TEXTURE_2D, p_go->tex_handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   
    int img_width, img_height, img_channel_count;
    stbi_set_flip_vertically_on_load(true);
    u8* image_data = stbi_load(p_mesh->texture_name, &img_width, &img_height, &img_channel_count, 0);
    if (!image_data) {
        printf("problem with texture file: %s\n", p_mesh->texture_name);
        assert(false);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(image_data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void render_delete_go(gameobject_t* p_go) {
    glDeleteVertexArrays(1, &(p_go->vao));
    glDeleteBuffers(1, &(p_go->vbo));
    glDeleteTextures(1, &(p_go->tex_handle));
}

void render_draw_go(gameobject_t* p_go) {
    glBindVertexArray(p_go->vao);
    glBindTexture(GL_TEXTURE_2D, p_go->tex_handle);
    glDrawArrays(GL_TRIANGLES, 0, p_go->vertex_count);
    glBindVertexArray(0);
}

void ui_init(ui_t* ui) {
    ui->shader = create_shader("src/shader_ui_vert.glsl", "src/shader_ui_frag.glsl");
    glUseProgram(ui->shader);
    glUniform1i(glGetUniformLocation(ui->shader, "u_texture_ui"), 0);

    u8* font_bytes = (u8*)read_entire_file("Consolas.ttf");
    u8* font_bitmap = malloc(FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT * sizeof(u8));
    stbtt_BakeFontBitmap((u8*)font_bytes, 0, FONT_TEXT_HEIGHT_PIXELS, font_bitmap, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, ' ', FONT_CHAR_COUNT, ui->font_char_data);

    stbtt_fontinfo font_info;
    stbtt_InitFont(&font_info, font_bytes, 0);
    ui->text_scale = stbtt_ScaleForPixelHeight(&font_info, FONT_TEXT_HEIGHT_PIXELS);

    glGenTextures(1, &(ui->font_bitmap_handle));
    glBindTexture(GL_TEXTURE_2D, ui->font_bitmap_handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, font_bitmap);

    free(font_bytes);
    free(font_bitmap);
}

void ui_create_text_static(uitext_t* ui_text, ui_t* ui, char* text_content, vec2 text_anchor_pixels, vec2 text_scale_pixels) {

    // Filling in buffer
    vec2 text_anchor_ndc = { text_anchor_pixels.x * 2 / SCREEN_WIDTH, text_anchor_pixels.y * 2 / SCREEN_HEIGHT };
    vec2 text_scale_ndc = { text_scale_pixels.x * 2 / SCREEN_WIDTH, text_scale_pixels.y * 2 / SCREEN_HEIGHT };
    u32 char_count = (u32)strlen(text_content);
    ui_text->vertex_count = char_count * 6; // Two triangles per char
    float* text_buffer = malloc(ui_text->vertex_count * 4 * sizeof(float)); // Each vertex has 4 floats
    u32 text_vert_curr = 0;
    for (u32 i = 0; i < char_count; i++) {
        char ch = text_content[i];
        vec2 text_unused = { 0 }; 
        stbtt_aligned_quad quad;
        stbtt_GetBakedQuad(ui->font_char_data, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 
                ch - ' ', &text_unused.x, &text_unused.y, &quad, true);

        // Since the baked bitmap is upside-down in memory (i.e. the glyphs are upside-down)
        // the glyph's top in bitmap is actually it's bottom. Therefore "quad.y0" is how high
        // the quad should be from the baseline
        //
        //                |----|-45
        //   y0|-----|-32 | f  |
        //     |  a  |    |    |
        // --y1|-----|----|----|
        
        // Negate because stbtt-Y is positive downwards
        float gl_quad_top = (-quad.y0) * ui->text_scale; 
        float gl_quad_bottom = (-quad.y1) * ui->text_scale;

        vec2 bottom_left_pos, bottom_left_uv; // Given to GL
        bottom_left_pos.x = text_anchor_ndc.x + (i * text_scale_ndc.x);
        bottom_left_pos.y = text_anchor_ndc.y + (gl_quad_bottom * text_scale_ndc.y); 
        bottom_left_uv.x = quad.s0;
        bottom_left_uv.y = quad.t1; // Notice the flip. Baked bitmap is upside-down in memory

        vec2 top_right_pos, top_right_uv;
        top_right_pos.x = text_anchor_ndc.x + ((i + 1) * text_scale_ndc.x);
        top_right_pos.y = text_anchor_ndc.y + (gl_quad_top * text_scale_ndc.y);
        top_right_uv.x = quad.s1;
        top_right_uv.y = quad.t0;

        // 0 bottom left
        text_buffer[text_vert_curr++] = bottom_left_pos.x;
        text_buffer[text_vert_curr++] = bottom_left_pos.y;
        text_buffer[text_vert_curr++] = bottom_left_uv.x;
        text_buffer[text_vert_curr++] = bottom_left_uv.y;

        // 1 bottom right
        text_buffer[text_vert_curr++] = top_right_pos.x;
        text_buffer[text_vert_curr++] = bottom_left_pos.y;
        text_buffer[text_vert_curr++] = top_right_uv.x;
        text_buffer[text_vert_curr++] = bottom_left_uv.y;

        // 2 top right
        text_buffer[text_vert_curr++] = top_right_pos.x;
        text_buffer[text_vert_curr++] = top_right_pos.y;
        text_buffer[text_vert_curr++] = top_right_uv.x;
        text_buffer[text_vert_curr++] = top_right_uv.y;

        // 0 bottom left
        text_buffer[text_vert_curr++] = bottom_left_pos.x;
        text_buffer[text_vert_curr++] = bottom_left_pos.y;
        text_buffer[text_vert_curr++] = bottom_left_uv.x;
        text_buffer[text_vert_curr++] = bottom_left_uv.y;

        // 2 top right
        text_buffer[text_vert_curr++] = top_right_pos.x;
        text_buffer[text_vert_curr++] = top_right_pos.y;
        text_buffer[text_vert_curr++] = top_right_uv.x;
        text_buffer[text_vert_curr++] = top_right_uv.y;
        
        // 3 top left
        text_buffer[text_vert_curr++] = bottom_left_pos.x;
        text_buffer[text_vert_curr++] = top_right_pos.y;
        text_buffer[text_vert_curr++] = bottom_left_uv.x;
        text_buffer[text_vert_curr++] = top_right_uv.y;
    }

    // Temp -- draw entire atlas
    //#define text_buffer_vertex_count 6
    //float* text_buffer = malloc(text_buffer_vertex_count * 4 * sizeof(float));
    //text_buffer[0]  = 0; text_buffer[1]  = 0; text_buffer[2]  = 0; text_buffer[3]  = 0; // 0
    //text_buffer[4]  = 1; text_buffer[5]  = 0; text_buffer[6]  = 1; text_buffer[7]  = 0; // 1
    //text_buffer[8]  = 1; text_buffer[9]  = 1; text_buffer[10] = 1; text_buffer[11] = 1; // 2
    //text_buffer[12] = 0; text_buffer[13] = 0; text_buffer[14] = 0; text_buffer[15] = 0; // 0
    //text_buffer[16] = 1; text_buffer[17] = 1; text_buffer[18] = 1; text_buffer[19] = 1; // 2
    //text_buffer[20] = 0; text_buffer[21] = 1; text_buffer[22] = 0; text_buffer[23] = 1; // 3

    glGenVertexArrays(1, &(ui_text->vao));
    glGenBuffers(1, &(ui_text->vbo));
    glBindVertexArray(ui_text->vao);

    glBindBuffer(GL_ARRAY_BUFFER, ui_text->vbo);
    glBufferData(GL_ARRAY_BUFFER, ui_text->vertex_count * 4 * sizeof(float), text_buffer, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    free(text_buffer);
}

int main(void) {
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Let's go", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Needs to be after making the gl context current
    // https://gamedev.stackexchange.com/a/73889/81738
    glewInit(); 

    // Another example: https://github.com/shreyaspranav/stb-truetype-example/blob/main/Main.cpp
    ui_t ui = { 0 };
    ui_init(&ui);

    uitext_t ui_text = { 0 };
    vec2 text_anchor_pixels = { 0, 0 };
    vec2 text_scale_pixels = { 100, 100 };
    ui_create_text_static(&ui_text, &ui, "a", text_anchor_pixels, text_scale_pixels);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mesh_t* meshes;
    u32 mesh_count = 0;
    read_obj_file("models/test_lighting.obj", &meshes, &mesh_count);

#define GOS_MAX 10
    gameobject_t gos[GOS_MAX] = { 0 };
    assert(mesh_count < GOS_MAX);
    for (u32 i = 0; i < mesh_count; i++) {
        render_create_buffer(&(gos[i]), &(meshes[i]));
    }

    // Paths need to be relative to the working directory
    // https://stackoverflow.com/a/24597194/4894526
    u32 world_shader = create_shader("src/shader_world_vert.glsl", "src/shader_world_frag.glsl");
    glUseProgram(world_shader);
    glUniform1i(glGetUniformLocation(world_shader, "u_tex"), 0);

    mat44 model = mat44_identity;
    glUniformMatrix4fv(glGetUniformLocation(world_shader, "u_model"), 1, GL_FALSE, model.data);

    vec3 eye = { 0.0f, 0.0f, 2 };
    vec3 up = { 0, 1, 0 };

    vec3 eye_forward = v3_forward;
    vec3 center = v3_add(eye, eye_forward);
    vec3 eye_right = v3_cross(eye_forward, v3_up);
    mat44 view = look_at(eye, center, up);
    glUniformMatrix4fv(glGetUniformLocation(world_shader, "u_view"), 1, GL_FALSE, view.data);

    mat44 proj = perspective(45.0f, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.01f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(world_shader, "u_proj"), 1, GL_FALSE, proj.data);

    float time = (float)glfwGetTime();

    double temp_prev_mouse_x, temp_prev_mouse_y;
    glfwGetCursorPos(window, &temp_prev_mouse_x, &temp_prev_mouse_y);
    float prev_mouse_x = (float)temp_prev_mouse_x;
    float prev_mouse_y = (float)temp_prev_mouse_y;

    while (!glfwWindowShouldClose(window)) {

        float dt = (float)glfwGetTime() - time;
        time = (float)glfwGetTime();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        vec3 move_dir = { 0 };
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            move_dir = v3_add(move_dir, eye_forward);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            move_dir = v3_add(move_dir, v3_neg(eye_right));
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            move_dir = v3_add(move_dir, v3_neg(eye_forward));
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            move_dir = v3_add(move_dir, eye_right);
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            move_dir = v3_add(move_dir, v3_up);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            move_dir = v3_add(move_dir, v3_neg(v3_up));
        }

        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);

        float dx = (float)mouse_x - prev_mouse_x;
        float dy = (float)mouse_y - prev_mouse_y;

        float mouse_sensitivity = 100.0f;
        eye_forward = v3_rotate_around(eye_forward, v3_up, -dx * mouse_sensitivity * dt);
        eye_right = v3_cross(eye_forward, v3_up);
        eye_forward = v3_rotate_around(eye_forward, eye_right, -dy * mouse_sensitivity * dt);

        prev_mouse_x = (float)mouse_x;
        prev_mouse_y = (float)mouse_y;

        if (!v3_iszero(move_dir)) {
            float move_speed = 10.0f;
            move_dir = v3_norm(move_dir);
            vec3 move = v3_scale(move_dir, dt * move_speed);
            eye = v3_add(eye, move);
        }
        center = v3_add(eye, eye_forward);
        view = look_at(eye, center, up);

        //vec3 rotate_euler = { 0.0f, 40.0f * dt, 0.0f };
        //mat44 rotate = euler_to_rot(rotate_euler);
        //model = mat44_mul(&model, &rotate);

        glUseProgram(world_shader);
        glUniformMatrix4fv(glGetUniformLocation(world_shader, "u_view"), 1, GL_FALSE, view.data);
        glUniformMatrix4fv(glGetUniformLocation(world_shader, "u_model"), 1, GL_FALSE, model.data);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        glUseProgram(world_shader);

        for (u32 i = 0; i < GOS_MAX; i++) {
            if (gos[i].vao == 0) continue;
            render_draw_go(&gos[i]);
        }

        glUseProgram(ui.shader);
        glBindVertexArray(ui_text.vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ui.font_bitmap_handle);
        glDrawArrays(GL_TRIANGLES, 0, ui_text.vertex_count);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(world_shader);

    for (u32 i = 0; i < GOS_MAX; i++) {
        if (gos[i].vao == 0) continue;
        render_delete_go(&gos[i]);
    }

    glDeleteVertexArrays(1, &(ui_text.vao));
    glDeleteBuffers(1, &(ui_text.vbo));
    glDeleteProgram(ui.shader);
    glDeleteTextures(1, &ui.font_bitmap_handle);

    glfwTerminate();

    return 0;
}
