#include "main.h"

#include <string.h>
#include <sys/mman.h>
#include <zigen-shell-client-protocol.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>


static void
cuboid_window_configure(void *data, zgn_cuboid_window *zgn_cuboid_window,
    uint32_t serial, wl_array *half_size_array,
    wl_array *quaternion_array)
{
  (void)data;
  (void)half_size_array;
  (void)quaternion_array;
  zgn_cuboid_window_ack_configure(zgn_cuboid_window, serial);
}

static void
cuboid_window_moved(void *data, zgn_cuboid_window *zgn_cuboid_window,
    wl_array *face_direction_array)
{
  (void)data;
  (void)zgn_cuboid_window;
  (void)face_direction_array;
}

static const zgn_cuboid_window_listener cuboid_window_listener = {
    cuboid_window_configure,
    cuboid_window_moved,
};

void
add_cuboid_window(app *app, float length)
{
  glm::vec3 half_size(length * 1.8);
  app->quaternion = glm::quat();

  wl_array half_size_array, quaternion_array;

  wl_array_init(&half_size_array);
  wl_array_init(&quaternion_array);

  size_t half_size_size = sizeof(glm::vec3);
  float *half_size_data =
      (float *)wl_array_add(&half_size_array, half_size_size);
  memcpy(half_size_data, &half_size, half_size_size);

  size_t quaternion_size = sizeof(glm::quat);
  float *quaternion_data =
      (float *)wl_array_add(&quaternion_array, quaternion_size);
  memcpy(quaternion_data, &app->quaternion, quaternion_size);

  zgn_cuboid_window *cuboid_window = zgn_shell_get_cuboid_window(
      app->shell, app->obj, &half_size_array, &quaternion_array);

  wl_array_release(&half_size_array);

  zgn_cuboid_window_add_listener(
      cuboid_window, &cuboid_window_listener, cuboid_window);
}
