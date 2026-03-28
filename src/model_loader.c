// src/model_loader.c

#include "model_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libdragon.h>
#include <math.h>

static int parse_obj_face_vertex_index(const char *token, int vertex_count,
                                       int *out_index) {
  if (!token || !*token)
    return 0;

  char *end = NULL;
  long raw_index = strtol(token, &end, 10);
  if (end == token)
    return 0;

  // Accept "v", "v/vt", "v/vt/vn", or "v//vn" tokens by reading only the
  // leading vertex index before the first '/'.
  if (*end != '\0' && *end != '/')
    return 0;

  int index = 0;
  if (raw_index > 0) {
    index = (int)raw_index - 1;
  } else if (raw_index < 0) {
    index = vertex_count + (int)raw_index;
  } else {
    return 0;
  }

  if (index < 0 || index >= vertex_count)
    return 0;

  *out_index = index;
  return 1;
}

static int parse_obj_face_indices(char *line, int vertex_count, int *out_indices,
                                  int max_indices) {
  int count = 0;
  char *save = NULL;
  char *tok = strtok_r(line, " \t\r\n", &save);

  if (!tok || strcmp(tok, "f") != 0)
    return -1;

  while ((tok = strtok_r(NULL, " \t\r\n", &save)) != NULL) {
    int index = 0;
    if (!parse_obj_face_vertex_index(tok, vertex_count, &index))
      return -1;
    if (count >= max_indices)
      return -1;
    out_indices[count++] = index;
  }

  return count;
}

model_t load_obj_model(const char *path) {
  model_t model = {0};

  FILE *f = fopen(path, "r");
  if (!f) {
    printf("Could not open model: %s\n", path);
    return model;
  }

  int v_count = 0, tri_count = 0;
  char line[256];
  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "v ", 2) == 0)
      v_count++;
    if (strncmp(line, "f ", 2) == 0) {
      char face_line[256];
      int face_indices[64];
      strncpy(face_line, line, sizeof(face_line) - 1);
      face_line[sizeof(face_line) - 1] = '\0';
      int face_count =
          parse_obj_face_indices(face_line, v_count, face_indices, 64);
      if (face_count >= 3)
        tri_count += face_count - 2;
    }
  }

  model.vertices = malloc(sizeof(float) * 3 * v_count);
  if (!model.vertices) {
    printf("Could not allocate vertices\n");
    fclose(f);
    return (model_t){0};
  }

  if (tri_count > 0) {
    model.indices = malloc(sizeof(int) * 3 * tri_count);
    if (!model.indices) {
      printf("Could not allocate indices\n");
      free(model.vertices);
      fclose(f);
      return (model_t){0};
    }
  } else {
    model.indices = NULL;
  }

  model.vertex_count = v_count;
  model.index_count = tri_count * 3;

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
      int face_indices[64];
      char face_line[256];
      strncpy(face_line, line, sizeof(face_line) - 1);
      face_line[sizeof(face_line) - 1] = '\0';

      // Supported OBJ subset:
      // - vertex positions ("v")
      // - faces ("f") with tokens: v, v/vt, v/vt/vn, v//vn
      // - negative indices (relative to current vertex count)
      // Limitations:
      // - ignores vt/vn payloads
      // - ignores faces with malformed/out-of-range indices
      // - face token count capped at 64 for this loader
      int face_count =
          parse_obj_face_indices(face_line, model.vertex_count, face_indices, 64);
      if (face_count < 3) {
        printf("Warning: skipping malformed/degenerate face in %s: %s", path,
               line);
        continue;
      }

      for (int i = 1; i + 1 < face_count; i++) {
        if (ii + 2 >= model.index_count) {
          printf("Warning: skipping face due to index buffer overflow in %s\n",
                 path);
          break;
        }
        model.indices[ii++] = face_indices[0];
        model.indices[ii++] = face_indices[i];
        model.indices[ii++] = face_indices[i + 1];
      }
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

    for (int y = screen_coords[0][1]; y <= screen_coords[2][1]; y++) {
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

      if (x_left < x_right) {
        rdpq_fill_rectangle(x_left, y, x_right + 1, y + 1);
      }
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
