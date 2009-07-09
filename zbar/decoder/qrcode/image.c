/*Copyright (C) 2008-2009  Timothy B. Terriberry (tterribe@xiph.org)
  You can redistribute this library and/or modify it under the terms of the
   GNU Lesser General Public License as published by the Free Software
   Foundation; either version 2.1 of the License, or (at your option) any later
   version.*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include "png.h"

static void png_read(png_structp _png,png_bytep _data,png_size_t _sz){
  size_t ret;
  ret=fread(_data,_sz,1,(FILE *)png_get_io_ptr(_png));
  if(ret!=1)png_error(_png,"Read Error");
}

static void png_write(png_structp _png,png_bytep _data,png_size_t _sz){
  size_t ret;
  ret=fwrite(_data,_sz,1,(FILE *)png_get_io_ptr(_png));
  if(ret!=1)png_error(_png,"Write Error");
}

static void png_flush(png_structp _png){
  fflush((FILE *)png_get_io_ptr(_png));
}

int image_read_png(unsigned char **_img,int *_width,int *_height,FILE *_fp){
  unsigned char *img;
  unsigned char  header[8];
  png_structp    png;
  png_infop      info;
  png_infop      end;
  png_color_16p  bkgd;
  png_bytep     *rows;
  png_uint_32    width;
  png_uint_32    height;
  int            bit_depth;
  int            color_type;
  int            interlace_type;
  int            compression_type;
  int            filter_method;
  int            y;
  if(fread(header,1,8,_fp)<1){
    if(feof(_fp))return -EINVAL;
    return -errno;
  }
  if(png_sig_cmp(header,0,8))return -EINVAL;
  png=png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  if(png==NULL)return -ENOMEM;
  info=png_create_info_struct(png);
  if(info==NULL){
    png_destroy_read_struct(&png,NULL,NULL);
    return -ENOMEM;
  }
  end=png_create_info_struct(png);
  if(end==NULL){
    png_destroy_read_struct(&png,&info,NULL);
    return -ENOMEM;
  }
  img=NULL;
  rows=NULL;
  if(setjmp(png_jmpbuf(png))){
    png_free(png,rows);
    free(img);
    png_destroy_read_struct(&png,&info,&end);
    return -EINVAL;
  }
  png_set_read_fn(png,_fp,png_read);
  png_set_sig_bytes(png,8);
  png_read_info(png,info);
  png_get_IHDR(png,info,&width,&height,&bit_depth,&color_type,
   &interlace_type,&compression_type,&filter_method);
  if(width>INT_MAX||height>INT_MAX||width*(png_size_t)height>INT_MAX){
    png_destroy_read_struct(&png,&info,&end);
    return -EINVAL;
  }
  png_set_expand(png);
  if(bit_depth<8)png_set_packing(png);
  if(bit_depth==16)png_set_strip_16(png);
  if(color_type&PNG_COLOR_MASK_COLOR)png_set_rgb_to_gray(png,1,-1,-1);
  /*Note that color_types 2 and 3 can also have alpha, despite not setting the
     PNG_COLOR_MASK_ALPHA bit.*/
  if(png_get_bKGD(png,info,&bkgd)){
    png_set_background(png,bkgd,PNG_BACKGROUND_GAMMA_FILE,1,1.0);
  }
  png_set_strip_alpha(png);
  img=(unsigned char *)malloc(height*width*sizeof(*img));
  if(img==NULL){
    png_destroy_read_struct(&png,&info,&end);
    return -ENOMEM;
  }
  rows=(png_bytep *)png_malloc(png,height*png_sizeof(png_bytep));
  if(rows==NULL){
    free(img);
    png_destroy_read_struct(&png,&info,&end);
    return -ENOMEM;
  }
  for(y=0;y<(int)height;y++)rows[y]=img+y*width;
  png_read_image(png,rows);
  png_read_end(png,end);
  png_free(png,rows);
  png_destroy_read_struct(&png,&info,&end);
  *_img=img;
  *_width=(int)width;
  *_height=(int)height;
  return 0;
}

int image_write_png(const unsigned char *_img,int _width,int _height,FILE *_fp){
  png_structp  png;
  png_infop    info;
  png_bytep   *rows;
  int          y;
  png=png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  if(png==NULL)return -ENOMEM;
  info=png_create_info_struct(png);
  if(info==NULL){
    png_destroy_write_struct(&png,NULL);
    return -ENOMEM;
  }
  rows=(png_bytep *)png_malloc(png,_height*png_sizeof(png_bytep));
  if(rows==NULL){
    png_destroy_write_struct(&png,&info);
    return -ENOMEM;
  }
  for(y=0;y<_height;y++)rows[y]=(void *)(_img+y*_width);
  if(setjmp(png_jmpbuf(png))){
    png_free(png,rows);
    png_destroy_write_struct(&png,&info);
    return -EINVAL;
  }
  png_set_write_fn(png,_fp,png_write,png_flush);
  png_set_compression_level(png,Z_BEST_COMPRESSION);
  png_set_IHDR(png,info,_width,_height,8,PNG_COLOR_TYPE_GRAY,
   PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);
  png_set_rows(png,info,rows);
  png_write_png(png,info,PNG_TRANSFORM_IDENTITY,NULL);
  png_write_end(png,info);
  png_free(png,rows);
  png_destroy_write_struct(&png,&info);
  return 0;
}
