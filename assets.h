char* read_entire_file(char* file_name) { 
    // taken from: https://stackoverflow.com/a/14002993/4894526
    
    // Need to read as binary "rb". If we read it as text (i.e. "r"),
    // then Windows line endings "\r\n" are converted to "\n" on `fread`
    // but `ftell` counts them, since they're part of the file.
    // https://stackoverflow.com/a/23690485/4894526
    FILE* f = fopen(file_name, "rb");

    if (f == NULL) {
        printf("file not found: %s\n", file_name);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    i32 file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buffer = malloc((file_size + 1) * sizeof(char));
    fread(buffer, file_size, 1, f);
    fclose(f);

    buffer[file_size] = 0;

    return buffer;
}

void read_mtl_file(char* filename, mtlasset_t* mtl_asset) {
    memset(mtl_asset, 0, sizeof(mtlasset_t));

    char* mtl_next_line_char;
    char* mtl_file_content = read_entire_file(filename);
    char* mtl_line = strtok_s(mtl_file_content, "\n", &mtl_next_line_char);
    i32 i_curr_mtl = -1;
    while(mtl_line != NULL) {

        if (strncmp(mtl_line, "newmtl", 6) == 0) {
            i_curr_mtl++;
            sscanf_s(mtl_line, "newmtl %s", &(mtl_asset->materials[i_curr_mtl].name), MTL_NAME_LEN);
        }
        else if (strncmp(mtl_line, "map_Kd", 6) == 0) {
            sscanf_s(mtl_line, "map_Kd %s", &(mtl_asset->materials[i_curr_mtl].texture_name), MTL_TEXTURE_FILENAME_LEN);
        }

        mtl_line = strtok_s(NULL, "\n", &mtl_next_line_char);
    }
    mtl_asset->mtl_count = (i_curr_mtl + 1);

    free(mtl_file_content);
}


void read_obj_file(char* obj_file_name, mesh_t* obj_data) {
    char* obj_file_content = read_entire_file(obj_file_name);

    u32 position_count = 0;
    u32 triangle_count = 0;
    u32 uv_count = 0;
    u32 normal_count = 0;

    // Avoiding having another copy of the file contents to use strtok
    // by counting the substrings. Careful, this wouldn't ignore the comments
    // https://stackoverflow.com/a/9052540/4894526
    char* tmp = obj_file_content;
    while ((tmp = strstr(tmp, "v "))) { tmp++; position_count++; }
    tmp = obj_file_content;
    while ((tmp = strstr(tmp, "f "))) { tmp++; triangle_count++; }
    tmp = obj_file_content;
    while ((tmp = strstr(tmp, "vt "))) { tmp++; uv_count++; }
    tmp = obj_file_content;
    while ((tmp = strstr(tmp, "vn "))) { tmp++; normal_count++; }

    //
    // Parse data out of the file
    // 

    float* positions = malloc(position_count * 3 * sizeof(float));
    u32 positions_index = 0;
    float* uvs = malloc(uv_count * 2 * sizeof(float));
    u32 uvs_index = 0;
    float* normals = malloc(normal_count * 3 * sizeof(float));
    u32 normals_index = 0;
    u32* triangles = malloc(triangle_count * 9 * sizeof(u32));
    u32 triangle_index = 0;

    char* next_line;
    char* line = strtok_s(obj_file_content, "\n", &next_line);
    while (line != NULL) {
        if (strncmp(line, "vt", 2) == 0) {
            float u, v;
            sscanf_s(line, "vt %f %f", &u, &v);
            uvs[uvs_index++] = u;
            uvs[uvs_index++] = v;
        } else if (strncmp(line, "vn", 2) == 0) {
            float norm_x, norm_y, norm_z;
            sscanf_s(line, "vn %f %f %f", &norm_x, &norm_y, &norm_z);
            normals[normals_index++] = norm_x;
            normals[normals_index++] = norm_y;
            normals[normals_index++] = norm_z;
        } else if (strncmp(line, "v", 1) == 0) {
            float pos_x, pos_y, pos_z;
            sscanf_s(line, "v %f %f %f", &pos_x, &pos_y, &pos_z);
            positions[positions_index++] = pos_x;
            positions[positions_index++] = pos_y;
            positions[positions_index++] = pos_z;
        } else if (strncmp(line, "f", 1) == 0) {
            u32 triangle_pos_0, triangle_pos_1, triangle_pos_2;
            u32 triangle_uv_0, triangle_uv_1, triangle_uv_2;
            u32 triangle_norm_0, triangle_norm_1, triangle_norm_2;
            sscanf_s(line, "f %u/%u/%u %u/%u/%u %u/%u/%u", 
                    &triangle_pos_0, &triangle_uv_0, &triangle_norm_0,
                    &triangle_pos_1, &triangle_uv_1, &triangle_norm_1,
                    &triangle_pos_2, &triangle_uv_2, &triangle_norm_2);
            triangles[triangle_index++] = triangle_pos_0;
            triangles[triangle_index++] = triangle_uv_0;
            triangles[triangle_index++] = triangle_norm_0;
            triangles[triangle_index++] = triangle_pos_1;
            triangles[triangle_index++] = triangle_uv_1;
            triangles[triangle_index++] = triangle_norm_1;
            triangles[triangle_index++] = triangle_pos_2;
            triangles[triangle_index++] = triangle_uv_2;
            triangles[triangle_index++] = triangle_norm_2;
        } else if (strncmp(line, "mtllib", 6) == 0) {
            char mtl_filename[MTL_FILENAME_LEN];
            sscanf_s(line, "mtllib %s", mtl_filename, MTL_FILENAME_LEN);

            char* mtl_next_line_char;
            char* mtl_file_content = read_entire_file(mtl_filename);
            char* mtl_line = strtok_s(mtl_file_content, "\n", &mtl_next_line_char);
            while(mtl_line != NULL) {
                if (strncmp(mtl_line, "map_Kd", 6) == 0) {
                    sscanf_s(mtl_line, "map_Kd %s", obj_data->texture_name, MTL_TEXTURE_FILENAME_LEN);
                }
                mtl_line = strtok_s(NULL, "\n", &mtl_next_line_char);
            }

            free(mtl_file_content);

        }
        line = strtok_s(NULL, "\n", &next_line);
    } 

    //
    // Filling in OpenGL-ready data

    u32 sizeof_vertex = 8 * sizeof(float);
    obj_data->vertex_data = malloc(triangle_count * 3 * sizeof_vertex);
    u32 i_vertdata = 0;
    for (triangle_index = 0; triangle_index < triangle_count; triangle_index++) {
        u32 index_position, index_uv, index_normal;

        // 1st vertex
        index_position = triangles[9 * triangle_index + 0] - 1;
        obj_data->vertex_data[i_vertdata++] = positions[3 * index_position + 0];
        obj_data->vertex_data[i_vertdata++] = positions[3 * index_position + 1];
        obj_data->vertex_data[i_vertdata++] = positions[3 * index_position + 2];

        index_uv = triangles[9 * triangle_index + 1] - 1;
        obj_data->vertex_data[i_vertdata++] = uvs[2 * index_uv + 0];
        obj_data->vertex_data[i_vertdata++] = uvs[2 * index_uv + 1];

        index_normal = triangles[9 * triangle_index + 2] - 1;
        obj_data->vertex_data[i_vertdata++] = normals[3 * index_normal + 0];
        obj_data->vertex_data[i_vertdata++] = normals[3 * index_normal + 1];
        obj_data->vertex_data[i_vertdata++] = normals[3 * index_normal + 2];

        // 2nd vertex
        index_position = triangles[9 * triangle_index + 3] - 1;
        obj_data->vertex_data[i_vertdata++] = positions[3 * index_position + 0];
        obj_data->vertex_data[i_vertdata++] = positions[3 * index_position + 1];
        obj_data->vertex_data[i_vertdata++] = positions[3 * index_position + 2];

        index_uv = triangles[9 * triangle_index + 4] - 1;
        obj_data->vertex_data[i_vertdata++] = uvs[2 * index_uv + 0];
        obj_data->vertex_data[i_vertdata++] = uvs[2 * index_uv + 1];

        index_normal = triangles[9 * triangle_index + 5] - 1;
        obj_data->vertex_data[i_vertdata++] = normals[3 * index_normal + 0];
        obj_data->vertex_data[i_vertdata++] = normals[3 * index_normal + 1];
        obj_data->vertex_data[i_vertdata++] = normals[3 * index_normal + 2];

        // 3rd vertex
        index_position = triangles[9 * triangle_index + 6] - 1;
        obj_data->vertex_data[i_vertdata++] = positions[3 * index_position + 0];
        obj_data->vertex_data[i_vertdata++] = positions[3 * index_position + 1];
        obj_data->vertex_data[i_vertdata++] = positions[3 * index_position + 2];

        index_uv = triangles[9 * triangle_index + 7] - 1;
        obj_data->vertex_data[i_vertdata++] = uvs[2 * index_uv + 0];
        obj_data->vertex_data[i_vertdata++] = uvs[2 * index_uv + 1];

        index_normal = triangles[9 * triangle_index + 8] - 1;
        obj_data->vertex_data[i_vertdata++] = normals[3 * index_normal + 0];
        obj_data->vertex_data[i_vertdata++] = normals[3 * index_normal + 1];
        obj_data->vertex_data[i_vertdata++] = normals[3 * index_normal + 2];
    }

    obj_data->vertex_count = triangle_count * 3; // Triangles

    free(obj_file_content);
    free(positions);
    free(uvs);
    free(normals);
}

void read_obj_file2(char* filename, mesh_t** pp_meshes, u32* mesh_count) {
    char* obj_file_content = read_entire_file(filename);
    objasset_t obj_asset;

    u32 position_count = 0;
    u32 uv_count = 0;
    u32 normal_count = 0;
    u32 mtl_count = 0;

    // Avoiding having another copy of the file contents to use strtok
    // by counting the substrings. Careful, this wouldn't ignore the comments
    // https://stackoverflow.com/a/9052540/4894526
    char* tmp = obj_file_content;
    while ((tmp = strstr(tmp, "v "))) { tmp++; position_count++; }
    tmp = obj_file_content;
    while ((tmp = strstr(tmp, "vt "))) { tmp++; uv_count++; }
    tmp = obj_file_content;
    while ((tmp = strstr(tmp, "vn "))) { tmp++; normal_count++; }
    tmp = obj_file_content;
    while ((tmp = strstr(tmp, "usemtl "))) { tmp++; mtl_count++; }

    //
    // Parse data out of the file
    // 

    obj_asset.positions = malloc(position_count * 3 * sizeof(float));
    obj_asset.uvs = malloc(uv_count * 2 * sizeof(float));
    obj_asset.normals = malloc(normal_count * 3 * sizeof(float));
    obj_asset.faces = malloc(mtl_count * sizeof(objfacedata_t));
    u32 positions_index = 0;
    u32 uvs_index = 0;
    u32 normals_index = 0;

    mtlasset_t mtl_asset = { 0 };
    char mtl_curr_name[MTL_NAME_LEN];
    i32 i_curr_mtl = -1;
    u32 i_curr_face = 0;

    char* obj_file_content_rest;
    char* line = strtok_s(obj_file_content, "\n", &obj_file_content_rest);
    while (line != NULL) {
        if (strncmp(line, "vt", 2) == 0) {
            float u, v;
            sscanf_s(line, "vt %f %f", &u, &v);
            obj_asset.uvs[uvs_index++] = u;
            obj_asset.uvs[uvs_index++] = v;
        } else if (strncmp(line, "vn", 2) == 0) {
            float norm_x, norm_y, norm_z;
            sscanf_s(line, "vn %f %f %f", &norm_x, &norm_y, &norm_z);
            obj_asset.normals[normals_index++] = norm_x;
            obj_asset.normals[normals_index++] = norm_y;
            obj_asset.normals[normals_index++] = norm_z;
        } else if (strncmp(line, "v", 1) == 0) {
            float pos_x, pos_y, pos_z;
            sscanf_s(line, "v %f %f %f", &pos_x, &pos_y, &pos_z);
            obj_asset.positions[positions_index++] = pos_x;
            obj_asset.positions[positions_index++] = pos_y;
            obj_asset.positions[positions_index++] = pos_z;
        } else if (strncmp(line, "f", 1) == 0) {

            u32 face_pos_0,  face_pos_1,  face_pos_2;
            u32 face_uv_0,   face_uv_1,   face_uv_2;
            u32 face_norm_0, face_norm_1, face_norm_2;
            sscanf_s(line, "f %u/%u/%u %u/%u/%u %u/%u/%u", 
                    &face_pos_0, &face_uv_0, &face_norm_0,
                    &face_pos_1, &face_uv_1, &face_norm_1,
                    &face_pos_2, &face_uv_2, &face_norm_2);

            obj_asset.faces[i_curr_mtl].face_data[i_curr_face++] = face_pos_0;
            obj_asset.faces[i_curr_mtl].face_data[i_curr_face++] = face_uv_0;
            obj_asset.faces[i_curr_mtl].face_data[i_curr_face++] = face_norm_0;
            obj_asset.faces[i_curr_mtl].face_data[i_curr_face++] = face_pos_1;
            obj_asset.faces[i_curr_mtl].face_data[i_curr_face++] = face_uv_1;
            obj_asset.faces[i_curr_mtl].face_data[i_curr_face++] = face_norm_1;
            obj_asset.faces[i_curr_mtl].face_data[i_curr_face++] = face_pos_2;
            obj_asset.faces[i_curr_mtl].face_data[i_curr_face++] = face_uv_2;
            obj_asset.faces[i_curr_mtl].face_data[i_curr_face++] = face_norm_2;

        } else if (strncmp(line, "mtllib", 6) == 0) {
            char mtl_filename[MTL_FILENAME_LEN];
            sscanf_s(line, "mtllib %s", mtl_filename, MTL_FILENAME_LEN);
            read_mtl_file(mtl_filename, &mtl_asset);

        } else if (strncmp(line, "usemtl", 6) == 0) {
            sscanf_s(line, "usemtl %s", mtl_curr_name, MTL_FILENAME_LEN);

            u32 face_count_this_mtl = 0;
            for (char* ch = obj_file_content_rest; *ch != '\0' && strncmp(ch, "usemtl", 6) != 0; ch++) {
                if (strncmp(ch, "f ", 2) == 0) {
                    face_count_this_mtl++;
                }
            }

            i_curr_mtl++;
            i_curr_face = 0;
            obj_asset.faces[i_curr_mtl].face_count = face_count_this_mtl;
            obj_asset.faces[i_curr_mtl].face_data = malloc(face_count_this_mtl * 9 * sizeof(u32));
            strcpy(obj_asset.faces[i_curr_mtl].mtl_name, mtl_curr_name);
        }

        line = strtok_s(NULL, "\n", &obj_file_content_rest);
    } 

    // Each mtl means a mesh
    *mesh_count = (i_curr_mtl + 1);
    *pp_meshes = malloc((*mesh_count) * sizeof(mesh_t));

    mesh_t* p_meshes = (*pp_meshes);

    for (u32 i_mtl = 0; i_mtl < mtl_count; i_mtl++) {
        objfacedata_t* curr_face = &obj_asset.faces[i_mtl];
        mesh_t* p_curr_mesh = &(p_meshes[i_mtl]);

        u32 mtl_vertex_count = curr_face->face_count * 3; // Each face is a triangle
        p_curr_mesh->vertex_count = mtl_vertex_count;
        p_curr_mesh->vertex_data = malloc(mtl_vertex_count * 8 * sizeof(float));

        mtldata_t* p_face_mat = NULL;
        for (u32 i = 0; i < mtl_asset.mtl_count; i++) {
            if (strcmp(mtl_asset.materials[i].name, curr_face->mtl_name) == 0) {
                p_face_mat = &(mtl_asset.materials[i]);
                break;
            }
        }
        assert(p_face_mat);

        strcpy(p_curr_mesh->texture_name, p_face_mat->texture_name);

        u32 i_vertex_data = 0;
        for (u32 i_face = 0; i_face < curr_face->face_count; i_face++) {

            // Single triangle (face data is 1-based, hence the "- 1")

            u32 i_pos0    = curr_face->face_data[i_face + 0] - 1;
            u32 i_uv0     = curr_face->face_data[i_face + 1] - 1;
            u32 i_normal0 = curr_face->face_data[i_face + 2] - 1;

            u32 i_pos1    = curr_face->face_data[i_face + 3] - 1;
            u32 i_uv1     = curr_face->face_data[i_face + 4] - 1;
            u32 i_normal1 = curr_face->face_data[i_face + 5] - 1;

            u32 i_pos2    = curr_face->face_data[i_face + 6] - 1;
            u32 i_uv2     = curr_face->face_data[i_face + 7] - 1;
            u32 i_normal2 = curr_face->face_data[i_face + 8] - 1;

            // Vertex data format: v,v,v, u,u, n,n,n

            // 1st vertex
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos0 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos0 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos0 + 2];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.uvs[i_uv0 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.uvs[i_uv0 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal0 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal0 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal0 + 2];

            // 2nd vertex
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos1 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos1 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos1 + 2];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.uvs[i_uv1 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.uvs[i_uv1 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal1 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal1 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal1 + 2];

            // 3rd vertex
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos2 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos2 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos2 + 2];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.uvs[i_uv2 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.uvs[i_uv2 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal2 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal2 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal2 + 2];
            
        }
    }


    free(obj_file_content);
    free(obj_asset.positions);
    free(obj_asset.uvs);
    free(obj_asset.normals);
    // TODO free the face_datas here as well
    free(obj_asset.faces);
}

