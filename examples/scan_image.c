#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <zbar.h>

#if !defined(PNG_LIBPNG_VER) || \
    PNG_LIBPNG_VER < 10018 ||   \
    (PNG_LIBPNG_VER > 10200 &&  \
     PNG_LIBPNG_VER < 10209)
  /* Changes to Libpng from version 1.2.42 to 1.4.0 (January 4, 2010)
   * ...
   * 2. m. The function png_set_gray_1_2_4_to_8() was removed. It has been
   *       deprecated since libpng-1.0.18 and 1.2.9, when it was replaced with
   *       png_set_expand_gray_1_2_4_to_8() because the former function also
   *       expanded palette images.
   */
# define png_set_expand_gray_1_2_4_to_8 png_set_gray_1_2_4_to_8
#endif

zbar_image_scanner_t *scanner = NULL;

/* to complete a runnable example, this abbreviated implementation of
 * get_data() will use libpng to read an image file. refer to libpng
 * documentation for details
 */
static void get_data (const char *name,
                      int *width, int *height,
                      void **raw)
{
    FILE *file = fopen(name, "rb");
    if(!file) exit(2);
    png_structp png =
        png_create_read_struct(PNG_LIBPNG_VER_STRING,
                               NULL, NULL, NULL);
    if(!png) exit(3);
    if(setjmp(png_jmpbuf(png))) exit(4);
    png_infop info = png_create_info_struct(png);
    if(!info) exit(5);
    png_init_io(png, file);
    png_read_info(png, info);
    /* configure for 8bpp grayscale input */
    int color = png_get_color_type(png, info);
    int bits = png_get_bit_depth(png, info);
    if(color & PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);
    if(color == PNG_COLOR_TYPE_GRAY && bits < 8)
        png_set_expand_gray_1_2_4_to_8(png);
    if(bits == 16)
        png_set_strip_16(png);
    if(color & PNG_COLOR_MASK_ALPHA)
        png_set_strip_alpha(png);
    if(color & PNG_COLOR_MASK_COLOR)
        png_set_rgb_to_gray_fixed(png, 1, -1, -1);
    /* allocate image */
    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    *raw = malloc(*width * *height);
    png_bytep rows[*height];
    int i;
    for(i = 0; i < *height; i++)
        rows[i] = *raw + (*width * i);
    png_read_image(png, rows);
}

int main (int argc, char **argv)
{
    if(argc < 2) return(1);

    /* create a reader */
    scanner = zbar_image_scanner_create();

    /* configure the reader */
    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);

    /* obtain image data */
    int width = 0, height = 0;
    void *raw = NULL;
    get_data(argv[1], &width, &height, &raw);

    /* wrap image data */
    zbar_image_t *image = zbar_image_create();
    zbar_image_set_format(image, zbar_fourcc('Y','8','0','0'));
    zbar_image_set_size(image, width, height);
    zbar_image_set_data(image, raw, width * height, zbar_image_free_data);

    /* scan the image for barcodes */
    int n = zbar_scan_image(scanner, image);

    /* extract results */
    const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
    for(; symbol; symbol = zbar_symbol_next(symbol)) {
        /* do something useful with results */
        zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
        const char *data = zbar_symbol_get_data(symbol);
        printf("decoded %s symbol \"%s\"\n",
               zbar_get_symbol_name(typ), data);
    }

    /* clean up */
    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner);

    return(0);
}
