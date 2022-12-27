#pragma once

#include <wlr/render/wlr_renderer.h>
#include <zigzag.h>

struct zn_power_button {
  struct zigzag_node *zigzag_node;

  struct wl_event_source *second_timer_source;
  long next_sec_ms;
};

struct zn_power_button *zn_power_button_create(
    struct zigzag_layout *zigzag_layout, struct wlr_renderer *renderer,
    struct wl_display *display);

void zn_power_button_destroy(struct zn_power_button *self);