#include "zen/screen.h"

#include <zen-common.h>

#include "zen/board.h"
#include "zen/screen-layout.h"
#include "zen/server.h"

void
zn_screen_damage(struct zn_screen *self, struct wlr_fbox *box)
{
  struct zn_server *server = zn_server_get_singleton();
  if (server->display_system != ZN_DISPLAY_SYSTEM_SCREEN) return;

  self->implementation->damage(self->user_data, box);
}

void
zn_screen_damage_whole(struct zn_screen *self)
{
  struct zn_server *server = zn_server_get_singleton();
  if (server->display_system != ZN_DISPLAY_SYSTEM_SCREEN) return;

  self->implementation->damage_whole(self->user_data);
}

void
zn_screen_send_frame_done(struct zn_screen *self, struct timespec *when)
{
  if (self->board) zn_board_send_frame_done(self->board, when);
}

void
zn_screen_get_screen_layout_coords(
    struct zn_screen *self, double x, double y, double *dest_x, double *dest_y)
{
  *dest_x = self->x + x;
  *dest_y = self->y + y;
}

void
zn_screen_get_effective_size(
    struct zn_screen *self, double *width, double *height)
{
  self->implementation->get_effective_size(self->user_data, width, height);
}

struct zn_screen *
zn_screen_create(
    const struct zn_screen_interface *implementation, void *user_data)
{
  struct zn_screen *self;

  self = zalloc(sizeof *self);
  if (self == NULL) {
    zn_error("Failed to allocate memory");
    goto err;
  }

  self->user_data = user_data;
  self->implementation = implementation;
  wl_list_init(&self->link);
  self->board = NULL;

  wl_signal_init(&self->events.destroy);

  return self;

err:
  return NULL;
}

void
zn_screen_destroy(struct zn_screen *self)
{
  struct zn_server *server = zn_server_get_singleton();
  zn_screen_layout_remove(server->scene->screen_layout, self);
  wl_signal_emit(&self->events.destroy, NULL);

  wl_list_remove(&self->events.destroy.listener_list);
  free(self);
}