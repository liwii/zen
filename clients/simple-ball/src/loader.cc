#include "loader.h"

#include <GL/glew.h>
#include <stdio.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

Texture *
load_bmp(const char *imagepath)
{
  unsigned char header[54];
  unsigned int dataPos;
  unsigned int width, height;
  unsigned int imageSize;

  unsigned char *data;

  Texture *t = (Texture *)malloc(sizeof(Texture));

  FILE *file = fopen(imagepath, "rb");
  if (!file) {
    fprintf(stderr, "Image could not be opened\n");
    return NULL;
  }

  if (fread(header, 1, 54, file) != 54) {
    fprintf(stderr, "Not a correct BMP file \n");
    return NULL;
  }
  if (header[0] != 'B' || header[1] != 'M') {
    fprintf(stderr, "Not a correct BMP file \n");
    return NULL;
  }

  dataPos = *(int *)&(header[0x0A]);
  imageSize = *(int *)&(header[0x22]);
  width = *(int *)&(header[0x12]);
  height = *(int *)&(header[0x16]);

  if (imageSize == 0) imageSize = width * height * 3;
  if (dataPos == 0) dataPos = 54;

  data = new unsigned char[imageSize];
  if (fread(data, 1, imageSize, file) != imageSize) {
    fprintf(stderr, "Could not read the BMP file\n");
    return NULL;
  }
  fclose(file);

  t->data = data;
  t->imageSize = imageSize;
  t->width = width;
  t->height = height;
  return t;
}
