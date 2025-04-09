// include/model_loader.h
#pragma once
#include <libdragon.h>

typedef struct {
  float *vertices;
  int vertex_count;
  int *indices;
  int index_count;
} model_t;

model_t load_obj_model(const char *path);
void draw_model(model_t *model, surface_t *disp, float angle);
void free_model(model_t *model);
