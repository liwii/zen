#include <ft2build.h>
#include <zigzag.h>
#include FT_FREETYPE_H

#include <cairo-ft.h>
#include <cairo.h>
#include <drm_fourcc.h>
#include <librsvg/rsvg.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_output_damage.h>

#include "build-config.h"
#include "zen-common.h"
#include "zen/output.h"
#include "zen/scene/zigzag-layout.h"

void
zen_zigzag_layout_on_damage(struct zigzag_node *node)
{
  struct zen_zigzag_layout_state *state =
      (struct zen_zigzag_layout_state *)node->state;
  wlr_output_damage_add_box(state->output->damage, node->frame);
}

void
zigzag_node_update_texture(struct zigzag_node *self)
{
  self->layout->on_damage(self);
  self->set_frame(
      self, self->layout->output_width, self->layout->output_height);
  struct wlr_texture *old_texture = self->texture;
  struct zn_server *server = zn_server_get_singleton();
  self->texture = zigzag_node_render_texture(self, server->renderer);
  self->layout->on_damage(self);
  wlr_texture_destroy(old_texture);
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
  // TODO: With cairo_in_fill(), non-rectangle clickable area can be created
  // TODO: use color code
  cairo_set_source_rgb(cr, 0.067, 0.122, 0.302);
  zigzag_cairo_draw_rounded_rectangle(
      cr, self->frame->width, self->frame->height, self->frame->height / 2);
  cairo_fill(cr);

  FT_Library library;
  FT_Init_FreeType(&library);
  FT_Face face;
  FT_New_Face(
      library, "/usr/share/fonts/truetype/ubuntu/Ubuntu-M.ttf", 0, &face);

  cairo_font_face_t *cr_face = cairo_ft_font_face_create_for_ft_face(face, 0);
  cairo_set_font_face(cr, cr_face);

  // TODO: verify the font
  // cairo_select_font_face(
  //     cr, "Times", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  // zn_error("Font type: %d",
  // cairo_font_face_get_type(cairo_get_font_face(cr)));

  cairo_set_font_size(cr, 18);
  cairo_set_source_rgb(cr, 0.953, 0.957, 0.965);
  zigzag_cairo_draw_centered_text(
      cr, "Zigen", self->frame->width, self->frame->height);
  FT_Done_Face(face);
  FT_Done_FreeType(library);
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

struct power_menu_quit_state {
  bool shown;
};

void
power_menu_quit_on_click(struct zigzag_node *self, double x, double y)
{
  UNUSED(x);
  UNUSED(y);
  UNUSED(self);
  struct zn_server *server = zn_server_get_singleton();
  zn_server_terminate(server, EXIT_SUCCESS);
}

void
power_menu_quit_render(struct zigzag_node *self, cairo_t *cr)
{
  struct power_menu_quit_state *state =
      (struct power_menu_quit_state *)self->state;
  if (!state->shown) return;
  cairo_set_source_rgb(cr, 0.812, 0.824, 0.859);
  cairo_paint(cr);

  cairo_select_font_face(
      cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, 18);
  cairo_set_source_rgb(cr, 0.102, 0.102, 0.102);
  zigzag_cairo_draw_left_aligned_text(
      cr, "Quit", self->frame->width, self->frame->height, 10);

  cairo_set_source_rgb(cr, 0.502, 0.502, 0.502);
  zigzag_cairo_draw_right_aligned_text(
      cr, "alt + q", self->frame->width, self->frame->height, 10);
}

void
power_menu_quit_set_frame(
    struct zigzag_node *self, int output_width, int output_height)
{
  struct power_menu_quit_state *state =
      (struct power_menu_quit_state *)self->state;
  if (state->shown) {
    double menu_width = 180;
    double menu_height = 30;

    self->frame->x = (double)output_width - menu_width;
    self->frame->y = (double)output_height - menu_height - 60;
    self->frame->width = menu_width;
    self->frame->height = menu_height;

  } else {
    self->frame->x = 0;
    self->frame->y = 0;
    self->frame->width = 1;
    self->frame->height = 1;
  }
}

struct zigzag_node *
create_power_menu_quit(
    struct zigzag_layout *node_layout, struct zn_server *server)
{
  struct power_menu_quit_state *state;
  state = zalloc(sizeof *state);
  state->shown = false;
  struct zigzag_node *power_menu_quit = zigzag_node_create(node_layout,
      (void *)state, server->renderer, power_menu_quit_set_frame,
      power_menu_quit_on_click, power_menu_quit_render);

  if (power_menu_quit == NULL) {
    zn_error("Failed to create the power button");
    goto err;
  }
  return power_menu_quit;
err:
  return NULL;
}

struct power_button_state {
  bool clicked;
};

void
power_button_on_click(struct zigzag_node *self, double x, double y)
{
  UNUSED(x);
  UNUSED(y);
  struct power_button_state *state = (struct power_button_state *)self->state;
  state->clicked = !state->clicked;
  zigzag_node_update_texture(self);
  struct zigzag_node *child;
  wl_list_for_each (child, &self->children, link) {
    struct power_menu_quit_state *child_state =
        (struct power_menu_quit_state *)child->state;
    child_state->shown = state->clicked;
    zigzag_node_update_texture(child);
  }
}

void
power_button_render(struct zigzag_node *self, cairo_t *cr)
{
  GError *error = NULL;
  struct power_button_state *state = (struct power_button_state *)self->state;
  GFile *file = g_file_new_for_path(
      state->clicked ? POWER_BUTTON_ICON_CLICKED : POWER_BUTTON_ICON);
  RsvgHandle *handle = rsvg_handle_new_from_gfile_sync(
      file, RSVG_HANDLE_FLAGS_NONE, NULL, &error);
  if (handle == NULL) {
    zn_error("Failed to create the svg handler: %s", error->message);
    goto err;
  }
  RsvgRectangle viewport = {
      .x = 0.0,
      .y = 0.0,
      .width = self->frame->width,
      .height = self->frame->height,
  };
  if (!rsvg_handle_render_document(handle, cr, &viewport, &error)) {
    zn_error("Failed to render the svg");
  }

  g_object_unref(handle);
err:
  return;
}

void
power_button_set_frame(
    struct zigzag_node *self, int output_width, int output_height)
{
  double button_width = 40;
  double button_height = 40;
  self->frame->x = (double)output_width - button_width - 10;
  self->frame->y = (double)output_height - button_height - 10;
  self->frame->width = button_width;
  self->frame->height = button_height;
}

struct zigzag_node *
create_power_button(struct zigzag_layout *node_layout, struct zn_server *server,
    struct zigzag_node *power_menu_quit)
{
  struct power_button_state *state;
  state = zalloc(sizeof *state);
  state->clicked = false;

  struct zigzag_node *power_button =
      zigzag_node_create(node_layout, (void *)state, server->renderer,
          power_button_set_frame, power_button_on_click, power_button_render);

  if (power_button == NULL) {
    zn_error("Failed to create the power button");
    goto err;
  }
  wl_list_insert(&power_button->children, &power_menu_quit->link);
  return power_button;
err:
  return NULL;
}

void
menu_bar_on_click(struct zigzag_node *self, double x, double y)
{
  UNUSED(self);
  UNUSED(x);
  UNUSED(y);
}

void
menu_bar_render(struct zigzag_node *self, cairo_t *cr)
{
  UNUSED(self);
  cairo_set_source_rgb(cr, 0.529, 0.557, 0.647);
  cairo_paint(cr);
}

void
menu_bar_set_frame(
    struct zigzag_node *self, int output_width, int output_height)
{
  double bar_width = (double)output_width;
  double bar_height = 60;

  self->frame->x = 0.;
  self->frame->y = (double)output_height - bar_height;
  self->frame->width = bar_width;
  self->frame->height = bar_height;
}

struct zigzag_node *
create_menu_bar(struct zigzag_layout *node_layout, struct zn_server *server,
    struct zigzag_node *vr_button, struct zigzag_node *power_button)
{
  struct zigzag_node *menu_bar = zigzag_node_create(node_layout, NULL,
      server->renderer, menu_bar_set_frame, menu_bar_on_click, menu_bar_render);

  if (menu_bar == NULL) {
    zn_error("Failed to create the menu bar");
    goto err;
  }
  wl_list_insert(&menu_bar->children, &vr_button->link);
  wl_list_insert(&menu_bar->children, &power_button->link);
  return menu_bar;
err:
  return NULL;
}

void
zigzag_layout_setup_default(
    struct zigzag_layout *node_layout, struct zn_server *server)
{
  struct zigzag_node *vr_button = create_vr_button(node_layout, server);
  struct zigzag_node *power_menu_quit =
      create_power_menu_quit(node_layout, server);
  struct zigzag_node *power_button =
      create_power_button(node_layout, server, power_menu_quit);
  struct zigzag_node *menu_bar =
      create_menu_bar(node_layout, server, vr_button, power_button);
  // Register the widgets on the screen
  wl_list_insert(&node_layout->nodes, &menu_bar->link);
}