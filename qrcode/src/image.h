#if !defined(_qrcode_image_H)
# define _qrcode_image_H (1)
#include <stdio.h>

int image_read_png(unsigned char **_img,int *_width,int *_height,FILE *_fp);
int image_write_png(const unsigned char *_img,int _width,int _height,FILE *_fp);

#endif
