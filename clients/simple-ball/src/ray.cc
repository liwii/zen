#include <zigen-client-protocol.h>

#include "main.h"

static void
ray_enter(void *data, zgn_ray *ray, uint32_t serial,
    zgn_virtual_object *virtual_object, wl_array *origin, wl_array *direction)
{
  app *app = (struct app *)data;
  app->ray_focus_obj = virtual_object;
  (void)ray;
  (void)serial;
  (void)origin;
  (void)direction;
}

static void
ray_leave(void *data, zgn_ray *ray, uint32_t serial,
    zgn_virtual_object *virtual_object)
{
  app *app = (struct app *)data;
  app->ray_focus_obj = NULL;
  (void)ray;
  (void)serial;
  (void)virtual_object;
}

static void
ray_motion(void *data, zgn_ray *ray, uint32_t time, wl_array *origin,
    wl_array *direction)
{
  (void)data;
  (void)ray;
  (void)time;
  (void)origin;
  (void)direction;
}

static void
ray_button(void *data, zgn_ray *ray, uint32_t serial, uint32_t time,
    uint32_t button, uint32_t state)
{
  app *app = (struct app *)data;
  if (app->ray_focus_obj == NULL) return;
  if (state == ZGN_RAY_BUTTON_STATE_PRESSED) {
    app->stopped = !app->stopped;
  }
  (void)ray;
  (void)serial;
  (void)time;
  (void)button;
  (void)state;
}

static const zgn_ray_listener ray_listener = {
    ray_enter,
    ray_leave,
    ray_motion,
    ray_button,
};

void
add_ray_listener(app *app)
{
  zgn_ray_add_listener(app->ray, &ray_listener, app);
};
