//------------------------------------------------------------------------
//  Copyright 2008-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
//
//  This file is part of the ZBar Bar Code Reader.
//
//  The ZBar Bar Code Reader is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser Public License as
//  published by the Free Software Foundation; either version 2.1 of
//  the License, or (at your option) any later version.
//
//  The ZBar Bar Code Reader is distributed in the hope that it will be
//  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
//  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser Public License for more details.
//
//  You should have received a copy of the GNU Lesser Public License
//  along with the ZBar Bar Code Reader; if not, write to the Free
//  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//  Boston, MA  02110-1301  USA
//
//  http://sourceforge.net/projects/zbar
//------------------------------------------------------------------------
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include <zbar.h>

typedef zbar_symbol_t *Barcode__ZBar__Symbol;
typedef zbar_image_t *Barcode__ZBar__Image;
typedef zbar_processor_t *Barcode__ZBar__Processor;
typedef zbar_video_t *Barcode__ZBar__Video;
typedef zbar_window_t *Barcode__ZBar__Window;
typedef zbar_image_scanner_t *Barcode__ZBar__ImageScanner;
typedef zbar_decoder_t *Barcode__ZBar__Decoder;
typedef zbar_scanner_t *Barcode__ZBar__Scanner;
typedef void *Barcode__ZBar__Error;

typedef unsigned long fourcc_t;
typedef int timeout_t;
typedef int config_error;

typedef struct handler_wrapper_s {
    SV *instance;
    SV *handler;
    SV *closure;
} handler_wrapper_t;


static AV *LOOKUP_zbar_color_t = NULL;
static AV *LOOKUP_zbar_symbol_type_t = NULL;
static AV *LOOKUP_zbar_error_t = NULL;
static AV *LOOKUP_zbar_config_t = NULL;

#define CONSTANT(typ, prefix, sym, name)                \
    do {                                                \
        SV *c = newSViv(ZBAR_ ## prefix ## sym);       \
        sv_setpv(c, name);                              \
        SvIOK_on(c);                                    \
        newCONSTSUB(stash, #sym, c);                    \
        av_store(LOOKUP_zbar_ ## typ ## _t,            \
                 ZBAR_ ## prefix ## sym,               \
                 SvREFCNT_inc(c));                      \
    } while(0)

#define LOOKUP_ENUM(typ, val) \
    lookup_enum(LOOKUP_zbar_ ## typ ## _t, val)

static inline SV *lookup_enum (AV *lookup, int val)
{
    SV **tmp = av_fetch(lookup, val, 0);
    return((tmp) ? *tmp : sv_newmortal());
}

static inline void check_error (int rc, void *obj)
{
    if(rc < 0) {
        sv_setref_pv(get_sv("@", TRUE), "Barcode::ZBar::Error", obj);
        croak(NULL);
    }
}

static void image_cleanup_handler (zbar_image_t *image)
{
    SV *data = zbar_image_get_userdata(image);
    if(!data)
        /* FIXME this is internal error */
        return;

    /* release reference to cleanup data */
    SvREFCNT_dec(data);
}

static inline int set_handler (handler_wrapper_t **wrapp,
                               SV *instance,
                               SV *handler,
                               SV *closure)
{
    handler_wrapper_t *wrap = *wrapp;
    if(!handler || !SvOK(handler)) {
        if(wrap) {
            if(wrap->instance) SvREFCNT_dec(wrap->instance);
            if(wrap->handler) SvREFCNT_dec(wrap->handler);
            if(wrap->closure) SvREFCNT_dec(wrap->closure);
            wrap->instance = wrap->handler = wrap->closure = NULL;
        }
        return(0);
    }

    if(!wrap) {
        Newxz(wrap, 1, handler_wrapper_t);
        wrap->instance = newSVsv(instance);
        wrap->closure = newSV(0);
        *wrapp = wrap;
    }

    if(wrap->handler)
        SvSetSV(wrap->handler, handler);
    else
        wrap->handler = newSVsv(handler);

    if(!closure || !SvOK(closure))
        SvSetSV(wrap->closure, &PL_sv_undef);
    else
        SvSetSV(wrap->closure, closure);
    return(1);
}

static inline void activate_handler (handler_wrapper_t *wrap,
                                     SV *param)
{
    dSP;
    if(!wrap)
        /* FIXME this is internal error */
        return;

    ENTER;
    SAVETMPS;

    PUSHMARK(SP);
    EXTEND(SP, 2);
    PUSHs(sv_mortalcopy(wrap->instance));
    if(param)
        PUSHs(param);
    PUSHs(sv_mortalcopy(wrap->closure));
    PUTBACK;

    call_sv(wrap->handler, G_DISCARD);

    FREETMPS;
    LEAVE;
}

static void processor_handler (zbar_image_t *image,
                               const void *userdata)
{
    SV *img;
    zbar_image_ref(image, 1);
    img = sv_setref_pv(newSV(0), "Barcode::ZBar::Image", image);
    activate_handler((void*)userdata, img);
    SvREFCNT_dec(img);
}

static void decoder_handler (zbar_decoder_t *decoder)
{
    activate_handler(zbar_decoder_get_userdata(decoder), NULL);
}


MODULE = Barcode::ZBar	PACKAGE = Barcode::ZBar		PREFIX = zbar_

PROTOTYPES: ENABLE

BOOT:
    {
        HV *stash = gv_stashpv("Barcode::ZBar", TRUE);

        LOOKUP_zbar_color_t = newAV();
        CONSTANT(color, , SPACE, "SPACE");
        CONSTANT(color, , BAR, "BAR");
    }

SV *
zbar_version()
    PREINIT:
	unsigned major;
        unsigned minor;
    CODE:
        zbar_version(&major, &minor);
        RETVAL = newSVpvf("%u.%u", major, minor);
    OUTPUT:
        RETVAL

void
zbar_increase_verbosity()

void
zbar_set_verbosity(verbosity)
	int	verbosity

SV *
parse_config(config_string)
        const char *	config_string
    PREINIT:
        zbar_symbol_type_t sym;
        zbar_config_t cfg;
        int val;
    PPCODE:
        if(zbar_parse_config(config_string, &sym, &cfg, &val))
            croak("invalid configuration setting: %s", config_string);
        EXTEND(SP, 3);
        PUSHs(LOOKUP_ENUM(symbol_type, sym));
        PUSHs(LOOKUP_ENUM(config, cfg));
        mPUSHi(val);


MODULE = Barcode::ZBar	PACKAGE = Barcode::ZBar::Error	PREFIX = zbar_

BOOT:
    {
        HV *stash = gv_stashpv("Barcode::ZBar::Error", TRUE);

        LOOKUP_zbar_error_t = newAV();
        CONSTANT(error, ERR_, NOMEM, "out of memory");
        CONSTANT(error, ERR_, INTERNAL, "internal library error");
        CONSTANT(error, ERR_, UNSUPPORTED, "unsupported request");
        CONSTANT(error, ERR_, INVALID, "invalid request");
        CONSTANT(error, ERR_, SYSTEM, "system error");
        CONSTANT(error, ERR_, LOCKING, "locking error");
        CONSTANT(error, ERR_, BUSY, "all resources busy");
        CONSTANT(error, ERR_, XDISPLAY, "X11 display error");
        CONSTANT(error, ERR_, XPROTO, "X11 protocol error");
        CONSTANT(error, ERR_, CLOSED, "output window is closed");
    }

zbar_error_t
get_error_code(err)
	Barcode::ZBar::Error	err
    CODE:
	RETVAL = _zbar_get_error_code(err);
    OUTPUT:
	RETVAL

const char *
error_string(err)
	Barcode::ZBar::Error	err
    CODE:
	RETVAL = _zbar_error_string(err, 1);
    OUTPUT:
	RETVAL


MODULE = Barcode::ZBar	PACKAGE = Barcode::ZBar::Config	PREFIX = zbar_config_

BOOT:
    {
        HV *stash = gv_stashpv("Barcode::ZBar::Config", TRUE);

        LOOKUP_zbar_config_t = newAV();
        CONSTANT(config, CFG_, ENABLE, "enable");
        CONSTANT(config, CFG_, ADD_CHECK, "add-check");
        CONSTANT(config, CFG_, EMIT_CHECK, "emit-check");
        CONSTANT(config, CFG_, ASCII, "ascii");
        CONSTANT(config, CFG_, MIN_LEN, "min-length");
        CONSTANT(config, CFG_, MAX_LEN, "max-length");
        CONSTANT(config, CFG_, X_DENSITY, "x-density");
        CONSTANT(config, CFG_, Y_DENSITY, "y-density");
    }


MODULE = Barcode::ZBar	PACKAGE = Barcode::ZBar::Symbol	PREFIX = zbar_symbol_

BOOT:
    {
        HV *stash = gv_stashpv("Barcode::ZBar::Symbol", TRUE);

        LOOKUP_zbar_symbol_type_t = newAV();
        CONSTANT(symbol_type, , NONE, "None");
        CONSTANT(symbol_type, , PARTIAL, "Partial");
        CONSTANT(symbol_type, , EAN8, zbar_get_symbol_name(ZBAR_EAN8));
        CONSTANT(symbol_type, , UPCE, zbar_get_symbol_name(ZBAR_UPCE));
        CONSTANT(symbol_type, , ISBN10, zbar_get_symbol_name(ZBAR_ISBN10));
        CONSTANT(symbol_type, , UPCA, zbar_get_symbol_name(ZBAR_UPCA));
        CONSTANT(symbol_type, , EAN13, zbar_get_symbol_name(ZBAR_EAN13));
        CONSTANT(symbol_type, , ISBN13, zbar_get_symbol_name(ZBAR_ISBN13));
        CONSTANT(symbol_type, , I25, zbar_get_symbol_name(ZBAR_I25));
        CONSTANT(symbol_type, , CODE39, zbar_get_symbol_name(ZBAR_CODE39));
        CONSTANT(symbol_type, , PDF417, zbar_get_symbol_name(ZBAR_PDF417));
        CONSTANT(symbol_type, , CODE128, zbar_get_symbol_name(ZBAR_CODE128));
    }

zbar_symbol_type_t
zbar_symbol_get_type(symbol)
	Barcode::ZBar::Symbol symbol

const char *
zbar_symbol_get_data(symbol)
	Barcode::ZBar::Symbol symbol

int
zbar_symbol_get_count(symbol)
	Barcode::ZBar::Symbol symbol

SV *
zbar_symbol_get_loc(symbol)
	Barcode::ZBar::Symbol symbol
    PREINIT:
        unsigned i, size;
    PPCODE:
        size = zbar_symbol_get_loc_size(symbol);
        EXTEND(SP, size);
        for(i = 0; i < size; i++) {
            AV *pt = (AV*)sv_2mortal((SV*)newAV());
            PUSHs(newRV((SV*)pt));
            av_push(pt, newSVuv(zbar_symbol_get_loc_x(symbol, i)));
            av_push(pt, newSVuv(zbar_symbol_get_loc_y(symbol, i)));
        }


MODULE = Barcode::ZBar	PACKAGE = Barcode::ZBar::Image	PREFIX = zbar_image_

Barcode::ZBar::Image
new(package)
        char *	package
    CODE:
        RETVAL = zbar_image_create();
    OUTPUT:
        RETVAL

void
DESTROY(image)
        Barcode::ZBar::Image	image
    CODE:
        zbar_image_destroy(image);

Barcode::ZBar::Image
zbar_image_convert(image, format)
        Barcode::ZBar::Image	image
	fourcc_t	format

Barcode::ZBar::Image
zbar_image_convert_resize(image, format, width, height)
        Barcode::ZBar::Image	image
	fourcc_t	format
	unsigned width
	unsigned height

fourcc_t
zbar_image_get_format(image)
        Barcode::ZBar::Image	image

unsigned
zbar_image_get_sequence(image)
        Barcode::ZBar::Image	image

void
get_size(image)
        Barcode::ZBar::Image	image
    PPCODE:
        EXTEND(SP, 2);
        mPUSHu(zbar_image_get_width(image));
        mPUSHu(zbar_image_get_height(image));

SV *
zbar_image_get_data(image)
        Barcode::ZBar::Image	image
    CODE:
	RETVAL = newSVpvn(zbar_image_get_data(image),
                          zbar_image_get_data_length(image));
    OUTPUT:
        RETVAL

SV *
get_symbols(image)
        Barcode::ZBar::Image	image
    PREINIT:
        const zbar_symbol_t *sym;
    PPCODE:
	sym = zbar_image_first_symbol(image);
	for(; sym; sym = zbar_symbol_next(sym))
            XPUSHs(sv_setref_pv(sv_newmortal(), "Barcode::ZBar::Symbol",
                   (void*)sym));

void
zbar_image_set_format(image, format)
        Barcode::ZBar::Image	image
	fourcc_t	format

void
zbar_image_set_sequence(image, seq_num)
        Barcode::ZBar::Image	image
	unsigned	seq_num

void
zbar_image_set_size(image, width, height)
        Barcode::ZBar::Image	image
	unsigned	width
	unsigned	height

void
zbar_image_set_data(image, data)
        Barcode::ZBar::Image	image
	SV *	data
    PREINIT:
        SV *old;
    CODE:
	if(!data || !SvOK(data)) {
            zbar_image_set_data(image, NULL, 0, NULL);
            zbar_image_set_userdata(image, NULL);
        }
        else if(SvPOK(data)) {
            /* FIXME is this copy of data or new ref to same data?
             * not sure this is correct:
             * need to retain a reference to image data,
             * but do not really want to copy it...maybe an RV?
             */
            SV *copy = newSVsv(data);
            STRLEN len;
            void *raw = SvPV(copy, len);
            zbar_image_set_data(image, raw, len, image_cleanup_handler);
            zbar_image_set_userdata(image, copy);
        }
        else
            croak("image data must be binary string");


MODULE = Barcode::ZBar	PACKAGE = Barcode::ZBar::Processor	PREFIX = zbar_processor_

Barcode::ZBar::Processor
new(package, threaded=0)
        char *	package
        bool	threaded
    CODE:
        RETVAL = zbar_processor_create(threaded);
    OUTPUT:
        RETVAL

void
DESTROY(processor)
        Barcode::ZBar::Processor	processor
    CODE:
        zbar_processor_destroy(processor);

void
zbar_processor_init(processor, video_device="/dev/video0", enable_display=1)
        Barcode::ZBar::Processor	processor
        const char *	video_device
	bool	enable_display
    CODE:
        check_error(zbar_processor_init(processor, video_device, enable_display),
                    processor);

void
zbar_processor_request_size(processor, width, height)
        Barcode::ZBar::Processor	processor
	unsigned	width
	unsigned	height
    CODE:
	check_error(zbar_processor_request_size(processor, width, height),
                    processor);

void
zbar_processor_force_format(processor, input_format=0, output_format=0)
        Barcode::ZBar::Processor	processor
	fourcc_t	input_format
	fourcc_t	output_format
    CODE:
	check_error(zbar_processor_force_format(processor, input_format, output_format),
                    processor);

void
zbar_processor_set_config(processor, symbology, config, value=1)
        Barcode::ZBar::Processor	processor
	zbar_symbol_type_t	symbology
	zbar_config_t	config
	int	value

config_error
zbar_processor_parse_config(processor, config_string)
        Barcode::ZBar::Processor	processor
        const char *config_string

bool
zbar_processor_is_visible(processor)
        Barcode::ZBar::Processor	processor
    CODE:
	check_error((RETVAL = zbar_processor_is_visible(processor)),
                    processor);
    OUTPUT:
	RETVAL

void
zbar_processor_set_visible(processor, visible=1)
        Barcode::ZBar::Processor	processor
	bool	visible
    CODE:
	check_error(zbar_processor_set_visible(processor, visible),
                    processor);

void
zbar_processor_set_active(processor, active=1)
        Barcode::ZBar::Processor	processor
	bool	active
    CODE:
	check_error(zbar_processor_set_active(processor, active),
                    processor);

int
zbar_processor_user_wait(processor, timeout=-1)
        Barcode::ZBar::Processor	processor
	timeout_t	timeout
    CODE:
	check_error((RETVAL = zbar_processor_user_wait(processor, timeout)),
                    processor);
    OUTPUT:
	RETVAL

int
process_one(processor, timeout=-1)
        Barcode::ZBar::Processor	processor
	timeout_t	timeout
    CODE:
	check_error((RETVAL = zbar_process_one(processor, timeout)),
                    processor);
    OUTPUT:
	RETVAL

int
process_image(processor, image)
        Barcode::ZBar::Processor	processor
        Barcode::ZBar::Image	image
    CODE:
	check_error((RETVAL = zbar_process_image(processor, image)),
                    processor);
    OUTPUT:
	RETVAL

void
zbar_processor_set_data_handler(processor, handler = 0, closure = 0)
        Barcode::ZBar::Processor	processor
	SV *	handler
	SV *	closure
    PREINIT:
        handler_wrapper_t *wrap;
        zbar_image_data_handler_t *callback = NULL;
    CODE:
        wrap = zbar_processor_get_userdata(processor);
        if(set_handler(&wrap, ST(0), handler, closure))
            callback = processor_handler;
        zbar_processor_set_data_handler(processor, callback, wrap);


MODULE = Barcode::ZBar	PACKAGE = Barcode::ZBar::ImageScanner	PREFIX = zbar_image_scanner_

Barcode::ZBar::ImageScanner
new(package)
        char *	package
    CODE:
        RETVAL = zbar_image_scanner_create();
    OUTPUT:
        RETVAL

void
DESTROY(scanner)
        Barcode::ZBar::ImageScanner	scanner
    CODE:
        zbar_image_scanner_destroy(scanner);

void
zbar_image_scanner_set_config(scanner, symbology, config, value=1)
        Barcode::ZBar::ImageScanner	scanner
	zbar_symbol_type_t	symbology
	zbar_config_t	config
	int	value

config_error
zbar_image_scanner_parse_config(scanner, config_string)
        Barcode::ZBar::ImageScanner	scanner
        const char *config_string

void
zbar_image_scanner_enable_cache(scanner, enable)
        Barcode::ZBar::ImageScanner	scanner
	int	enable

int
scan_image(scanner, image)
        Barcode::ZBar::ImageScanner	scanner
        Barcode::ZBar::Image	image
    CODE:
	RETVAL = zbar_scan_image(scanner, image);
    OUTPUT:
	RETVAL


MODULE = Barcode::ZBar	PACKAGE = Barcode::ZBar::Decoder	PREFIX = zbar_decoder_

Barcode::ZBar::Decoder
new(package)
        char *	package
    CODE:
        RETVAL = zbar_decoder_create();
    OUTPUT:
        RETVAL

void
DESTROY(decoder)
        Barcode::ZBar::Decoder	decoder
    CODE:
        /* FIXME cleanup handler wrapper */
        zbar_decoder_destroy(decoder);

void
zbar_decoder_set_config(decoder, symbology, config, value=1)
        Barcode::ZBar::Decoder	decoder
	zbar_symbol_type_t	symbology
	zbar_config_t	config
	int	value

config_error
zbar_decoder_parse_config(decoder, config_string)
        Barcode::ZBar::Decoder decoder
        const char *config_string

void
zbar_decoder_reset(decoder)
        Barcode::ZBar::Decoder	decoder

void
zbar_decoder_new_scan(decoder)
	Barcode::ZBar::Decoder	decoder

zbar_symbol_type_t
decode_width(decoder, width)
	Barcode::ZBar::Decoder	decoder
	unsigned	width
    CODE:
	RETVAL = zbar_decode_width(decoder, width);
    OUTPUT:
	RETVAL

zbar_color_t
zbar_decoder_get_color(decoder)
	Barcode::ZBar::Decoder	decoder

const char *
zbar_decoder_get_data(decoder)
	Barcode::ZBar::Decoder	decoder

zbar_symbol_type_t
zbar_decoder_get_type(decoder)
	Barcode::ZBar::Decoder	decoder

void
zbar_decoder_set_handler(decoder, handler = 0, closure = 0)
	Barcode::ZBar::Decoder	decoder
	SV *	handler
	SV *	closure
    PREINIT:
        handler_wrapper_t *wrap;
    CODE:
        wrap = zbar_decoder_get_userdata(decoder);
        zbar_decoder_set_handler(decoder, NULL);
        if(set_handler(&wrap, ST(0), handler, closure)) {
            zbar_decoder_set_userdata(decoder, wrap);
            zbar_decoder_set_handler(decoder, decoder_handler);
        }


MODULE = Barcode::ZBar	PACKAGE = Barcode::ZBar::Scanner	PREFIX = zbar_scanner_

Barcode::ZBar::Scanner
new(package, decoder = 0)
        char *	package
        Barcode::ZBar::Decoder	decoder
    CODE:
        RETVAL = zbar_scanner_create(decoder);
    OUTPUT:
        RETVAL

void
DESTROY(scanner)
        Barcode::ZBar::Scanner	scanner
    CODE:
        zbar_scanner_destroy(scanner);

zbar_symbol_type_t
zbar_scanner_reset(scanner)
	Barcode::ZBar::Scanner	scanner

zbar_symbol_type_t
zbar_scanner_new_scan(scanner)
	Barcode::ZBar::Scanner	scanner

zbar_color_t
zbar_scanner_get_color(scanner)
	Barcode::ZBar::Scanner	scanner

unsigned
zbar_scanner_get_width(scanner)
	Barcode::ZBar::Scanner	scanner

zbar_symbol_type_t
scan_y(scanner, y)
	Barcode::ZBar::Scanner	scanner
	int	y
    CODE:
	RETVAL = zbar_scan_y(scanner, y);
    OUTPUT:
	RETVAL
