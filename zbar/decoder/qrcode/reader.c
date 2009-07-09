/*Copyright (C) 2008-2009  Timothy B. Terriberry (tterribe@xiph.org)
  You can redistribute this library and/or modify it under the terms of the
   GNU Lesser General Public License as published by the Free Software
   Foundation; either version 2.1 of the License, or (at your option) any later
   version.*/
#include <stdio.h>
#include <stdlib.h>
#include "image.h"
#include "binarize.h"
#include "qrcode.h"

static int qr_code_data_cmp(const void *_a,const void *_b){
  const qr_code_data *a;
  const qr_code_data *b;
  int                 ai;
  int                 bi;
  int                 i;
  a=(const qr_code_data *)_a;
  b=(const qr_code_data *)_b;
  /*Find the top-left corner of each bounding box.*/
  ai=bi=0;
  for(i=1;i<4;i++){
    if(a->bbox[i][1]<a->bbox[ai][1]||
     a->bbox[i][1]==a->bbox[ai][1]&&a->bbox[i][0]<a->bbox[ai][0]){
      ai=i;
    }
    if(b->bbox[i][1]<b->bbox[bi][1]||
     b->bbox[i][1]==b->bbox[bi][1]&&b->bbox[i][0]<b->bbox[bi][0]){
      bi=i;
    }
  }
  /*Sort the codes in top-down, left-right order.*/
  return ((a->bbox[ai][1]>b->bbox[bi][1])-(a->bbox[ai][1]<b->bbox[bi][1])<<1)+
   (a->bbox[ai][0]>b->bbox[bi][0])-(a->bbox[ai][0]<b->bbox[bi][0]);
}

int main(int _argc,char **_argv){
  qr_reader          *reader;
  qr_code_data_list   qrlist;
  char              **text;
  int                 i;
  unsigned char      *img;
  int                 width;
  int                 height;
  if(_argc<2){
    fprintf(stderr,"usage: %s <image>.png\n",_argv[0]);
    return EXIT_FAILURE;
  }
  {
    FILE *fin;
    fin=fopen(_argv[1],"rb");
    if(fin==NULL){
      fprintf(stderr,"'%s' not found.\n",_argv[1]);
      return EXIT_FAILURE;
    }
    image_read_png(&img,&width,&height,fin);
    fclose(fin);
  }
  qr_binarize(img,width,height);
  reader=qr_reader_alloc();
  qr_code_data_list_init(&qrlist);
  if(qr_reader_locate(reader,&qrlist,img,width,height)>0){
    int ntext;
    /*Sort the codes to make test results reproducable.*/
    qsort(qrlist.qrdata,qrlist.nqrdata,sizeof(*qrlist.qrdata),qr_code_data_cmp);
    ntext=qr_code_data_list_extract_text(&qrlist,&text,1);
    qr_code_data_list_clear(&qrlist);
    for(i=0;i<ntext;i++)printf("%s\n",text[i]);
    qr_text_list_free(text,ntext);
  }
  qr_reader_free(reader);
  free(img);
  return EXIT_SUCCESS;
}
