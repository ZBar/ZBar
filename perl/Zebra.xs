//------------------------------------------------------------------------
//  Copyright 2008-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
//
//  This file is part of the Zebra Barcode Library.
//
//  The Zebra Barcode Library is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser Public License as
//  published by the Free Software Foundation; either version 2.1 of
//  the License, or (at your option) any later version.
//
//  The Zebra Barcode Library is distributed in the hope that it will be
//  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
//  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser Public License for more details.
//
//  You should have received a copy of the GNU Lesser Public License
//  along with the Zebra Barcode Library; if not, write to the Free
//  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//  Boston, MA  02110-1301  USA
//
//  http://sourceforge.net/projects/zebra
//------------------------------------------------------------------------
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include <zebra.h>

typedef zebra_symbol_t *Barcode__Zebra__Symbol;
typedef zebra_image_t *Barcode__Zebra__Image;
typedef zebra_processor_t *Barcode__Zebra__Processor;
typedef zebra_video_t *Barcode__Zebra__Video;
typedef zebra_window_t *Barcode__Zebra__Window;
typedef zebra_image_scanner_t *Barcode__Zebra__ImageScanner;
typedef zebra_decoder_t *Barcode__Zebra__Decoder;
typedef zebra_scanner_t *Barcode__Zebra__Scanner;
typedef void *Barcode__Zebra__Error;

typedef unsigned long fourcc_t;
typedef int timeout_t;
typedef int config_error;

typedef struct handler_wrapper_s {
    SV *instance;
    SV *handler;
    SV *closure;
} handler_wrapper_t;


static AV *LOOKUP_zebra_color_t = NULL;
static AV *LOOKUP_zebra_symbol_type_t = NULL;
static AV *LOOKUP_zebra_error_t = NULL;
static AV *LOOKUP_zebra_config_t = NULL;

#define CONSTANT(typ, prefix, sym, name)                \
    do {                                                \
        SV *c = newSViv(ZEBRA_ ## prefix ## sym);       \
        sv_setpv(c, name);                              \
        SvIOK_on(c);                                    \
        newCONSTSUB(stash, #sym, c);                    \
        av_store(LOOKUP_zebra_ ## typ ## _t,            \
                 ZEBRA_ ## prefix ## sym,               \
                 SvREFCNT_inc(c));                      \
    } while(0)

#define LOOKUP_ENUM(typ, val) \
    lookup_enum(LOOKUP_zebra_ ## typ ## _t, val)

static inline SV *lookup_enum (AV *lookup, int val)
{
    SV **tmp = av_fetch(lookup, val, 0);
    return((tmp) ? *tmp : sv_newmortal());
}

static inline void check_error (int rc, void *obj)
{
    if(rc < 0) {
        sv_setref_pv(get_sv("@", TRUE), "Barcode::Zebra::Error", obj);
        croak(NULL);
    }
}

static void image_cleanup_handler (zebra_image_t *image)
{
    SV *data = zebra_image_get_userdata(image);
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

static void processor_handler (zebra_image_t *image,
                               const void *userdata)
{
    SV *img;
    zebra_image_ref(image, 1);
    img = sv_setref_pv(sv_newmortal(), "Barcode::Zebra::Image", image);
    activate_handler((void*)userdata, img);
    SvREFCNT_dec(img);
}

static void decoder_handler (zebra_decoder_t *decoder)
{
    activate_handler(zebra_decoder_get_userdata(decoder), NULL);
}


MODULE = Barcode::Zebra	PACKAGE = Barcode::Zebra		PREFIX = zebra_

PROTOTYPES: ENABLE

BOOT:
    {
        HV *stash = gv_stashpv("Barcode::Zebra", TRUE);

        LOOKUP_zebra_color_t = newAV();
        CONSTANT(color, , SPACE, "SPACE");
        CONSTANT(color, , BAR, "BAR");
    }

SV *
zebra_version()
    PREINIT:
	unsigned major;
        unsigned minor;
    CODE:
        zebra_version(&major, &minor);
        RETVAL = newSVpvf("%u.%u", major, minor);
    OUTPUT:
        RETVAL

void
zebra_increase_verbosity()

void
zebra_set_verbosity(verbosity)
	int	verbosity

SV *
parse_config(config_string)
        const char *	config_string
    PREINIT:
        zebra_symbol_type_t sym;
        zebra_config_t cfg;
        int val;
    PPCODE:
        if(zebra_parse_config(config_string, &sym, &cfg, &val))
            croak("invalid configuration setting: %s", config_string);
        EXTEND(SP, 3);
        PUSHs(LOOKUP_ENUM(symbol_type, sym));
        PUSHs(LOOKUP_ENUM(config, cfg));
        mPUSHi(val);


MODULE = Barcode::Zebra	PACKAGE = Barcode::Zebra::Error	PREFIX = zebra_

BOOT:
    {
        HV *stash = gv_stashpv("Barcode::Zebra::Error", TRUE);

        LOOKUP_zebra_error_t = newAV();
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

zebra_error_t
get_error_code(err)
	Barcode::Zebra::Error	err
    CODE:
	RETVAL = _zebra_get_error_code(err);
    OUTPUT:
	RETVAL

const char *
error_string(err)
	Barcode::Zebra::Error	err
    CODE:
	RETVAL = _zebra_error_string(err, 1);
    OUTPUT:
	RETVAL


MODULE = Barcode::Zebra	PACKAGE = Barcode::Zebra::Config	PREFIX = zebra_config_

BOOT:
    {
        HV *stash = gv_stashpv("Barcode::Zebra::Config", TRUE);

        LOOKUP_zebra_config_t = newAV();
        CONSTANT(config, CFG_, ENABLE, "enable");
        CONSTANT(config, CFG_, ADD_CHECK, "add-check");
        CONSTANT(config, CFG_, EMIT_CHECK, "emit-check");
        CONSTANT(config, CFG_, ASCII, "ascii");
    }


MODULE = Barcode::Zebra	PACKAGE = Barcode::Zebra::Symbol	PREFIX = zebra_symbol_

BOOT:
    {
        HV *stash = gv_stashpv("Barcode::Zebra::Symbol", TRUE);

        LOOKUP_zebra_symbol_type_t = newAV();
        CONSTANT(symbol_type, , NONE, "None");
        CONSTANT(symbol_type, , PARTIAL, "Partial");
        CONSTANT(symbol_type, , EAN8, zebra_get_symbol_name(ZEBRA_EAN8));
        CONSTANT(symbol_type, , UPCE, zebra_get_symbol_name(ZEBRA_UPCE));
        CONSTANT(symbol_type, , ISBN10, zebra_get_symbol_name(ZEBRA_ISBN10));
        CONSTANT(symbol_type, , UPCA, zebra_get_symbol_name(ZEBRA_UPCA));
        CONSTANT(symbol_type, , EAN13, zebra_get_symbol_name(ZEBRA_EAN13));
        CONSTANT(symbol_type, , ISBN13, zebra_get_symbol_name(ZEBRA_ISBN13));
        CONSTANT(symbol_type, , I25, zebra_get_symbol_name(ZEBRA_I25));
        CONSTANT(symbol_type, , CODE39, zebra_get_symbol_name(ZEBRA_CODE39));
        CONSTANT(symbol_type, , PDF417, zebra_get_symbol_name(ZEBRA_PDF417));
        CONSTANT(symbol_type, , CODE128, zebra_get_symbol_name(ZEBRA_CODE128));
    }

zebra_symbol_type_t
zebra_symbol_get_type(symbol)
	Barcode::Zebra::Symbol symbol

const char *
zebra_symbol_get_data(symbol)
	Barcode::Zebra::Symbol symbol

int
zebra_symbol_get_count(symbol)
	Barcode::Zebra::Symbol symbol

SV *
zebra_symbol_get_loc(symbol)
	Barcode::Zebra::Symbol symbol
    PREINIT:
        unsigned i, size;
    PPCODE:
        size = zebra_symbol_get_loc_size(symbol);
        EXTEND(SP, size);
        for(i = 0; i < size; i++) {
            AV *pt = (AV*)sv_2mortal((SV*)newAV());
            PUSHs(newRV((SV*)pt));
            av_push(pt, newSVuv(zebra_symbol_get_loc_x(symbol, i)));
            av_push(pt, newSVuv(zebra_symbol_get_loc_y(symbol, i)));
        }


MODULE = Barcode::Zebra	PACKAGE = Barcode::Zebra::Image	PREFIX = zebra_image_

Barcode::Zebra::Image
new(package)
        char *	package
    CODE:
        RETVAL = zebra_image_create();
    OUTPUT:
        RETVAL

void
DESTROY(image)
        Barcode::Zebra::Image	image
    CODE:
        zebra_image_destroy(image);

Barcode::Zebra::Image
zebra_image_convert(image, format)
        Barcode::Zebra::Image	image
	fourcc_t	format

Barcode::Zebra::Image
zebra_image_convert_resize(image, format, width, height)
        Barcode::Zebra::Image	image
	fourcc_t	format
	unsigned width
	unsigned height

fourcc_t
zebra_image_get_format(image)
        Barcode::Zebra::Image	image

unsigned
zebra_image_get_sequence(image)
        Barcode::Zebra::Image	image

void
get_size(image)
        Barcode::Zebra::Image	image
    PPCODE:
        EXTEND(SP, 2);
        mPUSHu(zebra_image_get_width(image));
        mPUSHu(zebra_image_get_height(image));

SV *
zebra_image_get_data(image)
        Barcode::Zebra::Image	image
    CODE:
	RETVAL = newSVpvn(zebra_image_get_data(image),
                          zebra_image_get_data_length(image));
    OUTPUT:
        RETVAL

SV *
get_symbols(image)
        Barcode::Zebra::Image	image
    PREINIT:
        const zebra_symbol_t *sym;
    PPCODE:
	sym = zebra_image_first_symbol(image);
	for(; sym; sym = zebra_symbol_next(sym))
            XPUSHs(sv_setref_pv(sv_newmortal(), "Barcode::Zebra::Symbol",
                   (void*)sym));

void
zebra_image_set_format(image, format)
        Barcode::Zebra::Image	image
	fourcc_t	format

void
zebra_image_set_sequence(image, seq_num)
        Barcode::Zebra::Image	image
	unsigned	seq_num

void
zebra_image_set_size(image, width, height)
        Barcode::Zebra::Image	image
	unsigned	width
	unsigned	height

void
zebra_image_set_data(image, data)
        Barcode::Zebra::Image	image
	SV *	data
    PREINIT:
        SV *old;
    CODE:
	if(!data || !SvOK(data)) {
            zebra_image_set_data(image, NULL, 0, NULL);
            zebra_image_set_userdata(image, NULL);
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
            zebra_image_set_data(image, raw, len, image_cleanup_handler);
            zebra_image_set_userdata(image, copy);
        }
        else
            croak("image data must be binary string");


MODULE = Barcode::Zebra	PACKAGE = Barcode::Zebra::Processor	PREFIX = zebra_processor_

Barcode::Zebra::Processor
new(package, threaded=0)
        char *	package
        bool	threaded
    CODE:
        RETVAL = zebra_processor_create(threaded);
    OUTPUT:
        RETVAL

void
DESTROY(processor)
        Barcode::Zebra::Processor	processor
    CODE:
        zebra_processor_destroy(processor);

void
zebra_processor_init(processor, video_device="/dev/video0", enable_display=1)
        Barcode::Zebra::Processor	processor
        const char *	video_device
	bool	enable_display
    CODE:
        check_error(zebra_processor_init(processor, video_device, enable_display),
                    processor);

void
zebra_processor_request_size(processor, width, height)
        Barcode::Zebra::Processor	processor
	unsigned	width
	unsigned	height
    CODE:
	check_error(zebra_processor_request_size(processor, width, height),
                    processor);

void
zebra_processor_force_format(processor, input_format=0, output_format=0)
        Barcode::Zebra::Processor	processor
	fourcc_t	input_format
	fourcc_t	output_format
    CODE:
	check_error(zebra_processor_force_format(processor, input_format, output_format),
                    processor);

void
zebra_processor_set_config(processor, symbology, config, value=1)
        Barcode::Zebra::Processor	processor
	zebra_symbol_type_t	symbology
	zebra_config_t	config
	int	value

config_error
zebra_processor_parse_config(processor, config_string)
        Barcode::Zebra::Processor	processor
        const char *config_string

bool
zebra_processor_is_visible(processor)
        Barcode::Zebra::Processor	processor
    CODE:
	check_error((RETVAL = zebra_processor_is_visible(processor)),
                    processor);
    OUTPUT:
	RETVAL

void
zebra_processor_set_visible(processor, visible=1)
        Barcode::Zebra::Processor	processor
	bool	visible
    CODE:
	check_error(zebra_processor_set_visible(processor, visible),
                    processor);

void
zebra_processor_set_active(processor, active=1)
        Barcode::Zebra::Processor	processor
	bool	active
    CODE:
	check_error(zebra_processor_set_active(processor, active),
                    processor);

int
zebra_processor_user_wait(processor, timeout=-1)
        Barcode::Zebra::Processor	processor
	timeout_t	timeout
    CODE:
	check_error((RETVAL = zebra_processor_user_wait(processor, timeout)),
                    processor);
    OUTPUT:
	RETVAL

int
process_one(processor, timeout=-1)
        Barcode::Zebra::Processor	processor
	timeout_t	timeout
    CODE:
	check_error((RETVAL = zebra_process_one(processor, timeout)),
                    processor);
    OUTPUT:
	RETVAL

int
process_image(processor, image)
        Barcode::Zebra::Processor	processor
        Barcode::Zebra::Image	image
    CODE:
	check_error((RETVAL = zebra_process_image(processor, image)),
                    processor);
    OUTPUT:
	RETVAL

void
zebra_processor_set_data_handler(processor, handler = 0, closure = 0)
        Barcode::Zebra::Processor	processor
	SV *	handler
	SV *	closure
    PREINIT:
        handler_wrapper_t *wrap;
        zebra_image_data_handler_t *callback = NULL;
    CODE:
        wrap = zebra_processor_get_userdata(processor);
        if(set_handler(&wrap, ST(0), handler, closure))
            callback = processor_handler;
        zebra_processor_set_data_handler(processor, processor_handler, wrap);


MODULE = Barcode::Zebra	PACKAGE = Barcode::Zebra::ImageScanner	PREFIX = zebra_image_scanner_

Barcode::Zebra::ImageScanner
new(package)
        char *	package
    CODE:
        RETVAL = zebra_image_scanner_create();
    OUTPUT:
        RETVAL

void
DESTROY(scanner)
        Barcode::Zebra::ImageScanner	scanner
    CODE:
        zebra_image_scanner_destroy(scanner);

void
zebra_image_scanner_set_config(scanner, symbology, config, value=1)
        Barcode::Zebra::ImageScanner	scanner
	zebra_symbol_type_t	symbology
	zebra_config_t	config
	int	value

config_error
zebra_image_scanner_parse_config(scanner, config_string)
        Barcode::Zebra::ImageScanner	scanner
        const char *config_string

void
zebra_image_scanner_enable_cache(scanner, enable)
        Barcode::Zebra::ImageScanner	scanner
	int	enable

int
scan_image(scanner, image)
        Barcode::Zebra::ImageScanner	scanner
        Barcode::Zebra::Image	image
    CODE:
	RETVAL = zebra_scan_image(scanner, image);
    OUTPUT:
	RETVAL


MODULE = Barcode::Zebra	PACKAGE = Barcode::Zebra::Decoder	PREFIX = zebra_decoder_

Barcode::Zebra::Decoder
new(package)
        char *	package
    CODE:
        RETVAL = zebra_decoder_create();
    OUTPUT:
        RETVAL

void
DESTROY(decoder)
        Barcode::Zebra::Decoder	decoder
    CODE:
        /* FIXME cleanup handler wrapper */
        zebra_decoder_destroy(decoder);

void
zebra_decoder_set_config(decoder, symbology, config, value=1)
        Barcode::Zebra::Decoder	decoder
	zebra_symbol_type_t	symbology
	zebra_config_t	config
	int	value

config_error
zebra_decoder_parse_config(decoder, config_string)
        Barcode::Zebra::Decoder decoder
        const char *config_string

void
zebra_decoder_reset(decoder)
        Barcode::Zebra::Decoder	decoder

void
zebra_decoder_new_scan(decoder)
	Barcode::Zebra::Decoder	decoder

zebra_symbol_type_t
decode_width(decoder, width)
	Barcode::Zebra::Decoder	decoder
	unsigned	width
    CODE:
	RETVAL = zebra_decode_width(decoder, width);
    OUTPUT:
	RETVAL

zebra_color_t
zebra_decoder_get_color(decoder)
	Barcode::Zebra::Decoder	decoder

const char *
zebra_decoder_get_data(decoder)
	Barcode::Zebra::Decoder	decoder

zebra_symbol_type_t
zebra_decoder_get_type(decoder)
	Barcode::Zebra::Decoder	decoder

void
zebra_decoder_set_handler(decoder, handler = 0, closure = 0)
	Barcode::Zebra::Decoder	decoder
	SV *	handler
	SV *	closure
    PREINIT:
        handler_wrapper_t *wrap;
        zebra_decoder_handler_t *callback;
    CODE:
        wrap = zebra_decoder_get_userdata(decoder);
        zebra_decoder_set_handler(decoder, NULL);
        if(set_handler(&wrap, ST(0), handler, closure)) {
            zebra_decoder_set_userdata(decoder, wrap);
            zebra_decoder_set_handler(decoder, decoder_handler);
        }


MODULE = Barcode::Zebra	PACKAGE = Barcode::Zebra::Scanner	PREFIX = zebra_scanner_

Barcode::Zebra::Scanner
new(package, decoder = 0)
        char *	package
        Barcode::Zebra::Decoder	decoder
    CODE:
        RETVAL = zebra_scanner_create(decoder);
    OUTPUT:
        RETVAL

void
DESTROY(scanner)
        Barcode::Zebra::Scanner	scanner
    CODE:
        zebra_scanner_destroy(scanner);

zebra_symbol_type_t
zebra_scanner_reset(scanner)
	Barcode::Zebra::Scanner	scanner

zebra_symbol_type_t
zebra_scanner_new_scan(scanner)
	Barcode::Zebra::Scanner	scanner

zebra_color_t
zebra_scanner_get_color(scanner)
	Barcode::Zebra::Scanner	scanner

unsigned
zebra_scanner_get_width(scanner)
	Barcode::Zebra::Scanner	scanner

zebra_symbol_type_t
scan_y(scanner, y)
	Barcode::Zebra::Scanner	scanner
	int	y
    CODE:
	RETVAL = zebra_scan_y(scanner, y);
    OUTPUT:
	RETVAL
