#include <stdio.h>
#include <zbar.h>

static void my_handler (zbar_image_t *image,
                        const void *userdata)
{
    /* extract results */
    const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
    for(; symbol; symbol = zbar_symbol_next(symbol)) {
        /* do something useful with results */
        zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
        const char *data = zbar_symbol_get_data(symbol);
        printf("decoded %s symbol \"%s\"\n",
               zbar_get_symbol_name(typ), data);
    }
}

int main (int argc, char **argv)
{
    const char *device = "/dev/video0";

    /* create a Processor */
    zbar_processor_t *proc = zbar_processor_create(1);

    /* configure the Processor */
    zbar_processor_set_config(proc, 0, ZBAR_CFG_ENABLE, 1);

    /* initialize the Processor */
    if(argc > 1)
        device = argv[1];
    zbar_processor_init(proc, device, 1);

    /* setup a callback */
    zbar_processor_set_data_handler(proc, my_handler, NULL);

    /* enable the preview window */
    zbar_processor_set_visible(proc, 1);
    zbar_processor_set_active(proc, 1);

    /* keep scanning until user provides key/mouse input */
    zbar_processor_user_wait(proc, -1);

    /* clean up */
    zbar_processor_destroy(proc);

    return(0);
}
