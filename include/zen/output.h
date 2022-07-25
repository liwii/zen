#ifndef ZEN_OUTPUT_H
#define ZEN_OUTPUT_H

#include <wlr/types/wlr_output.h>

#include "zen/server.h"

/** this destroys itself when the given wlr_output is destroyed */
struct zn_output {
  struct wlr_output *wlr_output;  // nonnull
  struct zn_server *server;       // nonnull

  /** nonnull, automatically destroyed when wlr_output is destroyed */
  struct wlr_output_damage *damage;

  struct zn_screen *screen;  // controlled by zn_output

  // TODO: use this for better repaint loop
  struct wl_event_source *repaint_timer;

  struct wl_listener wlr_output_destroy_listener;
  struct wl_listener damage_frame_listener;
};

void zn_output_update_global(struct zn_output *self);

struct zn_output *zn_output_create(
    struct wlr_output *wlr_output, struct zn_server *server);

#endif  //  ZEN_OUTPUT_H