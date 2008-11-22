/*------------------------------------------------------------------------
 *  Copyright 2007-2008 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the Zebra Barcode Library.
 *
 *  The Zebra Barcode Library is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The Zebra Barcode Library is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the Zebra Barcode Library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zebra
 *------------------------------------------------------------------------*/
#ifndef _ZEBRA_H_
#define _ZEBRA_H_

/** @file
 * Zebra Barcode Reader Library C API definition
 */

/** @mainpage
 *
 * interface to the barcode reader is available at several levels.
 * most applications will want to use the high-level interfaces:
 *
 * @section high-level High-Level Interfaces
 *
 * these interfaces wrap all library functionality into an easy-to-use
 * package for a specific toolkit:
 * - the "GTK+ 2.x widget" may be used with GTK GUI applications.  a
 *   Python wrapper is included for PyGtk
 * - the @ref zebra::QZebra "Qt4 widget" may be used with Qt GUI
 *   applications
 * - the Processor interface (in @ref c-processor "C" or @ref
 *   zebra::Processor "C++") adds a scanning window to an application
 *   with no GUI.
 *
 * @section mid-level Intermediate Interfaces
 *
 * building blocks used to construct high-level interfaces:
 * - the ImageScanner (in @ref c-imagescanner "C" or @ref
 *   zebra::ImageScanner "C++") looks for barcodes in a library defined
 *   image object
 * - the Window abstraction (in @ref c-window "C" or @ref
 *   zebra::Window "C++") sinks library images, displaying them on the
 *   platform display
 * - the Video abstraction (in @ref c-video "C" or @ref zebra::Video
 *   "C++") sources library images from a video device
 *
 * @section low-level Low-Level Interfaces
 *
 * direct interaction with barcode scanning and decoding:
 * - the Scanner (in @ref c-scanner "C" or @ref zebra::Scanner "C++")
 *   looks for barcodes in a linear intensity sample stream
 * - the Decoder (in @ref c-decoder "C" or @ref zebra::Decoder "C++")
 *   extracts barcodes from a stream of bar and space widths
 */

#ifdef __cplusplus

/** C++ namespace for library interfaces */
namespace zebra {
    extern "C" {
#endif


/** @name Global library interfaces */
/*@{*/

/** "color" of element: bar or space. */
typedef enum zebra_color_e {
    ZEBRA_SPACE = 0,    /**< light area or space between bars */
    ZEBRA_BAR = 1,      /**< dark area or colored bar segment */
} zebra_color_t;

/** decoded symbol type. */
typedef enum zebra_symbol_type_e {
    ZEBRA_NONE        =      0,   /**< no symbol decoded */
    ZEBRA_PARTIAL     =      1,   /**< intermediate status */
    ZEBRA_EAN8        =      8,   /**< EAN-8 */
    ZEBRA_UPCE        =      9,   /**< UPC-E */
    ZEBRA_ISBN10      =     10,   /**< ISBN-10 (from EAN-13). @since 0.4 */
    ZEBRA_UPCA        =     12,   /**< UPC-A */
    ZEBRA_EAN13       =     13,   /**< EAN-13 */
    ZEBRA_ISBN13      =     14,   /**< ISBN-13 (from EAN-13). @since 0.4 */
    ZEBRA_I25         =     25,   /**< Interleaved 2 of 5. @since 0.4 */
    ZEBRA_CODE39      =     39,   /**< Code 39. @since 0.4 */
    ZEBRA_PDF417      =     57,   /**< PDF417. @since 0.6 */
    ZEBRA_CODE128     =    128,   /**< Code 128 */
    ZEBRA_SYMBOL      = 0x00ff,   /**< mask for base symbol type */
    ZEBRA_ADDON2      = 0x0200,   /**< 2-digit add-on flag */
    ZEBRA_ADDON5      = 0x0500,   /**< 5-digit add-on flag */
    ZEBRA_ADDON       = 0x0700,   /**< add-on flag mask */
} zebra_symbol_type_t;

/** error codes. */
typedef enum zebra_error_e {
    ZEBRA_OK = 0,               /**< no error */
    ZEBRA_ERR_NOMEM,            /**< out of memory */
    ZEBRA_ERR_INTERNAL,         /**< internal library error */
    ZEBRA_ERR_UNSUPPORTED,      /**< unsupported request */
    ZEBRA_ERR_INVALID,          /**< invalid request */
    ZEBRA_ERR_SYSTEM,           /**< system error */
    ZEBRA_ERR_LOCKING,          /**< locking error */
    ZEBRA_ERR_BUSY,             /**< all resources busy */
    ZEBRA_ERR_XDISPLAY,         /**< X11 display error */
    ZEBRA_ERR_XPROTO,           /**< X11 protocol error */
    ZEBRA_ERR_CLOSED,           /**< output window is closed */
    ZEBRA_ERR_NUM               /**< number of error codes */
} zebra_error_t;

/** decoder configuration options.
 * @since 0.4
 */
typedef enum zebra_config_e {
    ZEBRA_CFG_ENABLE = 0,       /**< enable symbology/feature */
    ZEBRA_CFG_ADD_CHECK,        /**< enable check digit when optional */
    ZEBRA_CFG_EMIT_CHECK,       /**< return check digit when present */
    ZEBRA_CFG_ASCII,            /**< enable full ASCII character set */
    ZEBRA_CFG_NUM               /**< number of configs */
} zebra_config_t;

/** retrieve runtime library version information.
 * @param major set to the running major version (unless NULL)
 * @param minor set to the running minor version (unless NULL)
 * @returns 0
 */
extern int zebra_version(unsigned *major,
                         unsigned *minor);

/** set global library debug level.
 * @param verbosity desired debug level.  higher values create more spew
 */
extern void zebra_set_verbosity(int verbosity);

/** increase global library debug level.
 * eg, for -vvvv
 */
extern void zebra_increase_verbosity();

/** retrieve string name for symbol encoding.
 * @param sym symbol type encoding
 * @returns the static string name for the specified symbol type,
 * or "UNKNOWN" if the encoding is not recognized
 */
extern const char *zebra_get_symbol_name(zebra_symbol_type_t sym);

/** retrieve string name for addon encoding.
 * @param sym symbol type encoding
 * @returns static string name for any addon, or the empty string
 * if no addons were decoded
 */
extern const char *zebra_get_addon_name(zebra_symbol_type_t sym);

/** parse a configuration string of the form "[symbology.]config[=value]".
 * the config must match one of the recognized names.
 * the symbology, if present, must match one of the recognized names.
 * if symbology is unspecified, it will be set to 0.
 * if value is unspecified it will be set to 1.
 * @returns 0 if the config is parsed successfully, 1 otherwise
 * @since 0.4
 */
extern int zebra_parse_config(const char *config_string,
                              zebra_symbol_type_t *symbology,
                              zebra_config_t *config,
                              int *value);

/** @internal type unsafe error API (don't use) */
extern int _zebra_error_spew(const void *object,
                             int verbosity);
extern const char *_zebra_error_string(const void *object,
                                       int verbosity);
extern zebra_error_t _zebra_get_error_code(const void *object);

/*@}*/


/*------------------------------------------------------------*/
/** @name Symbol interface
 * decoded barcode symbol result object.  stores type, data, and image
 * location of decoded symbol.  all memory is owned by the library
 */
/*@{*/

struct zebra_symbol_s;
/** opaque decoded symbol object. */
typedef struct zebra_symbol_s zebra_symbol_t;

/** retrieve type of decoded symbol.
 * @returns the symbol type
 */
extern zebra_symbol_type_t zebra_symbol_get_type(const zebra_symbol_t *symbol);

/** retrieve ASCII data decoded from symbol.
 * @returns the data string
 */
extern const char *zebra_symbol_get_data(const zebra_symbol_t *symbol);

/** retrieve current cache count.  when the cache is enabled for the
 * image_scanner this provides inter-frame reliability and redundancy
 * information for video streams.
 * @returns < 0 if symbol is still uncertain.
 * @returns 0 if symbol is newly verified.
 * @returns > 0 for duplicate symbols
 */
extern int zebra_symbol_get_count(const zebra_symbol_t *symbol);

/** retrieve the number of points in the location polygon.  the
 * location polygon defines the image area that the symbol was
 * extracted from.
 * @returns the number of points in the location polygon
 * @note this is currently not a polygon, but the scan locations
 * where the symbol was decoded
 */
extern unsigned zebra_symbol_get_loc_size(const zebra_symbol_t *symbol);

/** retrieve location polygon x-coordinates.
 * points are specified by 0-based index.
 * @returns the x-coordinate for a point in the location polygon.
 * @returns -1 if index is out of range
 */
extern int zebra_symbol_get_loc_x(const zebra_symbol_t *symbol,
                                  unsigned index);

/** retrieve location polygon y-coordinates.
 * points are specified by 0-based index.
 * @returns the y-coordinate for a point in the location polygon.
 * @returns -1 if index is out of range
 */
extern int zebra_symbol_get_loc_y(const zebra_symbol_t *symbol,
                                  unsigned index);

/** iterate the result set.
 * @returns the next result symbol, or
 * @returns NULL when no more results are available
 */
extern const zebra_symbol_t *zebra_symbol_next(const zebra_symbol_t *symbol);

/** print XML symbol element representation to user result buffer.
 * @see http://zebra.sourceforge.net/2008/barcode.xsd for the schema.
 * @param buffer is the inout result pointer, it will be reallocated
 * with a larger size if necessary.
 * @param buflen is inout length of the result buffer.
 * @returns the buffer pointer
 * @since 0.6
 */
extern char *zebra_symbol_xml(const zebra_symbol_t *symbol,
                              char **buffer,
                              unsigned *buflen);

/*@}*/

/*------------------------------------------------------------*/
/** @name Image interface
 * stores image data samples along with associated format and size
 * metadata
 */
/*@{*/

struct zebra_image_s;
/** opaque image object. */
typedef struct zebra_image_s zebra_image_t;

/** cleanup handler callback function.
 * called to free sample data when an image is destroyed.
 */
typedef void (zebra_image_cleanup_handler_t)(zebra_image_t *image);

/** data handler callback function.
 * called when decoded symbol results are available for an image
 */
typedef void (zebra_image_data_handler_t)(zebra_image_t *image,
                                          const void *userdata);

/** new image constructor.
 * @returns a new image object with uninitialized data and format.
 * this image should be destroyed (using zebra_image_destroy()) as
 * soon as the application is finished with it
 */
extern zebra_image_t *zebra_image_create();

/** image destructor.  all images created by or returned to the
 * application should be destroyed using this function.  when an image
 * is destroyed, the associated data cleanup handler will be invoked
 * if available
 * @note make no assumptions about the image or the data buffer.
 * they may not be destroyed/cleaned immediately if the library
 * is still using them.  if necessary, use the cleanup handler hook
 * to keep track of image data buffers
 */
extern void zebra_image_destroy(zebra_image_t *image);

/** image reference count manipulation.
 * increment the reference count when you store a new reference to the
 * image.  decrement when the reference is no longer used.  do not
 * refer to the image any longer once the count is decremented.
 * zebra_image_ref(image, -1) is the same as zebra_image_destroy(image)
 * @since 0.5
 */
extern void zebra_image_ref(zebra_image_t *image,
                            int refs);

/** image format conversion.  refer to the documentation for supported
 * image formats
 * @returns a @em new image with the sample data from the original image
 * converted to the requested format.  the original image is
 * unaffected.
 * @note the converted image size may be rounded (up) due to format
 * constraints
 */
extern zebra_image_t *zebra_image_convert(const zebra_image_t *image,
                                          unsigned long format);

/** image format conversion with crop/pad.
 * if the requested size is larger than the image, the last row/column
 * are duplicated to cover the difference.  if the requested size is
 * smaller than the image, the extra rows/columns are dropped from the
 * right/bottom.
 * @returns a @em new image with the sample data from the original
 * image converted to the requested format and size.
 * @note the image is @em not scaled
 * @see zebra_image_convert()
 * @since 0.4
 */
extern zebra_image_t *zebra_image_convert_resize(const zebra_image_t *image,
                                                 unsigned long format,
                                                 unsigned width,
                                                 unsigned height);

/** retrieve the image format.
 * @returns the fourcc describing the format of the image sample data
 */
extern unsigned long zebra_image_get_format(const zebra_image_t *image);

/** retrieve a "sequence" (page/frame) number associated with this image.
 * @since 0.6
 */
extern unsigned zebra_image_get_sequence(const zebra_image_t *image);

/** retrieve the width of the image.
 * @returns the width in sample columns
 */
extern unsigned zebra_image_get_width(const zebra_image_t *image);

/** retrieve the height of the image.
 * @returns the height in sample rows
 */
extern unsigned zebra_image_get_height(const zebra_image_t *image);

/** return the image sample data.  the returned data buffer is only
 * valid until zebra_image_destroy() is called
 */
extern const void *zebra_image_get_data(const zebra_image_t *image);

/** return the size of image data.
 * @since 0.6
 */
unsigned long zebra_image_get_data_length(const zebra_image_t *img);

/** image_scanner decode result iterator.
 * @returns the first decoded symbol result for an image
 * or NULL if no results are available
 */
extern const zebra_symbol_t*
zebra_image_first_symbol(const zebra_image_t *image);

/** specify the fourcc image format code for image sample data.
 * refer to the documentation for supported formats.
 * @note this does not convert the data!
 * (see zebra_image_convert() for that)
 */
extern void zebra_image_set_format(zebra_image_t *image,
                                   unsigned long format);

/** associate a "sequence" (page/frame) number with this image.
 * @since 0.6
 */
extern void zebra_image_set_sequence(zebra_image_t *image,
                                     unsigned sequence_num);

/** specify the pixel size of the image.
 * @note this does not affect the data!
 */
extern void zebra_image_set_size(zebra_image_t *image,
                                 unsigned width,
                                 unsigned height);

/** specify image sample data.  when image data is no longer needed by
 * the library the specific data cleanup handler will be called
 * (unless NULL)
 * @note application image data will not be modified by the library
 */
extern void zebra_image_set_data(zebra_image_t *image,
                                 const void *data,
                                 unsigned long data_byte_length,
                                 zebra_image_cleanup_handler_t *cleanup_hndlr);

/** built-in cleanup handler.
 * passes the image data buffer to free()
 */
extern void zebra_image_free_data(zebra_image_t *image);

/** associate user specified data value with an image.
 * @since 0.5
 */
extern void zebra_image_set_userdata(zebra_image_t *image,
                                     void *userdata);

/** return user specified data value associated with the image.
 * @since 0.5
 */
extern void *zebra_image_get_userdata(const zebra_image_t *image);

/** dump raw image data to a file for debug.
 * the data will be prefixed with a 16 byte header consisting of:
 *   - 4 bytes uint = 0x676d697a ("zimg")
 *   - 4 bytes format fourcc
 *   - 2 bytes width
 *   - 2 bytes height
 *   - 4 bytes size of following image data in bytes
 * this header can be dumped w/eg:
 * @verbatim
       od -Ax -tx1z -N16 -w4 [file]
@endverbatim
 * for some formats the image can be displayed/converted using
 * ImageMagick, eg:
 * @verbatim
       display -size 640x480+16 [-depth ?] [-sampling-factor ?x?] \
           {GRAY,RGB,UYVY,YUV}:[file]
@endverbatim
 *
 * @param image the image object to dump
 * @param filebase base filename, appended with ".XXXX.zimg" where
 * XXXX is the format fourcc
 * @returns 0 on success or a system error code on failure
 */
extern int zebra_image_write(const zebra_image_t *image,
                             const char *filebase);

/** read back an image in the format written by zebra_image_write()
 * @note TBD
 */
extern zebra_image_t *zebra_image_read(char *filename);

/*@}*/

/*------------------------------------------------------------*/
/** @name Processor interface
 * @anchor c-processor
 * high-level self-contained image processor.
 * processes video and images for barcodes, optionally displaying
 * images to a library owned output window
 */
/*@{*/

struct zebra_processor_s;
/** opaque standalone processor object. */
typedef struct zebra_processor_s zebra_processor_t;

/** constructor.
 * if threaded is set and threading is available the processor
 * will spawn threads where appropriate to avoid blocking and
 * improve responsiveness
 */
extern zebra_processor_t *zebra_processor_create(int threaded);

/** destructor.  cleans up all resources associated with the processor
 */
extern void zebra_processor_destroy(zebra_processor_t *processor);

/** (re)initialization.
 * opens a video input device and/or prepares to display output
 */
extern int zebra_processor_init(zebra_processor_t *processor,
                                const char *video_device,
                                int enable_display);

/** force specific input and output formats for debug/testing.
 * @note must be called before zebra_processor_init()
 */
extern int zebra_processor_force_format (zebra_processor_t *processor,
                                         unsigned long input_format,
                                         unsigned long output_format);

/** setup result handler callback.
 * the specified function will be called by the processor whenever
 * new results are available from the video stream or a static image.
 * pass a NULL value to disable callbacks.
 * @param userdata is set as with zebra_processor_set_userdata().
 * @returns the previously registered handler
 */
extern zebra_image_data_handler_t*
zebra_processor_set_data_handler(zebra_processor_t *processor,
                                 zebra_image_data_handler_t *handler,
                                 const void *userdata);

/** associate user specified data value with the processor.
 * @since 0.6
 */
extern void zebra_processor_set_userdata(zebra_processor_t *processor,
                                         void *userdata);

/** return user specified data value associated with the processor.
 * @since 0.6
 */
extern void *zebra_processor_get_userdata(const zebra_processor_t *processor);

/** set config for indicated symbology (0 for all) to specified value.
 * @returns 0 for success, non-0 for failure (config does not apply to
 * specified symbology, or value out of range)
 * @see zebra_decoder_set_config()
 * @since 0.4
 */
extern int zebra_processor_set_config(zebra_processor_t *processor,
                                      zebra_symbol_type_t symbology,
                                      zebra_config_t config,
                                      int value);

/** parse configuration string using zebra_parse_config()
 * and apply to processor using zebra_processor_set_config().
 * @returns 0 for success, non-0 for failure
 * @see zebra_parse_config()
 * @see zebra_processor_set_config()
 * @since 0.4
 */
static inline int zebra_processor_parse_config (zebra_processor_t *processor,
                                                const char *config_string)
{
    zebra_symbol_type_t sym;
    zebra_config_t cfg;
    int val;
    return(zebra_parse_config(config_string, &sym, &cfg, &val) ||
           zebra_processor_set_config(processor, sym, cfg, val));
}

/** retrieve the current state of the ouput window.
 * @returns 1 if the output window is currently displayed, 0 if not.
 * @returns -1 if an error occurs
 */
extern int zebra_processor_is_visible(zebra_processor_t *processor);

/** show or hide the display window owned by the library.
 * the size will be adjusted to the input size
 */
extern int zebra_processor_set_visible(zebra_processor_t *processor,
                                       int visible);

/** control the processor in free running video mode.
 * only works if video input is initialized. if threading is in use,
 * scanning will occur in the background, otherwise this is only
 * useful wrapping calls to zebra_processor_user_wait(). if the
 * library output window is visible, video display will be enabled.
 */
extern int zebra_processor_set_active(zebra_processor_t *processor,
                                      int active);

/** wait for input to the display window from the user
 * (via mouse or keyboard).
 * @returns >0 when input is received, 0 if timeout ms expired
 * with no input or -1 in case of an error
 */
extern int zebra_processor_user_wait(zebra_processor_t *processor,
                                     int timeout);

/** process from the video stream until a result is available,
 * or the timeout (in milliseconds) expires.
 * specify a timeout of -1 to scan indefinitely
 * (zebra_processor_set_active() may still be used to abort the scan
 * from another thread).
 * if the library window is visible, video display will be enabled.
 * @note that multiple results may still be returned (despite the
 * name).
 * @returns >0 if symbols were successfully decoded,
 * 0 if no symbols were found (ie, the timeout expired)
 * or -1 if an error occurs
 */
extern int zebra_process_one(zebra_processor_t *processor,
                             int timeout);

/** process the provided image for barcodes.
 * if the library window is visible, the image will be displayed.
 * @returns >0 if symbols were successfully decoded,
 * 0 if no symbols were found or -1 if an error occurs
 */
extern int zebra_process_image(zebra_processor_t *processor,
                               zebra_image_t *image);

/** display detail for last processor error to stderr.
 * @returns a non-zero value suitable for passing to exit()
 */
static inline int
zebra_processor_error_spew (const zebra_processor_t *processor,
                            int verbosity)
{
    return(_zebra_error_spew(processor, verbosity));
}

/** retrieve the detail string for the last processor error. */
static inline const char*
zebra_processor_error_string (const zebra_processor_t *processor,
                              int verbosity)
{
    return(_zebra_error_string(processor, verbosity));
}

/** retrieve the type code for the last processor error. */
static inline zebra_error_t
zebra_processor_get_error_code (const zebra_processor_t *processor)
{
    return(_zebra_get_error_code(processor));
}

/*@}*/

/*------------------------------------------------------------*/
/** @name Video interface
 * @anchor c-video
 * mid-level video source abstraction.
 * captures images from a video device
 */
/*@{*/

struct zebra_video_s;
/** opaque video object. */
typedef struct zebra_video_s zebra_video_t;

/** constructor. */
extern zebra_video_t *zebra_video_create();

/** destructor. */
extern void zebra_video_destroy(zebra_video_t *video);

/** open and probe a video device.
 * the device specified by platform specific unique name
 * (v4l device node path in *nix eg "/dev/video",
 *  DirectShow DevicePath property in windows).
 * @returns 0 if successful or -1 if an error occurs
 */
extern int zebra_video_open(zebra_video_t *video,
                            const char *device);

/** retrieve file descriptor associated with open *nix video device
 * useful for using select()/poll() to tell when new images are
 * available (NB v4l2 only!!).
 * @returns the file descriptor or -1 if the video device is not open
 * or the driver only supports v4l1
 */
extern int zebra_video_get_fd(const zebra_video_t *video);

/** retrieve current output image width.
 * @returns the width or 0 if the video device is not open
 */
extern int zebra_video_get_width(const zebra_video_t *video);

/** retrieve current output image height.
 * @returns the height or 0 if the video device is not open
 */
extern int zebra_video_get_height(const zebra_video_t *video);

/** initialize video using a specific format for debug.
 * use zebra_negotiate_format() to automatically select and initialize
 * the best available format
 */
extern int zebra_video_init(zebra_video_t *video,
                            unsigned long format);

/** start/stop video capture.
 * all buffered images are retired when capture is disabled.
 * @returns 0 if successful or -1 if an error occurs
 */
extern int zebra_video_enable(zebra_video_t *video,
                              int enable);

/** retrieve next captured image.  blocks until an image is available.
 * @returns NULL if video is not enabled or an error occurs
 */
extern zebra_image_t *zebra_video_next_image(zebra_video_t *video);

/** display detail for last video error to stderr.
 * @returns a non-zero value suitable for passing to exit()
 */
static inline int zebra_video_error_spew (const zebra_video_t *video,
                                          int verbosity)
{
    return(_zebra_error_spew(video, verbosity));
}

/** retrieve the detail string for the last video error. */
static inline const char *zebra_video_error_string (const zebra_video_t *video,
                                                    int verbosity)
{
    return(_zebra_error_string(video, verbosity));
}

/** retrieve the type code for the last video error. */
static inline zebra_error_t
zebra_video_get_error_code (const zebra_video_t *video)
{
    return(_zebra_get_error_code(video));
}

/*@}*/

/*------------------------------------------------------------*/
/** @name Window interface
 * @anchor c-window
 * mid-level output window abstraction.
 * displays images to user-specified platform specific output window
 */
/*@{*/

struct zebra_window_s;
/** opaque window object. */
typedef struct zebra_window_s zebra_window_t;

/** constructor. */
extern zebra_window_t *zebra_window_create();

/** destructor. */
extern void zebra_window_destroy(zebra_window_t *window);

/** associate reader with an existing platform window.
 * This can be any "Drawable" for X Windows or a "HWND" for windows.
 * input images will be scaled into the output window.
 * pass NULL to detach from the resource, further input will be
 * ignored
 */
extern int zebra_window_attach(zebra_window_t *window,
                               void *x11_display_w32_hwnd,
                               unsigned long x11_drawable);

/** control content level of the reader overlay.
 * the overlay displays graphical data for informational or debug
 * purposes.  higher values increase the level of annotation (possibly
 * decreasing performance). @verbatim
    0 = disable overlay
    1 = outline decoded symbols (default)
    2 = also track and display input frame rate
@endverbatim
 */
extern void zebra_window_set_overlay(zebra_window_t *window,
                                     int level);

/** draw a new image into the output window. */
extern int zebra_window_draw(zebra_window_t *window,
                             zebra_image_t *image);

/** redraw the last image (exposure handler). */
extern int zebra_window_redraw(zebra_window_t *window);

/** resize the image window (reconfigure handler).
 * this does @em not update the contents of the window
 * @since 0.3, changed in 0.4 to not redraw window
 */
extern int zebra_window_resize(zebra_window_t *window,
                               unsigned width,
                               unsigned height);

/** display detail for last window error to stderr.
 * @returns a non-zero value suitable for passing to exit()
 */
static inline int zebra_window_error_spew (const zebra_window_t *window,
                                           int verbosity)
{
    return(_zebra_error_spew(window, verbosity));
}

/** retrieve the detail string for the last window error. */
static inline const char*
zebra_window_error_string (const zebra_window_t *window,
                           int verbosity)
{
    return(_zebra_error_string(window, verbosity));
}

/** retrieve the type code for the last window error. */
static inline zebra_error_t
zebra_window_get_error_code (const zebra_window_t *window)
{
    return(_zebra_get_error_code(window));
}


/** select a compatible format between video input and output window.
 * the selection algorithm attempts to use a format shared by
 * video input and window output which is also most useful for
 * barcode scanning.  if a format conversion is necessary, it will
 * heuristically attempt to minimize the cost of the conversion
 */
extern int zebra_negotiate_format(zebra_video_t *video,
                                  zebra_window_t *window);

/*@}*/

/*------------------------------------------------------------*/
/** @name Image Scanner interface
 * @anchor c-imagescanner
 * mid-level image scanner interface.
 * reads barcodes from 2-D images
 */
/*@{*/

struct zebra_image_scanner_s;
/** opaque image scanner object. */
typedef struct zebra_image_scanner_s zebra_image_scanner_t;

/** constructor. */
extern zebra_image_scanner_t *zebra_image_scanner_create();

/** destructor. */
extern void zebra_image_scanner_destroy(zebra_image_scanner_t *scanner);

/** setup result handler callback.
 * the specified function will be called by the scanner whenever
 * new results are available from a decoded image.
 * pass a NULL value to disable callbacks.
 * @returns the previously registered handler
 */
extern zebra_image_data_handler_t*
zebra_image_scanner_set_data_handler(zebra_image_scanner_t *scanner,
                                     zebra_image_data_handler_t *handler,
                                     const void *userdata);


/** set config for indicated symbology (0 for all) to specified value.
 * @returns 0 for success, non-0 for failure (config does not apply to
 * specified symbology, or value out of range)
 * @see zebra_decoder_set_config()
 * @since 0.4
 */
extern int zebra_image_scanner_set_config(zebra_image_scanner_t *scanner,
                                          zebra_symbol_type_t symbology,
                                          zebra_config_t config,
                                          int value);

/** parse configuration string using zebra_parse_config()
 * and apply to image scanner using zebra_image_scanner_set_config().
 * @returns 0 for success, non-0 for failure
 * @see zebra_parse_config()
 * @see zebra_image_scanner_set_config()
 * @since 0.4
 */
static inline int
zebra_image_scanner_parse_config (zebra_image_scanner_t *scanner,
                                  const char *config_string)
{
    zebra_symbol_type_t sym;
    zebra_config_t cfg;
    int val;
    return(zebra_parse_config(config_string, &sym, &cfg, &val) ||
           zebra_image_scanner_set_config(scanner, sym, cfg, val));
}

/** enable or disable the inter-image result cache (default disabled).
 * mostly useful for scanning video frames, the cache filters
 * duplicate results from consecutive images, while adding some
 * consistency checking and hysteresis to the results.
 * this interface also clears the cache
 */
extern void zebra_image_scanner_enable_cache(zebra_image_scanner_t *scanner,
                                             int enable);

/** scan for symbols in provided image.
 * @returns >0 if symbols were successfully decoded from the image,
 * 0 if no symbols were found or -1 if an error occurs
 */
extern int zebra_scan_image(zebra_image_scanner_t *scanner,
                            zebra_image_t *image);

/*@}*/

/*------------------------------------------------------------*/
/** @name Decoder interface
 * @anchor c-decoder
 * low-level bar width stream decoder interface.
 * identifies symbols and extracts encoded data
 */
/*@{*/

struct zebra_decoder_s;
/** opaque decoder object. */
typedef struct zebra_decoder_s zebra_decoder_t;

/** decoder data handler callback function.
 * called by decoder when new data has just been decoded
 */
typedef void (zebra_decoder_handler_t)(zebra_decoder_t *decoder);

/** constructor. */
extern zebra_decoder_t *zebra_decoder_create();

/** destructor. */
extern void zebra_decoder_destroy(zebra_decoder_t *decoder);

/** set config for indicated symbology (0 for all) to specified value.
 * @returns 0 for success, non-0 for failure (config does not apply to
 * specified symbology, or value out of range)
 * @since 0.4
 */
extern int zebra_decoder_set_config(zebra_decoder_t *decoder,
                                    zebra_symbol_type_t symbology,
                                    zebra_config_t config,
                                    int value);

/** parse configuration string using zebra_parse_config()
 * and apply to decoder using zebra_decoder_set_config().
 * @returns 0 for success, non-0 for failure
 * @see zebra_parse_config()
 * @see zebra_decoder_set_config()
 * @since 0.4
 */
static inline int zebra_decoder_parse_config (zebra_decoder_t *decoder,
                                              const char *config_string)
{
    zebra_symbol_type_t sym;
    zebra_config_t cfg;
    int val;
    return(zebra_parse_config(config_string, &sym, &cfg, &val) ||
           zebra_decoder_set_config(decoder, sym, cfg, val));
}

/** clear all decoder state.
 * any partial symbols are flushed
 */
extern void zebra_decoder_reset(zebra_decoder_t *decoder);

/** mark start of a new scan pass.
 * clears any intra-symbol state and resets color to ::ZEBRA_SPACE.
 * any partially decoded symbol state is retained
 */
extern void zebra_decoder_new_scan(zebra_decoder_t *decoder);

/** process next bar/space width from input stream.
 * the width is in arbitrary relative units.  first value of a scan
 * is ::ZEBRA_SPACE width, alternating from there.
 * @returns appropriate symbol type if width completes
 * decode of a symbol (data is available for retrieval)
 * @returns ::ZEBRA_PARTIAL as a hint if part of a symbol was decoded
 * @returns ::ZEBRA_NONE (0) if no new symbol data is available
 */
extern zebra_symbol_type_t zebra_decode_width(zebra_decoder_t *decoder,
                                              unsigned width);

/** retrieve color of @em next element passed to
 * zebra_decode_width(). */
extern zebra_color_t zebra_decoder_get_color(const zebra_decoder_t *decoder);

/** retrieve last decoded data in ASCII format.
 * @returns the data string or NULL if no new data available.
 * the returned data buffer is owned by library, contents are only
 * valid between non-0 return from zebra_decode_width and next library
 * call
 */
extern const char *zebra_decoder_get_data(const zebra_decoder_t *decoder);

/** retrieve last decoded symbol type.
 * @returns the type or ::ZEBRA_NONE if no new data available
 */
extern zebra_symbol_type_t
zebra_decoder_get_type(const zebra_decoder_t *decoder);

/** setup data handler callback.
 * the registered function will be called by the decoder
 * just before zebra_decode_width() returns a non-zero value.
 * pass a NULL value to disable callbacks.
 * @returns the previously registered handler
 */
extern zebra_decoder_handler_t*
zebra_decoder_set_handler(zebra_decoder_t *decoder,
                          zebra_decoder_handler_t *handler);

/** associate user specified data value with the decoder. */
extern void zebra_decoder_set_userdata(zebra_decoder_t *decoder,
                                       void *userdata);

/** return user specified data value associated with the decoder. */
extern void *zebra_decoder_get_userdata(const zebra_decoder_t *decoder);

/*@}*/

/*------------------------------------------------------------*/
/** @name Scanner interface
 * @anchor c-scanner
 * low-level linear intensity sample stream scanner interface.
 * identifies "bar" edges and measures width between them.
 * optionally passes to bar width decoder
 */
/*@{*/

struct zebra_scanner_s;
/** opaque scanner object. */
typedef struct zebra_scanner_s zebra_scanner_t;

/** constructor.
 * if decoder is non-NULL it will be attached to scanner
 * and called automatically at each new edge
 * current color is initialized to ::ZEBRA_SPACE
 * (so an initial BAR->SPACE transition may be discarded)
 */
extern zebra_scanner_t *zebra_scanner_create(zebra_decoder_t *decoder);

/** destructor. */
extern void zebra_scanner_destroy(zebra_scanner_t *scanner);

/** clear all scanner state.
 * also resets an associated decoder
 */
extern zebra_symbol_type_t zebra_scanner_reset(zebra_scanner_t *scanner);

/** mark start of a new scan pass. resets color to ::ZEBRA_SPACE.
 * also updates an associated decoder.
 * @returns any decode results flushed from the pipeline
 * @note when not using callback handlers, the return value should
 * be checked the same as zebra_scan_y()
 */
extern zebra_symbol_type_t zebra_scanner_new_scan(zebra_scanner_t *scanner);

/** process next sample intensity value.
 * intensity (y) is in arbitrary relative units.
 * @returns result of zebra_decode_width() if a decoder is attached,
 * otherwise @returns (::ZEBRA_PARTIAL) when new edge is detected
 * or 0 (::ZEBRA_NONE) if no new edge is detected
 */
extern zebra_symbol_type_t zebra_scan_y(zebra_scanner_t *scanner,
                                        int y);

/** process next sample from RGB (or BGR) triple. */
static inline zebra_symbol_type_t zebra_scan_rgb24 (zebra_scanner_t *scanner,
                                                    unsigned char *rgb)
{
    return(zebra_scan_y(scanner, rgb[0] + rgb[1] + rgb[2]));
}

/** retrieve last scanned width. */
extern unsigned zebra_scanner_get_width(const zebra_scanner_t *scanner);

/** retrieve last scanned color. */
extern zebra_color_t zebra_scanner_get_color(const zebra_scanner_t *scanner);

/*@}*/

#ifdef __cplusplus
    }
}

# include "zebra/Exception.h"
# include "zebra/Decoder.h"
# include "zebra/Scanner.h"
# include "zebra/Symbol.h"
# include "zebra/Image.h"
# include "zebra/ImageScanner.h"
# include "zebra/Video.h"
# include "zebra/Window.h"
# include "zebra/Processor.h"
#endif

#endif
