#include "zen/screen/output.h"

#include <math.h>
#include <pixman.h>
#include <wlr/render/wlr_renderer.h>
#include <zen-common.h>

#include "zen/screen/renderer.h"
#include "zen/screen/zigzag-layout.h"

static void zn_output_destroy(struct zn_output *self);

static inline int
scale_length(float length, float offset, float scale)
{
  return round((offset + length) * scale) - round(offset * scale);
}

void
zn_output_box_effective_to_transformed_coords(struct zn_output *self,
    struct wlr_fbox *effective, struct wlr_box *transformed)
{
  float scale = self->wlr_output->scale;
  transformed->x = round(effective->x * scale);
  transformed->y = round(effective->y * scale);
  transformed->width = scale_length(effective->width, effective->x, scale);
  transformed->height = scale_length(effective->height, effective->y, scale);
}

static void
zn_output_handle_screen_damage(void *user_data, struct wlr_fbox *box)
{
  struct wlr_box transformed;
  struct zn_output *self = user_data;

  zn_output_box_effective_to_transformed_coords(self, box, &transformed);
  wlr_output_damage_add_box(self->damage, &transformed);
}

static void
zn_output_handle_screen_damage_whole(void *user_data)
{
  struct zn_output *self = user_data;
  wlr_output_damage_add_whole(self->damage);
}

static void
zn_output_handle_get_effective_size(
    void *user_data, double *width, double *height)
{
  struct zn_output *self = user_data;
  int width_int, height_int;
  wlr_output_effective_resolution(self->wlr_output, &width_int, &height_int);
  *width = width_int;
  *height = height_int;
}

const struct zn_screen_interface screen_implementation = {
    .damage = zn_output_handle_screen_damage,
    .damage_whole = zn_output_handle_screen_damage_whole,
    .get_effective_size = zn_output_handle_get_effective_size,
};

static void
zn_output_handle_wlr_output_destroy(struct wl_listener *listener, void *data)
{
  UNUSED(data);
  struct zn_output *self =
      zn_container_of(listener, self, wlr_output_destroy_listener);

  zn_output_destroy(self);
}

static void
zn_output_handle_damage_frame(struct wl_listener *listener, void *data)
{
  UNUSED(data);
  struct zn_output *self =
      zn_container_of(listener, self, damage_frame_listener);
  struct wlr_renderer *renderer = self->wlr_output->renderer;
  bool needs_frame;
  pixman_region32_t damage;

  pixman_region32_init(&damage);

  if (!wlr_output_damage_attach_render(self->damage, &needs_frame, &damage)) {
    goto damage_finish;
  }

  if (needs_frame) {
    wlr_renderer_begin(
        renderer, self->wlr_output->width, self->wlr_output->height);

    zn_screen_renderer_render(self, renderer, &damage);

    wlr_renderer_end(renderer);
    wlr_output_commit(self->wlr_output);
  } else {
    wlr_output_rollback(self->wlr_output);
  }

damage_finish:
  pixman_region32_fini(&damage);
}

struct zn_output *
zn_output_create(struct wlr_output *wlr_output)
{
  struct zn_output *self;
  struct wlr_output_mode *mode;
  struct zn_server *server = zn_server_get_singleton();

  self = zalloc(sizeof *self);
  if (self == NULL) {
    goto err;
  }

  self->wlr_output = wlr_output;
  wlr_output->data = self;

  wlr_output_create_global(wlr_output);

  self->damage = wlr_output_damage_create(wlr_output);
  if (self->damage == NULL) {
    zn_error("Failed to create a wlr_output_damage");
    goto err_free;
  }

  self->screen = zn_screen_create(&screen_implementation, self);
  if (self->screen == NULL) {
    zn_error("Failed to create z zn_screen");
    goto err_damage;
  }

  self->wlr_output_destroy_listener.notify =
      zn_output_handle_wlr_output_destroy;
  wl_signal_add(
      &wlr_output->events.destroy, &self->wlr_output_destroy_listener);

  self->damage_frame_listener.notify = zn_output_handle_damage_frame;
  wl_signal_add(&self->damage->events.frame, &self->damage_frame_listener);

  if (wl_list_empty(&self->wlr_output->modes)) {
    zn_error("Failed to get output mode");
    goto err_screen;
  }

  mode = wlr_output_preferred_mode(self->wlr_output);
  if (mode == NULL) {
    zn_error("Failed to get preferred output mode");
    goto err_screen;
  }

  wlr_output_set_mode(self->wlr_output, mode);
  wlr_output_enable(self->wlr_output, true);
  if (wlr_output_commit(self->wlr_output) == false) {
    zn_error("Failed to set output mode");
    goto err_screen;
  }

  struct zen_zigzag_layout_state *state;
  state = zalloc(sizeof *state);
  if (state == NULL) {
    zn_error("Failed to allocate memory");
    goto err_screen;
  }

  int output_width, output_height;
  wlr_output_transformed_resolution(wlr_output, &output_width, &output_height);

  struct zigzag_layout *node_layout = zigzag_layout_create(
      output_width, output_height, (void *)state, zen_zigzag_layout_on_damage);

  if (node_layout == NULL) {
    zn_error("Failed to create zigzag_layout");
    goto err_state;
  }

  zen_zigzag_layout_setup_default(node_layout, server);
  self->node_layout = node_layout;

  return self;

err_state:
  free(state);

err_screen:
  zn_screen_destroy(self->screen);

err_damage:
  wlr_output_damage_destroy(self->damage);

err_free:
  free(self);

err:
  return NULL;
}

static void
zn_output_destroy(struct zn_output *self)
{
  free(self->node_layout->state);
  zigzag_node_cleanup_list(&self->node_layout->nodes);
  zigzag_layout_destroy(self->node_layout);
  wl_list_remove(&self->damage_frame_listener.link);
  wl_list_remove(&self->wlr_output_destroy_listener.link);
  zn_screen_destroy(self->screen);
  wlr_output_destroy_global(self->wlr_output);
  free(self);
}
