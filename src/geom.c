typedef struct {
    float x, y;
} vec2;

typedef struct {
    float x, y, z;
} vec3;

vec3 v3_forward = { 0.0f, 0.0f, -1.0f };
vec3 v3_right = { 1.0f, 0.0f, 0.0f };
vec3 v3_up = { 0.0f, 1.0f, 0.0f };

void v3_print(vec3 v) {
    printf("%f %f %f\n", v.x, v.y, v.z);
}

bool v3_iszero(vec3 v) {
    return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f;
}

vec3 v3_neg(vec3 v) {
    v.x = -(v.x);
    v.y = -(v.y);
    v.z = -(v.z);
    return v;
}

vec3 v3_scale(vec3 v, float f) {
    v.x *= f;
    v.y *= f;
    v.z *= f;

    return v;
}

vec3 v3_add(vec3 a, vec3 b) {
    vec3 retval;
    retval.x = a.x + b.x;
    retval.y = a.y + b.y;
    retval.z = a.z + b.z;
    return retval;
}

vec3 v3_sub(vec3 a, vec3 b) {
    vec3 retval;
    retval.x = a.x - b.x;
    retval.y = a.y - b.y;
    retval.z = a.z - b.z;
    return retval;
}

vec3 v3_norm(vec3 v) {
    if (v3_iszero(v)) { printf("normalizing zero vector\n"); }

    float len = (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    vec3 retval;
    retval.x = v.x / len;
    retval.y = v.y / len;
    retval.z = v.z / len;
    return retval;
}

vec3 v3_cross(vec3 a, vec3 b) {
    vec3 retval;
    retval.x = a.y * b.z - a.z * b.y;
    retval.y = -(a.x * b.z - a.z * b.x);
    retval.z = a.x * b.y - a.y * b.x;
    return retval;
}

float v3_dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 v3_rotate_around(vec3 v, vec3 axis, float angle) {
    float angle_r = angle * DEG2RAD;
    float cos_angle = (float)cos(angle_r);
    float sin_angle = (float)sin(angle_r);

    // Rodrigues rotation formula
    vec3 a = v3_scale(v, cos_angle);
    vec3 b = v3_scale(v3_cross(axis, v), sin_angle);
    vec3 c = v3_scale(axis, v3_dot(axis, v) * (1 - cos_angle));
    return v3_add(a, v3_add(b, c));
}

typedef struct {
    float data[16]; // Column-major: https://stackoverflow.com/a/19253305/4894526
} mat44;

static mat44 mat44_identity = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

mat44 mat44_mul(mat44* m1, mat44* m2) { // Column-major
    mat44 m = { 0 };
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                m.data[j * 4 + i] += m1->data[k * 4 + i] * m2->data[j * 4 + k];
            }
        }
    }
    return m;
}

mat44 euler_to_rot(vec3 euler) {
    mat44 m = { 0 };

    const float cos_x = (float)cos(euler.x * DEG2RAD);
    const float cos_y = (float)cos(euler.y * DEG2RAD);
    const float cos_z = (float)cos(euler.z * DEG2RAD);

    const float sin_x = (float)sin(euler.x * DEG2RAD);
    const float sin_y = (float)sin(euler.y * DEG2RAD);
    const float sin_z = (float)sin(euler.z * DEG2RAD);

    m.data[0 * 4 + 0] = cos_y * cos_z;
    m.data[1 * 4 + 0] = cos_y * sin_z;
    m.data[2 * 4 + 0] = -sin_y;
    m.data[3 * 4 + 0] = 0.0f;
    m.data[0 * 4 + 1] = sin_x * sin_y * cos_z - cos_x * sin_z;
    m.data[1 * 4 + 1] = sin_x * sin_y * sin_z + cos_x * cos_z;
    m.data[2 * 4 + 1] = sin_x * cos_y;
    m.data[3 * 4 + 1] = 0.0f;
    m.data[0 * 4 + 2] = cos_x * sin_y * cos_z + sin_x * sin_z;
    m.data[1 * 4 + 2] = cos_x * sin_y * sin_z - sin_x * cos_z;
    m.data[2 * 4 + 2] = cos_x * cos_y;
    m.data[3 * 4 + 2] = 0.0f;
    m.data[0 * 4 + 3] = 0.0f;
    m.data[1 * 4 + 3] = 0.0f;
    m.data[2 * 4 + 3] = 0.0f;
    m.data[3 * 4 + 3] = 1.0f;

    return m;
}

mat44 look_at(vec3 eye, vec3 center, vec3 up) {
    mat44 m = { 0 };

    vec3 forward = v3_norm(v3_sub(center, eye));
    vec3 right = v3_norm(v3_cross(forward, up));
    vec3 local_up = v3_cross(right, forward);

    m.data[0 * 4 + 0] = right.x;
    m.data[0 * 4 + 1] = local_up.x;
    m.data[0 * 4 + 2] = -forward.x;
    m.data[0 * 4 + 3] = 0.0f;

    m.data[1 * 4 + 0] = right.y;
    m.data[1 * 4 + 1] = local_up.y;
    m.data[1 * 4 + 2] = -forward.y;
    m.data[1 * 4 + 3] = 0.0f;

    m.data[2 * 4 + 0] = right.z;
    m.data[2 * 4 + 1] = local_up.z;
    m.data[2 * 4 + 2] = -forward.z;
    m.data[2 * 4 + 3] = 0.0f;

    m.data[3 * 4 + 0] = -v3_dot(eye, right);
    m.data[3 * 4 + 1] = -v3_dot(eye, local_up);
    m.data[3 * 4 + 2] = v3_dot(eye, forward); // Not negative!
    m.data[3 * 4 + 3] = 1.0f;

    return m;
}

mat44 ortho(float left, float right, float bottom, float top, float near, float far) {
    mat44 m = mat44_identity;
    m.data[0 * 4 + 0] = 2 / (right - left);
    m.data[1 * 4 + 1] = 2 / (top - bottom);
    m.data[2 * 4 + 2] = -2 / (far - near);
    m.data[3 * 4 + 0] = -(right + left) / (right - left);
    m.data[3 * 4 + 1] = -(top + bottom) / (top - bottom);
    m.data[3 * 4 + 2] = -(far + near) / (far - near);
    return m;
}

mat44 perspective(float fov, float aspect_ratio, float near, float far) {
    mat44 m = { 0 };
    float tan_half_fov = (float)tan(fov * 0.5f * DEG2RAD);

    m.data[0 * 4 + 0] = 1.0f / (aspect_ratio * tan_half_fov);
    m.data[1 * 4 + 1] = 1.0f / tan_half_fov;
    m.data[2 * 4 + 2] = -(far + near) / (far - near);
    m.data[2 * 4 + 3] = -1.0f;
    m.data[3 * 4 + 2] = -(2.0f * far * near) / (far - near);

    return m;
}
