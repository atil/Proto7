char* read_entire_file(char* file_name) { 
    // taken from: https://stackoverflow.com/a/14002993/4894526
    
    // Need to read as binary "rb". If we read it as text (i.e. "r"),
    // then Windows line endings "\r\n" are converted to "\n" on `fread`
    // but `ftell` counts them, since they're part of the file.
    // https://stackoverflow.com/a/23690485/4894526
    FILE* f = fopen(file_name, "rb");

    if (f == NULL) {
        printf("file not found: %s\n", file_name);
        assert(false);
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

void append_prefix(char* str, const char* prefix, u64 max_len, char* result) {
    u64 len_prefix = strlen(prefix);
    strcpy_s(result, max_len, prefix);
    strcat_s(result + len_prefix, max_len - len_prefix, str);
}

void read_mtl_file(char* filename, mtlasset_t* mtl_asset) {
    memset(mtl_asset, 0, sizeof(mtlasset_t));

    char mtl_file_path[MTL_FILENAME_LEN] = { 0 };
    append_prefix(filename, "models/", MTL_FILENAME_LEN, mtl_file_path);

    char* mtl_next_line_char;
    char* mtl_file_content = read_entire_file(mtl_file_path);
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

void read_obj_file(char* filename, mesh_t** pp_meshes, u32* mesh_count) {
    char* obj_file_content = read_entire_file(filename);
    objasset_t obj_asset = { 0 };

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
    obj_asset.subs = malloc(mtl_count * sizeof(objsubdata_t));
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

            obj_asset.subs[i_curr_mtl].face_data[i_curr_face++] = face_pos_0;
            obj_asset.subs[i_curr_mtl].face_data[i_curr_face++] = face_uv_0;
            obj_asset.subs[i_curr_mtl].face_data[i_curr_face++] = face_norm_0;
            obj_asset.subs[i_curr_mtl].face_data[i_curr_face++] = face_pos_1;
            obj_asset.subs[i_curr_mtl].face_data[i_curr_face++] = face_uv_1;
            obj_asset.subs[i_curr_mtl].face_data[i_curr_face++] = face_norm_1;
            obj_asset.subs[i_curr_mtl].face_data[i_curr_face++] = face_pos_2;
            obj_asset.subs[i_curr_mtl].face_data[i_curr_face++] = face_uv_2;
            obj_asset.subs[i_curr_mtl].face_data[i_curr_face++] = face_norm_2;

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
            obj_asset.subs[i_curr_mtl].face_count = face_count_this_mtl;
            obj_asset.subs[i_curr_mtl].face_data = malloc(face_count_this_mtl * 9 * sizeof(u32));
            strcpy_s(obj_asset.subs[i_curr_mtl].mtl_name, MTL_NAME_LEN, mtl_curr_name);
        }

        line = strtok_s(NULL, "\n", &obj_file_content_rest);
    } 

    // Each mtl means a mesh
    *mesh_count = (i_curr_mtl + 1);
    *pp_meshes = malloc((*mesh_count) * sizeof(mesh_t));

    mesh_t* p_meshes = (*pp_meshes);

    for (u32 i_mtl = 0; i_mtl < mtl_count; i_mtl++) {
        objsubdata_t* curr_sub = &obj_asset.subs[i_mtl];
        mesh_t* p_curr_mesh = &(p_meshes[i_mtl]);

        u32 mtl_vertex_count = curr_sub->face_count * 3; // Each face is a triangle
        p_curr_mesh->vertex_count = mtl_vertex_count;
        p_curr_mesh->vertex_data = malloc(mtl_vertex_count * 8 * sizeof(float));

        mtldata_t* p_face_mat = NULL;
        for (u32 i = 0; i < mtl_asset.mtl_count; i++) {
            if (strcmp(mtl_asset.materials[i].name, curr_sub->mtl_name) == 0) {
                p_face_mat = &(mtl_asset.materials[i]);
                break;
            }
        }
        assert(p_face_mat);

        append_prefix(p_face_mat->texture_name, "textures/", MTL_TEXTURE_FILENAME_LEN, p_curr_mesh->texture_name);

        u32 i_vertex_data = 0;
        for (u32 i_face = 0; i_face < curr_sub->face_count; i_face++) {

            // Single triangle (face data is 1-based, hence the "- 1")

            u32 i_pos0    = curr_sub->face_data[i_face * 9 + 0] - 1;
            u32 i_uv0     = curr_sub->face_data[i_face * 9 + 1] - 1;
            u32 i_normal0 = curr_sub->face_data[i_face * 9 + 2] - 1;

            u32 i_pos1    = curr_sub->face_data[i_face * 9 + 3] - 1;
            u32 i_uv1     = curr_sub->face_data[i_face * 9 + 4] - 1;
            u32 i_normal1 = curr_sub->face_data[i_face * 9 + 5] - 1;

            u32 i_pos2    = curr_sub->face_data[i_face * 9 + 6] - 1;
            u32 i_uv2     = curr_sub->face_data[i_face * 9 + 7] - 1;
            u32 i_normal2 = curr_sub->face_data[i_face * 9 + 8] - 1;

            // Vertex data format: v,v,v, u,u, n,n,n

            // 1st vertex
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos0 * 3 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos0 * 3 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos0 * 3 + 2];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.uvs[i_uv0 * 2 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.uvs[i_uv0 * 2 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal0 * 3 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal0 * 3 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal0 * 3 + 2];

            // 2nd vertex
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos1 * 3 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos1 * 3 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos1 * 3 + 2];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.uvs[i_uv1 * 2 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.uvs[i_uv1 * 2 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal1 * 3 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal1 * 3 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal1 * 3 + 2];

            // 3rd vertex
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos2 * 3 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos2 * 3 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.positions[i_pos2 * 3 + 2];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.uvs[i_uv2 * 2 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.uvs[i_uv2 * 2 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal2 * 3 + 0];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal2 * 3 + 1];
            p_curr_mesh->vertex_data[i_vertex_data++] = obj_asset.normals[i_normal2 * 3 + 2];
            
        }
    }


    free(obj_file_content);
    free(obj_asset.positions);
    free(obj_asset.uvs);
    free(obj_asset.normals);
    for (u32 i = 0; i < obj_asset.mtl_count; i++) {
        free(obj_asset.subs[i].face_data);
    }
    free(obj_asset.subs);
}

