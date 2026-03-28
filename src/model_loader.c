// src/model_loader.c

#include "model_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libdragon.h>
#include <math.h>

model_t load_obj_model(const char *path) {
  model_t model = {0};

  FILE *f = fopen(path, "r");
  if (!f) {
    printf("Could not open model: %s\n", path);
    return model;
  }

  int v_count = 0, f_count = 0;
  char line[256];
  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "v ", 2) == 0)
      v_count++;
    if (strncmp(line, "f ", 2) == 0)
      f_count++;
  }

  model.vertices = malloc(sizeof(float) * 3 * v_count);
  if (!model.vertices) {
    printf("Could not allocate vertices\n");
    fclose(f);
    return (model_t){0};
  }

  model.indices = malloc(sizeof(int) * 3 * f_count);
  if (!model.indices) {
    printf("Could not allocate indices\n");
    free(model.vertices);
    fclose(f);
    return (model_t){0};
  }

  model.vertex_count = v_count;
  model.index_count = f_count * 3;

  rewind(f);

  int vi = 0, ii = 0;
  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "v ", 2) == 0) {
      float x, y, z;
      sscanf(line, "v %f %f %f", &x, &y, &z);
      model.vertices[vi++] = x;
      model.vertices[vi++] = y;
      model.vertices[vi++] = z;
    } else if (strncmp(line, "f ", 2) == 0) {
      int a, b, c;
      sscanf(line, "f %d %d %d", &a, &b, &c);
      model.indices[ii++] = a - 1;
      model.indices[ii++] = b - 1;
      model.indices[ii++] = c - 1;
    }
  }

  fclose(f);
  return model;
}

static void rotate_y(float x, float z, float angle, float *out_x,
                     float *out_z) {
  *out_x = x * cosf(angle) + z * sinf(angle);
  *out_z = -x * sinf(angle) + z * cosf(angle);
}

void draw_model(model_t *model, surface_t *disp, float angle) {
  rdpq_attach(disp, NULL);

  float half_w = disp->width / 2.0f;
  float half_h = disp->height / 2.0f;
  int clip_min_x = 0;
  int clip_max_x = disp->width - 1;
  int clip_min_y = 0;
  int clip_max_y = disp->height - 1;

  color_t gold = RGBA32(255, 215, 0, 255);
  rdpq_set_mode_fill(gold);

  for (int i = 0; i < model->index_count; i += 3) {
    int i0 = model->indices[i] * 3;
    int i1 = model->indices[i + 1] * 3;
    int i2 = model->indices[i + 2] * 3;

    float verts[3][3] = {
        {model->vertices[i0], model->vertices[i0 + 1], model->vertices[i0 + 2]},
        {model->vertices[i1], model->vertices[i1 + 1], model->vertices[i1 + 2]},
        {model->vertices[i2], model->vertices[i2 + 1], model->vertices[i2 + 2]},
    };

    int screen_coords[3][2];
    float projected_z[3];

    for (int j = 0; j < 3; j++) {
      float x = verts[j][0], y = verts[j][1], z = verts[j][2];
      float rx, rz;
      rotate_y(x, z, angle, &rx, &rz);

      projected_z[j] = rz;

      // near-plane clipping
      if (rz < -2.5f)
        goto skip_triangle;

      float scale = 200.0f / (rz + 3.0f);
      int sx = (int)(half_w + rx * scale);
      int sy = (int)(half_h - y * scale);

      screen_coords[j][0] = sx;
      screen_coords[j][1] = sy;
    }

    // backface culling
    int dx1 = screen_coords[1][0] - screen_coords[0][0];
    int dy1 = screen_coords[1][1] - screen_coords[0][1];
    int dx2 = screen_coords[2][0] - screen_coords[0][0];
    int dy2 = screen_coords[2][1] - screen_coords[0][1];
    int cross = dx1 * dy2 - dy1 * dx2;

    if (cross <= 0)
      goto skip_triangle;

    if (screen_coords[0][1] > screen_coords[1][1]) {
      int temp_x = screen_coords[0][0];
      int temp_y = screen_coords[0][1];
      screen_coords[0][0] = screen_coords[1][0];
      screen_coords[0][1] = screen_coords[1][1];
      screen_coords[1][0] = temp_x;
      screen_coords[1][1] = temp_y;
    }
    if (screen_coords[1][1] > screen_coords[2][1]) {
      int temp_x = screen_coords[1][0];
      int temp_y = screen_coords[1][1];
      screen_coords[1][0] = screen_coords[2][0];
      screen_coords[1][1] = screen_coords[2][1];
      screen_coords[2][0] = temp_x;
      screen_coords[2][1] = temp_y;
    }
    if (screen_coords[0][1] > screen_coords[1][1]) {
      int temp_x = screen_coords[0][0];
      int temp_y = screen_coords[0][1];
      screen_coords[0][0] = screen_coords[1][0];
      screen_coords[0][1] = screen_coords[1][1];
      screen_coords[1][0] = temp_x;
      screen_coords[1][1] = temp_y;
    }

    // Screen coordinates use x rightward and y downward; raster bounds are inclusive [0..width-1] and [0..height-1].
    int min_y = screen_coords[0][1];
    int max_y = screen_coords[2][1];
    if (min_y < clip_min_y)
      min_y = clip_min_y;
    if (max_y > clip_max_y)
      max_y = clip_max_y;
    if (min_y > max_y)
      goto skip_triangle;

    for (int y = min_y; y <= max_y; y++) {
      int x_left, x_right;

      if (y < screen_coords[1][1]) {
        if (screen_coords[1][1] - screen_coords[0][1] != 0) {
          float t = (float)(y - screen_coords[0][1]) /
                    (screen_coords[1][1] - screen_coords[0][1]);
          x_left = screen_coords[0][0] +
                   t * (screen_coords[1][0] - screen_coords[0][0]);
        } else {
          x_left = screen_coords[0][0];
        }

        if (screen_coords[2][1] - screen_coords[0][1] != 0) {
          float t = (float)(y - screen_coords[0][1]) /
                    (screen_coords[2][1] - screen_coords[0][1]);
          x_right = screen_coords[0][0] +
                    t * (screen_coords[2][0] - screen_coords[0][0]);
        } else {
          x_right = screen_coords[0][0];
        }
      } else {
        if (screen_coords[2][1] - screen_coords[1][1] != 0) {
          float t = (float)(y - screen_coords[1][1]) /
                    (screen_coords[2][1] - screen_coords[1][1]);
          x_left = screen_coords[1][0] +
                   t * (screen_coords[2][0] - screen_coords[1][0]);
        } else {
          x_left = screen_coords[1][0];
        }

        if (screen_coords[2][1] - screen_coords[0][1] != 0) {
          float t = (float)(y - screen_coords[0][1]) /
                    (screen_coords[2][1] - screen_coords[0][1]);
          x_right = screen_coords[0][0] +
                    t * (screen_coords[2][0] - screen_coords[0][0]);
        } else {
          x_right = screen_coords[0][0];
        }
      }

      if (x_left > x_right) {
        int temp = x_left;
        x_left = x_right;
        x_right = temp;
      }

      if (x_left < clip_min_x)
        x_left = clip_min_x;
      if (x_right > clip_max_x)
        x_right = clip_max_x;
      if (x_left > x_right)
        continue;

      rdpq_fill_rectangle(x_left, y, x_right + 1, y + 1);
    }

  skip_triangle:
    continue;
  }

  rdpq_detach_wait();
}

void free_model(model_t *model) {
  if (model->vertices)
    free(model->vertices);
  if (model->indices)
    free(model->indices);
}
