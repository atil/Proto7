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

void read_obj_file(char* obj_file_name, obj_t* obj_data) {
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

    char* dummy;
    char* line = strtok_s(obj_file_content, "\n", &dummy);
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
        }
        line = strtok_s(NULL, "\n", &dummy);
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
