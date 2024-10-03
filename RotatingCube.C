#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>

#define WIDTH 76
#define HEIGHT 38
#define LENGTH(v) sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2])
#define NORMALIZE(out, v) {float _l=LENGTH(v); out[0] = v[0] / _l; out[1] = v[1] / _l; out[2] = v[2] / _l;}
#define CROSS(out, v0, v1) {out[0]=v0[1]*v1[2]-v0[2]*v1[1]; out[1]=v0[2]*v1[0]-v0[0]*v1[2]; out[2]=v0[0]*v1[1]-v0[1]*v1[0];}
#define DOT(v0, v1) (v0[0]*v1[0]+v0[1]*v1[1]+v0[2]*v1[2])
#define IDENTITY {{1.0f,0.0f,0.0f,0.0f},{0.0f,1.0f,0.0f,0.0f},{0.0f,0.0f,1.0f,0.0f},{0.0f,0.0f,0.0f,1.0f}}

typedef float vec[4];
typedef float mat[4][4];

char chars[] = " `^\",:;Il!i~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
float z_buf[WIDTH * HEIGHT];
float color_buf[WIDTH * HEIGHT];
vec vertices[8] = {{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
vec tex_coords[4] = {{0,0},{1,0},{1,1},{0,1}};
vec normals[6] = {{0,0,1},{1,0,0},{0,0,-1},{-1,0,0},{0,1,0},{0,-1,0}};
int indices[36] = {0,1,3,3,1,2,1,5,2,2,5,6,5,4,6,6,4,7,4,0,7,7,0,3,3,2,7,7,2,6,4,5,0,0,5,1};
int tex_coords_indices[6] = {0,1,3,3,1,2};

float edge(vec a, vec b, vec c) {
    return (c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0]);
}

void triangle(vec v0, vec v1, vec v2, vec tc0, vec tc1, vec tc2) {
    v0[0] /= v0[3]; v0[1] /= v0[3]; v0[2] /= v0[3];
    v1[0] /= v1[3]; v1[1] /= v1[3]; v1[2] /= v1[3];
    v2[0] /= v2[3]; v2[1] /= v2[3]; v2[2] /= v2[3];
    v0[0] = roundf((v0[0] + 1.0f) / 2.0f * (WIDTH - 1)); v0[1] = roundf((v0[1] + 1.0f) / 2.0f * (HEIGHT - 1));
    v1[0] = roundf((v1[0] + 1.0f) / 2.0f * (WIDTH - 1)); v1[1] = roundf((v1[1] + 1.0f) / 2.0f * (HEIGHT - 1));
    v2[0] = roundf((v2[0] + 1.0f) / 2.0f * (WIDTH - 1)); v2[1] = roundf((v2[1] + 1.0f) / 2.0f * (HEIGHT - 1));
    float area = edge(v0, v1, v2);
    for (int j = 0; j < HEIGHT; ++j) {
        for (int i = 0; i < WIDTH; ++i) {
            vec p = {i + 0.5f, j + 0.5f};
            float w0 = edge(v1, v2, p);     float w1 = edge(v2, v0, p);     float w2 = edge(v0, v1, p);
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                w0 /= area;     w1 /= area;     w2 /= area;
                float z = w0 * v0[2] + w1 * v1[2] + w2 * v2[2];
                if (z > z_buf[j * WIDTH + i]) {
                    z_buf[j * WIDTH + i] = z;
                    float s = w0 * tc0[0] + w1 * tc1[0] + w2 * tc2[0];
                    float t = w0 * tc0[1] + w1 * tc1[1] + w2 * tc2[1];
                    float M = 1.0f;
                    float pattern = (fmod(s * M, 1.0) > 0.5) ^ (fmod(t * M, 1.0) < 0.5);
                    if (pattern == 0) pattern = 0.1f;
                    color_buf[j * WIDTH + i] = pattern;
                }
            }
        }
    }
}

void perspective(mat m, float aspect, float fov, float near, float far) {
    memset(m, 0, 16 * sizeof(float));
    float half_tan = tanf(fov / 2);
    m[0][0] = 1 / (half_tan * aspect);      m[1][1] = 1 / half_tan;
    m[2][2] = -(far + near) / (far - near); m[2][3] = -1;
    m[3][2] = -(2 * far * near) / (far - near);
}

void look_at(mat m, vec pos, vec target, vec up) {
    memset(m, 0, 16 * sizeof(float));
    vec diff = {target[0] - pos[0], target[1] - pos[1], target[2] - pos[2]};
    vec forward;    NORMALIZE(forward, diff);
    vec right;      CROSS(right, forward, up);          NORMALIZE(right, right);
    vec local_up;   CROSS(local_up, right, forward);    NORMALIZE(local_up, local_up);
    m[0][0] = right[0];         m[1][0] = right[1];             m[2][0] = right[2];
    m[0][1] = local_up[0];      m[1][1] = local_up[1];          m[2][1] = local_up[2];
    m[0][2] = -forward[0];      m[1][2] = -forward[1];          m[2][2] = -forward[2];
    m[3][0] = -DOT(right, pos); m[3][1] = -DOT(local_up, pos);  m[3][2] = DOT(forward, pos);    m[3][3] = 1.0f;
}

void vec_mul_mat(vec v, mat m, vec out) {
    for (int y = 0; y < 4; y++) {
        float i = 0;
        for (int x = 0; x < 4; x++)
            i += m[x][y] * v[x];
        out[y] = i;
    }
}

void mat_mul_mat(mat m0, mat m1, mat out) {
    memset(out, 0, 16 * sizeof(float));
    for (int x = 0; x < 4; x++)
        for (int y = 0; y < 4; y++)
            for (int i = 0; i < 4; i++)
                out[x][y] += m0[x][i] * m1[i][y];
}

void rotate_x(mat m, float angle) {
    mat x = IDENTITY;
    x[1][1] = cosf(angle);  x[1][2] = -sinf(angle);
    x[2][1] = sinf(angle);  x[2][2] = cosf(angle);
    mat temp;
    memcpy(temp, m, 16 * sizeof(float));
    mat_mul_mat(temp, x, m);
}

void rotate_y(mat m, float angle) {
    mat y = IDENTITY;
    y[0][0] = cosf(angle);  y[0][2] = sinf(angle);
    y[2][0] = -sinf(angle); y[2][2] = cosf(angle);
    mat temp;
    memcpy(temp, m, 16 * sizeof(float));
    mat_mul_mat(temp, y, m);
}

void clear() {
    printf("\033[u");
    memset(z_buf, 0, WIDTH * HEIGHT * sizeof(float));
    memset(color_buf, 0, WIDTH * HEIGHT * sizeof(float));
}

int main() {
    printf("\033[s");
    mat proj, view;
    perspective(proj, HEIGHT / (float)WIDTH * 2.0f, 70.0f, 0.0001f, 1000.0f);
    look_at(view, (vec){-2.3f, 2.3f, -2.3f}, (vec){0.0f, 0.0f, 0.0f}, (vec){0.0f, -1.0f, 0.0f});
    vec vertex_buf[36];
    vec tex_coord_buf[36];
    for (int i = 0; i < 36; i++) {
        memcpy(vertex_buf[i], vertices[indices[i]], 3 * sizeof(float));
        vertex_buf[i][3] = 1.0f;
        memcpy(tex_coord_buf[i], tex_coords[tex_coords_indices[i % 6]], 2 * sizeof(float));
    }
    while (1) {
        clear();
        clock_t uptime = clock() / (CLOCKS_PER_SEC / 1000);
        mat model = IDENTITY, vp, mvp;
        rotate_y(model, (float)(uptime * M_PI / 3000));
        rotate_x(model, (float)(uptime * M_PI / 5000));
        mat_mul_mat(view, proj, vp);
        mat_mul_mat(model, vp, mvp);
        for (int i = 0; i < 12; i++) {
            vec tri[3];
            for (int j = 0; j < 3; j++)
                vec_mul_mat(vertex_buf[i * 3 + j], mvp, tri[j]);
            triangle(tri[0], tri[1], tri[2], tex_coord_buf[i * 3 + 0], tex_coord_buf[i * 3 + 1], tex_coord_buf[i * 3 + 2]);
        }
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                float color = color_buf[y * WIDTH + x];
                int i = (int)((strlen(chars) - 1) * color);
                putchar(chars[i]);
            }
            putchar('\n');
        }
    }
}
