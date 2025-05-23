// src/main.c

#include "model_loader.h"

int main(void) {
  display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE,
               ANTIALIAS_RESAMPLE);

  dfs_init(DFS_DEFAULT_LOCATION);
  controller_init();
  rdpq_init();
  rspq_init();

  model_t egg_model = load_obj_model("rom:/egg.obj");
  float angle = 0.0f;

  while (1) {
    controller_scan();
    struct controller_data keys = get_keys_held();

    if (keys.c[0].x > 10)
      angle += 0.05f;
    if (keys.c[0].x < -10)
      angle -= 0.05f;

    surface_t *disp = display_get();
    graphics_fill_screen(disp, graphics_make_color(0, 0, 32, 255));
    draw_model(&egg_model, disp, angle);
    display_show(disp);
  }

  return 0;
}
