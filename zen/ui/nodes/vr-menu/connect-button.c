#include "zen/ui/nodes/vr-menu/connect-button.h"

#include <cairo.h>
#include <zen-common.h>
#include <zigzag.h>

#include "zen/peer.h"
#include "zen/server.h"
#include "zen/ui/layout-constants.h"
#include "zen/ui/nodes/vr-menu/vr-menu.h"

static void
zn_vr_menu_headset_connect_button_on_click(
    struct zigzag_node *node, double x, double y)
{
  UNUSED(x);
  UNUSED(y);
  struct zn_vr_menu_headset_connect_button *self = node->user_data;
  struct zn_peer *peer = self->peer;
  struct zn_server *server = zn_server_get_singleton();
  struct zn_remote *remote = server->remote;
  struct znr_session *session =
      znr_remote_create_session(remote->znr_remote, peer->znr_remote_peer);
  if (session == NULL) return;

  zn_peer_set_session(peer, session);
  zna_system_set_current_session(server->appearance_system, session);
}

static bool
zn_vr_menu_headset_connect_button_render(struct zigzag_node *node, cairo_t *cr)
{
  cairo_set_source_rgb(cr, 0.07, 0.12, 0.30);
  zigzag_cairo_draw_rounded_rectangle(cr, 0., 0., node->frame.width,
      node->frame.height, node->frame.height / 2);
  cairo_fill(cr);

  cairo_set_font_size(cr, 13);
  cairo_set_source_rgb(cr, 1., 1., 1.);
  zigzag_cairo_draw_text(cr, "Connect", node->frame.width / 2,
      node->frame.height / 2, ZIGZAG_ANCHOR_CENTER, ZIGZAG_ANCHOR_CENTER);
  return true;
}

static void
zn_vr_menu_headset_connect_button_set_frame(
    struct zigzag_node *node, double screen_width, double screen_height)
{
  struct zn_vr_menu_headset_connect_button *self = node->user_data;
  node->frame.x = screen_width - vr_menu_space_right -
                  (vr_menu_bubble_width - vr_menu_headset_width) / 2 -
                  vr_menu_headset_connect_button_margin -
                  vr_menu_headset_connect_button_width;

  struct zn_server *server = zn_server_get_singleton();
  struct zn_remote *remote = server->remote;

  node->frame.y =
      screen_height - menu_bar_height - tip_height - vr_how_to_connect_height -
      (wl_list_length(&remote->peer_list) - self->idx) *
          vr_menu_headset_height +
      (vr_menu_headset_height - vr_menu_headset_connect_button_height) / 2;

  node->frame.width = vr_menu_headset_connect_button_width;
  node->frame.height = vr_menu_headset_connect_button_height;
}

static const struct zigzag_node_impl implementation = {
    .on_click = zn_vr_menu_headset_connect_button_on_click,
    .set_frame = zn_vr_menu_headset_connect_button_set_frame,
    .render = zn_vr_menu_headset_connect_button_render,
};

struct zn_vr_menu_headset_connect_button *
zn_vr_menu_headset_connect_button_create(struct zigzag_layout *zigzag_layout,
    struct wlr_renderer *renderer, struct zn_peer *peer, int idx)
{
  struct zn_vr_menu_headset_connect_button *self;

  self = zalloc(sizeof *self);
  if (self == NULL) {
    zn_error("Failed to allocate memory");
    goto err;
  }
  self->peer = peer;
  self->idx = idx;

  struct zigzag_node *zigzag_node =
      zigzag_node_create(&implementation, zigzag_layout, renderer, true, self);

  if (zigzag_node == NULL) {
    zn_error("Failed to create a zigzag_node");
    goto err_vr_menu_headset_connect_button;
  }
  self->zigzag_node = zigzag_node;

  return self;

err_vr_menu_headset_connect_button:
  free(self);

err:
  return NULL;
}

void
zn_vr_menu_headset_connect_button_destroy(
    struct zn_vr_menu_headset_connect_button *self)
{
  zigzag_node_destroy(self->zigzag_node);
  free(self);
}
