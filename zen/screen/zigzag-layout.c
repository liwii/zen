#include "zen/screen/zigzag-layout.h"

#include <wlr/types/wlr_output_damage.h>
#include <zen-common.h>
#include <zigzag.h>

#include "zen/server.h"

void
zen_zigzag_layout_on_damage(struct zigzag_node *node)
{
  struct zen_zigzag_layout_state *state =
      (struct zen_zigzag_layout_state *)node->state;
  wlr_output_damage_add_box(state->output->damage, node->frame);
}

void
vr_button_on_click(struct zigzag_node *self, double x, double y)
{
  UNUSED(self);
  UNUSED(x);
  UNUSED(y);
  zn_warn("VR Mode starting...");
}

void
vr_button_render(struct zigzag_node *self, cairo_t *cr)
{
  cairo_set_source_rgb(cr, 0.067, 0.122, 0.302);
  zigzag_cairo_draw_rounded_rectangle(
      cr, self->frame->width, self->frame->height, self->frame->height / 2);
  cairo_fill(cr);

  cairo_set_font_size(cr, 18);
  cairo_set_source_rgb(cr, 0.953, 0.957, 0.965);
  zigzag_cairo_draw_centered_text(
      cr, "VR", self->frame->width, self->frame->height);
}

void
vr_button_set_frame(
    struct zigzag_node *self, int output_width, int output_height)
{
  double button_width = 160;
  double button_height = 40;

  self->frame->x = (double)output_width / 2 - button_width / 2;
  self->frame->y = (double)output_height - button_height - 10.;
  self->frame->width = button_width;
  self->frame->height = button_height;
}

struct zigzag_node *
create_vr_button(struct zigzag_layout *node_layout, struct zn_server *server)
{
  struct zigzag_node *vr_button =
      zigzag_node_create(node_layout, NULL, server->renderer,
          vr_button_set_frame, vr_button_on_click, vr_button_render);

  if (vr_button == NULL) {
    zn_error("Failed to create the VR button");
    goto err;
  }
  return vr_button;
err:
  return NULL;
}

void
zen_zigzag_layout_setup_default(
    struct zigzag_layout *node_layout, struct zn_server *server)
{
  struct zigzag_node *vr_button = create_vr_button(node_layout, server);
  zn_error("VR button created");
  wl_list_insert(&node_layout->nodes, &vr_button->link);
}
