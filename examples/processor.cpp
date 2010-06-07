#include <iostream>
#include <zbar.h>

using namespace std;
using namespace zbar;

class MyHandler : public Image::Handler
{
    void image_callback (Image &image)
    {
        for(SymbolIterator symbol = image.symbol_begin();
            symbol != image.symbol_end();
            ++symbol)
            cout << "decoded " << symbol->get_type_name() << " symbol "
                 << "\"" << symbol->get_data() << "\"" << endl;
    }
};

int main (int argc, char **argv)
{
    // create and initialize a Processor
    const char *device = "/dev/video0";
    if(argc > 1)
        device = argv[1];
    Processor proc(true, device);

    // configure the Processor
    proc.set_config(ZBAR_NONE, ZBAR_CFG_ENABLE, 1);

    // setup a callback
    MyHandler my_handler;
    proc.set_handler(my_handler);

    // enable the preview window
    proc.set_visible();
    proc.set_active();

    try {
        // keep scanning until user provides key/mouse input
        proc.user_wait();
    }
    catch(ClosedError &e) {
    }
    return(0);
}
