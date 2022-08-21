#include <GL/glew.h>

#ifndef LOADER_H
#define LOADER_H

struct Texture {
  unsigned char *data;
  unsigned int imageSize;
  unsigned int width;
  unsigned int height;
};

Texture *load_bmp(const char *imagepath);
#endif  //  LOADER_H