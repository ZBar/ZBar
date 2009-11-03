//------------------------------------------------------------------------
//  Copyright 2009 (c) Jeff Brown <spadix@users.sourceforge.net>
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

#import <sys/times.h>
#import <limits.h>

#import <zbar/ZBarReaderController.h>
#import "ZBarHelpController.h"

#define MIN_QUALITY 10
#ifdef ZRC_DEBUG
# define zlog(fmt, ...) \
    NSLog(@"ZBarReaderController: " fmt , ##__VA_ARGS__)

static inline long timer_now ()
{
    struct tms now;
    times(&now);
    return(now.tms_utime + now.tms_stime);
}

static inline double timer_elapsed (long start, long end)
{
    double clk_tck = sysconf(_SC_CLK_TCK);
    if(!clk_tck)
        return(-1);
    return((end - start) / clk_tck);
}

#else
# define zlog(...)
#endif

NSString* const ZBarReaderControllerResults = @"ZBarReaderControllerResults";


@implementation ZBarReaderController

@synthesize readerDelegate, showsZBarControls, showsHelpOnFail;

- (id) init
{
    if(self = [super init]) {
        scanner = zbar_image_scanner_create();
        showsHelpOnFail = YES;
        if([UIImagePickerController
               isSourceTypeAvailable: UIImagePickerControllerSourceTypeCamera])
            self.sourceType = UIImagePickerControllerSourceTypeCamera;
    }
    return(self);
}

- (void) initOverlay
{
    CGRect r = self.view.bounds;
    overlay = [[UIView new] initWithFrame: r];
    overlay.backgroundColor = [UIColor clearColor];

    UIToolbar *toolbar = [UIToolbar new];
    toolbar.barStyle = UIBarStyleBlackOpaque;
    [toolbar sizeToFit];
    r.origin.y = r.size.height - 56;
    r.size.height = 56;
    toolbar.frame = r;
    [overlay addSubview: toolbar];
    [toolbar release];

    UIButton *btn = _ZBarButton(48, .1, .7, .1);
    btn.frame = CGRectMake(r.size.width / 4 + 4, r.origin.y + 4,
                           r.size.width / 2 - 8, 48);
    [btn addTarget: self
         action: @selector(takePicture)
         forControlEvents: UIControlEventTouchDown];
    [btn setTitle: @"Scan!"
         forState: UIControlStateNormal];
    [overlay addSubview: btn];

    btn = _ZBarButton(40, .85, .15, .15);
    btn.frame = CGRectMake(4, r.origin.y + 4,
                           r.size.width / 4 - 8, 48);
    [btn addTarget: self
         action: @selector(cancel)
         forControlEvents: UIControlEventTouchUpInside];
    [btn setTitle: @"done"
         forState: UIControlStateNormal];
    [overlay addSubview: btn];
}

- (void) viewDidLoad
{
    [super viewDidLoad];
    [super setDelegate: self];
    [self initOverlay];
}

- (void) cleanup
{
    [overlay release];
    overlay = nil;
}

- (void) viewDidUnload
{
    [self cleanup];
    [super viewDidUnload];
}

- (void) dealloc
{
    [self cleanup];
    if(scanner) {
        zbar_image_scanner_destroy(scanner);
        scanner = NULL;
    }
    [super dealloc];
}

- (void) cancel
{
    [self performSelector: @selector(imagePickerControllerDidCancel:)
          withObject: self
          afterDelay: 0];
}

- (void) viewWillAppear: (BOOL) animated
{
    if(self.sourceType == UIImagePickerControllerSourceTypeCamera &&
       showsZBarControls) {
        self.showsCameraControls = NO;
        self.cameraOverlayView = overlay;
    }
    [super viewWillAppear: animated];
}

- (void) setDelegate: (id <UINavigationControllerDelegate,
                           UIImagePickerControllerDelegate>) delegate
{
    // FIXME raise
    assert(0);
}

// image scanner config wrappers
- (void) parseConfig: (NSString*) cfg
{
    const char *str = [cfg UTF8String];
    zbar_image_scanner_parse_config(scanner, str);
    // FIXME throw errors
}

- (void) setSymbology: (zbar_symbol_type_t) sym
               config: (zbar_config_t) cfg
                   to: (int) val
{
    zbar_image_scanner_set_config(scanner, sym, cfg, val);
    // FIXME throw errors
}


// scan UIImage and return something enumerable
- (id <NSFastEnumeration>) scanImage: (UIImage*) img
{
#ifdef ZRC_DEBUG
    long t_start = timer_now();
    zlog(@"captured image %gx%g\n", img.size.width, img.size.height);
#endif

    int w = img.size.width;
    int h = img.size.height;
    if(w > 1280 || h > 1280) {
        w /= 2;
        h /= 2;
    }
    long datalen = w * h;
    uint8_t *raw = malloc(datalen);
    // FIXME handle OOM
    assert(raw);

    zbar_image_t *zimg = zbar_image_create();
    zbar_image_set_data(zimg, raw, datalen, zbar_image_free_data);
    zbar_image_set_format(zimg, *(int*)"Y800");
    zbar_image_set_size(zimg, w, h);

    // generate grayscale image data
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceGray();
    CGContextRef ctx =
        CGBitmapContextCreate(raw, w, h, 8, w, cs, kCGImageAlphaNone);
    CGColorSpaceRelease(cs);
    CGContextSetAllowsAntialiasing(ctx, 0);
    CGRect bbox = CGRectMake(0, 0, w, h);
    CGContextDrawImage(ctx, bbox, img.CGImage);
    CGContextRelease(ctx);

#ifdef ZRC_DEBUG
    long t_convert = timer_now();
    zlog(@"  converted to %dx%d Y800 in +%gs\n",
         w, h, timer_elapsed(t_start, t_convert));
#endif

    // limit the maximum number of scan passes
    int density;
    if(w > 480)
        density = (w / 240 + 1) / 2;
    else
        density = 1;
    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_X_DENSITY, density);

    if(h > 480)
        density = (h / 240 + 1) / 2;
    else
        density = 1;
    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_Y_DENSITY, density);

    int nsyms = zbar_scan_image(scanner, zimg);

#ifdef ZRC_DEBUG
    long t_scan = timer_now();
    zlog(@"  scanned %d raw symbols in +%gs\n",
         nsyms, timer_elapsed(t_convert, t_scan));
#endif

    NSMutableArray *syms = nil;
    if(nsyms) {
        // quality/type filtering
        int max_quality = MIN_QUALITY;
        const zbar_symbol_t *zsym = zbar_image_first_symbol(zimg);
        assert(zsym);
        for(; zsym; zsym = zbar_symbol_next(zsym)) {
            if(zbar_symbol_get_count(zsym))
                continue;

            zbar_symbol_type_t type = zbar_symbol_get_type(zsym);
            int quality;
            if(type == ZBAR_QRCODE)
                quality = INT_MAX;
            else
                quality = zbar_symbol_get_quality(zsym);

            if(quality < max_quality) {
                zlog(@"    type=%d quality=%d < %d\n",
                     type, quality, max_quality);
                continue;
            }

            if(max_quality < quality) {
                max_quality = quality;
                if(syms)
                    [syms removeAllObjects];
            }
            zlog(@"    type=%d quality=%d\n", type, quality);
            if(!syms)
                syms = [NSMutableArray arrayWithCapacity: 1];

            ZBarSymbol *sym = [[ZBarSymbol alloc] initWithSymbol: zsym];
            [syms addObject: sym];
            [sym release];
        }
    }
    zbar_image_destroy(zimg);

#ifdef ZRC_DEBUG
    long t_end = timer_now();
    zlog(@"read %d filtered symbols in %gs total\n",
          (!syms) ? 0 : [syms count], timer_elapsed(t_start, t_end));
#endif
    return(syms);
}

- (void)  imagePickerController: (UIImagePickerController*) picker
  didFinishPickingMediaWithInfo: (NSDictionary*) info
{
    UIImage *img = [info objectForKey: UIImagePickerControllerOriginalImage];

    id results = [self scanImage: img];
    if(results) {
        NSMutableDictionary *newinfo = [info mutableCopy];
        [newinfo setObject: results
                 forKey: ZBarReaderControllerResults];
        [readerDelegate imagePickerController: self
                        didFinishPickingMediaWithInfo: newinfo];
        return;
    }

    if(showsHelpOnFail) {
        ZBarHelpController *help = [ZBarHelpController new];
        help.delegate = self;
        [self presentModalViewController: help
              animated: YES];
        [help release];
    }

    //[readerDelegate zbarReaderControllerDidFailToRead: self];
}

- (void) imagePickerControllerDidCancel: (UIImagePickerController*) picker
{
    [readerDelegate imagePickerControllerDidCancel: self];
}

- (void)   actionSheet: (UIActionSheet*) sheet
  clickedButtonAtIndex: (NSInteger) idx
{
    if(!idx)
        [readerDelegate imagePickerControllerDidCancel: self];
}

@end
