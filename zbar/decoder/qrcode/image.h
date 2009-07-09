/*Copyright (C) 2008-2009  Timothy B. Terriberry (tterribe@xiph.org)
  You can redistribute this library and/or modify it under the terms of the
   GNU Lesser General Public License as published by the Free Software
   Foundation; either version 2.1 of the License, or (at your option) any later
   version.*/
#if !defined(_qrcode_image_H)
# define _qrcode_image_H (1)
#include <stdio.h>

int image_read_png(unsigned char **_img,int *_width,int *_height,FILE *_fp);
int image_write_png(const unsigned char *_img,int _width,int _height,FILE *_fp);

#endif
