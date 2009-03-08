#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "util.h"
#include "image.h"
#include "binarize.h"
#include "qrcode.h"

int main(int _argc,char **_argv){
  qr_reader      *reader;
  char          **text;
  int             ntext;
  int             i;
  unsigned char  *img;
  int             width;
  int             height;
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
  ntext=qr_reader_extract_text(reader,img,width,height,&text,1);
  qr_reader_free(reader);
  for(i=0;i<ntext;i++)printf("%s\n",text[i]);
  qr_text_list_free(text,ntext);
  free(img);
  return EXIT_SUCCESS;
}
