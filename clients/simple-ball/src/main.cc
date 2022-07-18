#include <stdio.h>
#include <stdlib.h>
#include <wayland-client.h>

int
main(void)
{
  struct wl_display *display;
  display = wl_display_connect("zigen-0");

  if (display == nullptr) {
    fprintf(stderr, "Cannot connect to the socket: zigen-0\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
