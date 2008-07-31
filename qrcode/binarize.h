#if !defined(_qrcode_binarize_H)
# define _qrcode_binarize_H (1)

void qr_image_cross_masking_median_filter(unsigned char *_img,
 int _width,int _height);

void qr_wiener_filter(unsigned char *_img,int _width,int _height);

/*Binarizes a grayscale image.*/
void qr_binarize(unsigned char *_img,int _width,int _height);

#endif
