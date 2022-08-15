#include "zen/cursor.h"

#include <drm/drm_fourcc.h>
#include <linux/input.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/xcursor.h>

#include "zen-common.h"
#include "zen/scene/screen-layout.h"
#include "zen/server.h"

// screen_x and screen_y must be less than screen->width/height
static void
zn_cursor_update_position(struct zn_cursor* self, struct zn_screen* screen,
    int screen_x, int screen_y)
{
  self->x = screen_x;
  self->y = screen_y;

  if (self->screen == screen) {
    return;
  }

  if (self->screen != NULL) {
    wl_list_remove(&self->destroy_screen_listener.link);
    wl_list_init(&self->destroy_screen_listener.link);
  }
  if (screen != NULL) {
    wl_signal_add(&screen->events.destroy, &self->destroy_screen_listener);
  }

  self->screen = screen;
}

static void
zn_cursor_handle_new_screen(struct wl_listener* listener, void* data)
{
  struct zn_cursor* self = zn_container_of(listener, self, new_screen_listener);
  struct zn_screen* screen = data;
  struct wlr_box box;

  if (self->screen == NULL) {
    zn_screen_get_box(screen, &box);
    zn_cursor_update_position(self, screen, box.width / 2, box.height / 2);
  }
}

static void
zn_cursor_handle_destroy_screen(struct wl_listener* listener, void* data)
{
  UNUSED(data);
  struct zn_cursor* self =
      zn_container_of(listener, self, destroy_screen_listener);
  struct zn_server* server = zn_server_get_singleton();
  struct zn_screen_layout* screen_layout = server->scene->screen_layout;
  struct zn_screen* screen;
  struct wlr_box box = {0};
  bool found = false;

  wl_list_for_each(screen, &screen_layout->screens, link)
  {
    if (screen != self->screen) {
      found = true;
      break;
    }
  }

  if (found) {
    zn_screen_get_box(self->screen, &box);
  }
  zn_cursor_update_position(
      self, found ? screen : NULL, box.width / 2, box.height / 2);
}

void
zn_cursor_move_relative(struct zn_cursor* self, int dx, int dy)
{
  struct zn_server* server = zn_server_get_singleton();
  struct zn_screen_layout* layout = server->scene->screen_layout;
  struct zn_screen* new_screen;
  int layout_x, layout_y;
  int screen_x = self->x + dx;
  int screen_y = self->y + dy;

  if (self->screen == NULL) {
    return;
  }

  zn_screen_get_screen_layout_coords(
      self->screen, screen_x, screen_y, &layout_x, &layout_y);

  new_screen = zn_screen_layout_get_closest_screen(
      layout, layout_x, layout_y, &screen_x, &screen_y);
  zn_cursor_update_position(self, new_screen, screen_x, screen_y);
}

struct zn_cursor*
zn_cursor_create(void)
{
  struct zn_server* server = zn_server_get_singleton();
  struct zn_screen_layout* screen_layout = server->scene->screen_layout;
  struct wlr_xcursor* xcursor;
  struct wlr_xcursor_image* image;
  struct zn_cursor* self;

  self = zalloc(sizeof *self);
  if (self == NULL) {
    zn_error("Failed to allocate memory");
    goto err;
  }

  self->xcursor_manager = wlr_xcursor_manager_create(NULL, 24);
  if (self->xcursor_manager == NULL) {
    zn_error("Failed to create wlr_xcursor_manager");
    goto err_free;
  }
  wlr_xcursor_manager_load(self->xcursor_manager, 1.f);

  xcursor =
      wlr_xcursor_manager_get_xcursor(self->xcursor_manager, "left_ptr", 1.f);
  if (xcursor == NULL) {
    zn_error("Failed to get xcursor");
    goto err_wlr_xcursor_manager;
  }
  image = xcursor->images[0];

  self->texture = wlr_texture_from_pixels(server->renderer, DRM_FORMAT_ARGB8888,
      image->width * 4, image->width, image->height, image->buffer);

  self->new_screen_listener.notify = zn_cursor_handle_new_screen;
  wl_signal_add(&screen_layout->events.new_screen, &self->new_screen_listener);
  self->destroy_screen_listener.notify = zn_cursor_handle_destroy_screen;

  wl_list_init(&self->destroy_screen_listener.link);

  self->screen = NULL;

  return self;

err_wlr_xcursor_manager:
  wlr_xcursor_manager_destroy(self->xcursor_manager);

err_free:
  free(self);

err:
  return NULL;
}

void
zn_cursor_destroy(struct zn_cursor* self)
{
  wl_list_remove(&self->new_screen_listener.link);
  wl_list_remove(&self->destroy_screen_listener.link);
  wlr_texture_destroy(self->texture);
  wlr_xcursor_manager_destroy(self->xcursor_manager);
  free(self);
}