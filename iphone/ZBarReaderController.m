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

#import <zbar/ZBarReaderController.h>
#import "ZBarHelpController.h"
#import "debug.h"

#define MIN_QUALITY 10

NSString* const ZBarReaderControllerResults = @"ZBarReaderControllerResults";

// expose undocumented API
CGImageRef UIGetScreenImage(void);

@implementation ZBarReaderController

@synthesize scanner, readerDelegate, showsHelpOnFail, takesPicture, cameraMode;
@dynamic showsZBarControls;

- (id) init
{
    if(self = [super init]) {
        showsHelpOnFail = YES;
        hasOverlay = showsZBarControls =
            [self respondsToSelector: @selector(cameraOverlayView)];
        scanner = [ZBarImageScanner new];
        if([UIImagePickerController
               isSourceTypeAvailable: UIImagePickerControllerSourceTypeCamera])
            self.sourceType = UIImagePickerControllerSourceTypeCamera;
        cameraMode = ZBarReaderControllerCameraModeSampling;
    }
    return(self);
}

- (void) showHelpOverlay
{
    if(!help) {
        help = [[ZBarHelpController alloc]
                   initWithTitle: @"Barcode Reader"];
        help.delegate = self;
    }
    help.wantsFullScreenLayout = YES;
    help.view.alpha = 0;
    [[self cameraOverlayView] addSubview: help.view];
    [UIView beginAnimations: @"ZBarHelp"
            context: nil];
    help.view.alpha = 1;
    [UIView commitAnimations];
}

- (void) initOverlay
{
    CGRect bounds = self.view.bounds;
    overlay = [[UIView alloc] initWithFrame: bounds];
    overlay.backgroundColor = [UIColor clearColor];

    CGRect r = bounds;
    r.size.height -= 54;
    boxView = [[UIView alloc] initWithFrame: r];

    boxLayer = [CALayer new];
    boxLayer.frame = r;
    boxLayer.borderWidth = 1;
    boxLayer.borderColor = [UIColor greenColor].CGColor;
    [boxView.layer addSublayer: boxLayer];

    toolbar = [UIToolbar new];
    toolbar.barStyle = UIBarStyleBlackOpaque;
    r.origin.y = r.size.height;
    r.size.height = 54;
    toolbar.frame = r;

    cancelBtn = [[UIBarButtonItem alloc]
                    initWithBarButtonSystemItem: UIBarButtonSystemItemCancel
                    target: self
                    action: @selector(cancel)];
    cancelBtn.width = r.size.width / 4 - 16;

    scanBtn = [[UIBarButtonItem alloc]
                  initWithTitle: @"Scan!"
                  style: UIBarButtonItemStyleDone
                  target: self
                  action: @selector(scan)];
    scanBtn.width = r.size.width / 2 - 16;

    for(int i = 0; i < 2; i++)
        space[i] = [[UIBarButtonItem alloc]
                       initWithBarButtonSystemItem:
                           UIBarButtonSystemItemFlexibleSpace
                       target: nil
                       action: nil];

    space[2] = [[UIBarButtonItem alloc]
                    initWithBarButtonSystemItem:
                        UIBarButtonSystemItemFixedSpace
                    target: nil
                    action: nil];
    space[2].width = r.size.width / 4 - 16;

    infoBtn = [[UIButton buttonWithType: UIButtonTypeInfoLight] retain];
    r.origin.x = r.size.width - 54;
    r.size.width = 54;
    infoBtn.frame = r;
    [infoBtn addTarget: self
             action: @selector(showHelpOverlay)
             forControlEvents: UIControlEventTouchUpInside];
}

- (void) viewDidLoad
{
    [super viewDidLoad];
    [super setDelegate: self];
    if(hasOverlay)
        [self initOverlay];
}

- (void) cleanup
{
    [overlay release];
    overlay = nil;
    [boxView release];
    boxView = nil;
    [boxLayer release];
    boxLayer = nil;
    [toolbar release];
    toolbar = nil;
    [cancelBtn release];
    cancelBtn = nil;
    [scanBtn release];
    scanBtn = nil;
    for(int i = 0; i < 3; i++) {
        [space[i] release];
        space[i] = nil;
    }
    [infoBtn release];
    infoBtn = nil;
    [help release];
    help = nil;
}

- (void) viewDidUnload
{
    [self cleanup];
    [super viewDidUnload];
}

- (void) dealloc
{
    [self cleanup];
    [scanner release];
    scanner = nil;
    [super dealloc];
}

- (void) scan
{
    scanBtn.enabled = NO;
    self.view.userInteractionEnabled = NO;
    [self takePicture];
}

- (void) cancel
{
    [self performSelector: @selector(imagePickerControllerDidCancel:)
          withObject: self
          afterDelay: 0.1];
}

- (void) reenable
{
    scanBtn.enabled = YES;
    self.view.userInteractionEnabled = YES;
}

- (void) viewWillAppear: (BOOL) animated
{
    if(help) {
        [help.view removeFromSuperview];
        [help release];
        help = nil;
    }

    if(hasOverlay &&
       self.sourceType == UIImagePickerControllerSourceTypeCamera) {
        if(showsZBarControls || ![self cameraOverlayView])
            [self setCameraOverlayView: overlay];

        UIView *activeOverlay = [self cameraOverlayView];

        if(showsZBarControls) {
            if(!toolbar.superview) {
                [overlay addSubview: toolbar];
                [overlay addSubview: infoBtn];
            }
            [self setShowsCameraControls: NO];
        }
        else {
            [toolbar removeFromSuperview];
            [infoBtn removeFromSuperview];
            if(activeOverlay == overlay)
                [self setShowsCameraControls: YES];
        }

        self.view.userInteractionEnabled = YES;
        if(cameraMode == ZBarReaderControllerCameraModeSampling)
            toolbar.items = [NSArray arrayWithObjects:
                                                     cancelBtn, space[0], nil];
        else {
            scanBtn.enabled = NO;
            toolbar.items = [NSArray arrayWithObjects:
                        cancelBtn, space[0], scanBtn, space[1], space[2], nil];

            [self performSelector: @selector(reenable)
                  withObject: nil
                  afterDelay: .5];
        }

        if(cameraMode == ZBarReaderControllerCameraModeSampling) {
            t_frame = timer_now();
            dt_frame = 0;
            sampling = YES;
            boxLayer.opacity = 0;
            if(boxView.superview != activeOverlay)
                [boxView removeFromSuperview];
            if(!boxView.superview)
                [activeOverlay addSubview: boxView];

            [scanner setSymbology: 0
                     config: ZBAR_CFG_X_DENSITY
                     to: 2];
            [scanner setSymbology: 0
                     config: ZBAR_CFG_Y_DENSITY
                     to: 2];
            scanner.enableCache = YES;

            [self performSelector: @selector(scanScreen)
                  withObject: nil
                  afterDelay: 1];
#ifndef NDEBUG
            [self performSelector: @selector(dumpFPS)
                  withObject: nil
                  afterDelay: 4];
#endif
        }
        else {
            sampling = NO;
            [boxView removeFromSuperview];
        }
    }

    [super viewWillAppear: animated];
}

- (void) viewWillDisappear: (BOOL) animated
{
    sampling = NO;
    scanner.enableCache = NO;
    [super viewWillDisappear: animated];
}

- (void) scanScreen
{
    if(!sampling)
        return;

    uint64_t now = timer_now();
    if(dt_frame)
        dt_frame = (dt_frame + timer_elapsed(t_frame, now)) / 2;
    else
        dt_frame = timer_elapsed(t_frame, now);
    t_frame = now;

    // FIXME ugly hack: use private API to sample screen
    CGImageRef image = UIGetScreenImage();

    CGRect crop = CGRectMake(0, 0,
                             CGImageGetWidth(image), CGImageGetHeight(image));
    if(crop.size.width > crop.size.height)
        crop.size.width -= 54;
    else
        crop.size.height -= 54;

    ZBarImage *zimg = [[ZBarImage alloc]
                          initWithCGImage: image
                          crop: crop
                          size: crop.size];
    CGImageRelease(image);

    [scanner scanImage: zimg];
    [zimg release];

    ZBarSymbol *sym = nil;
    ZBarSymbolSet *results = scanner.results;
    results.filterSymbols = NO;
    for(ZBarSymbol *s in results)
        if(!sym || sym.quality < s.quality)
            sym = s;

    if(sym && !sym.count) {
        SEL cb = @selector(imagePickerController:didFinishPickingMediaWithInfo:);
        if(takesPicture) {
            symbol = [sym retain];
            [self takePicture];
        }
        else if([readerDelegate respondsToSelector: cb]) {
            symbol = [sym retain];

            [CATransaction begin];
            [CATransaction setDisableActions: YES];
            boxLayer.opacity = 0;
            [CATransaction commit];

            // capture preview image and send to delegate
            // after box has been hidden
            [self performSelector: @selector(captureScreen)
                  withObject: nil
                  afterDelay: 0.001];
            return;
        }
    }

    // reschedule
    [self performSelector: @selector(scanScreen)
          withObject: nil
          afterDelay: 0.001];

    [CATransaction begin];
    [CATransaction setAnimationDuration: .3];
    [CATransaction setAnimationTimingFunction:
        [CAMediaTimingFunction functionWithName:
            kCAMediaTimingFunctionLinear]];

    CGFloat alpha = boxLayer.opacity;
    if(sym) {
        CGRect r = sym.bounds;
        if(r.size.width > 16 && r.size.height > 16) {
            r = CGRectInset(r, -16, -16);
            if(alpha > .25) {
                CGRect frame = boxLayer.frame;
                r.origin.x = (r.origin.x * 3 + frame.origin.x) / 4;
                r.origin.y = (r.origin.y * 3 + frame.origin.y) / 4;
                r.size.width = (r.size.width * 3 + frame.size.width) / 4;
                r.size.height = (r.size.height * 3 + frame.size.height) / 4;
            }
            boxLayer.frame = r;
            boxLayer.opacity = 1;
        }
    }
    else {
        if(alpha > .1)
            boxLayer.opacity = alpha / 2;
        else if(alpha > 0)
            boxLayer.opacity = 0;
    }
    [CATransaction commit];
}

- (void) captureScreen
{
    CGImageRef screen = UIGetScreenImage();
    CGRect r = CGRectMake(0, 0,
                          CGImageGetWidth(screen), CGImageGetHeight(screen));
    if(r.size.width > r.size.height)
        r.size.width -= 54;
    else
        r.size.height -= 54;
    CGImageRef preview = CGImageCreateWithImageInRect(screen, r);
    CGImageRelease(screen);

    UIImage *image = [UIImage imageWithCGImage: preview];
    CGImageRelease(preview);

    [readerDelegate
        imagePickerController: self
        didFinishPickingMediaWithInfo:
            [NSDictionary dictionaryWithObjectsAndKeys:
                image, UIImagePickerControllerOriginalImage,
                [NSArray arrayWithObject: symbol],
                    ZBarReaderControllerResults,
                nil]];
    [symbol release];
    symbol = nil;

    // continue scanning until dismissed
    [self performSelector: @selector(scanScreen)
          withObject: nil
          afterDelay: 0.001];
}

#ifndef NDEBUG
- (void) dumpFPS
{
    if(!sampling)
        return;
    [self performSelector: @selector(dumpFPS)
          withObject: nil
          afterDelay: 2];
    zlog(@"fps=%g", 1 / dt_frame);
}
#endif

- (void)  imagePickerController: (UIImagePickerController*) picker
  didFinishPickingMediaWithInfo: (NSDictionary*) info
{
    UIImage *img = [info objectForKey: UIImagePickerControllerOriginalImage];

    id results = nil;
    if(sampling) {
        results = [NSArray arrayWithObject: symbol];
        [symbol release];
        symbol = nil;
    }
    else
        results = [self scanImage: img.CGImage];

    [self performSelector: @selector(reenable)
         withObject: nil
         afterDelay: .25];

    if(results) {
        NSMutableDictionary *newinfo = [info mutableCopy];
        [newinfo setObject: results
                 forKey: ZBarReaderControllerResults];
        SEL cb = @selector(imagePickerController:didFinishPickingMediaWithInfo:);
        if([readerDelegate respondsToSelector: cb])
            [readerDelegate imagePickerController: self
                            didFinishPickingMediaWithInfo: newinfo];
        else
            [self dismissModalViewControllerAnimated: YES];
        [newinfo release];
        return;
    }

    BOOL camera = (self.sourceType == UIImagePickerControllerSourceTypeCamera);
    BOOL retry = !camera || (hasOverlay && ![self showsCameraControls]);
    if(showsHelpOnFail && retry) {
        help = [[ZBarHelpController alloc]
                   initWithTitle: @"No Barcode Found"];
        help.delegate = self;
        if(camera)
            [self showHelpOverlay];
        else
            [self presentModalViewController: help
                  animated: YES];
    }

    SEL cb = @selector(readerControllerDidFailToRead:withRetry:);
    if([readerDelegate respondsToSelector: cb])
        // assume delegate dismisses controller if necessary
        [readerDelegate readerControllerDidFailToRead: self
                        withRetry: retry];
    else if(!retry)
        // must dismiss stock controller
        [self dismissModalViewControllerAnimated: YES];
}

- (void) imagePickerControllerDidCancel: (UIImagePickerController*) picker
{
    SEL cb = @selector(imagePickerControllerDidCancel:);
    if([readerDelegate respondsToSelector: cb])
        [readerDelegate imagePickerControllerDidCancel: self];
    else
        [self dismissModalViewControllerAnimated: YES];
}

- (void) helpController: (ZBarHelpController*) hlp
   clickedButtonAtIndex: (NSInteger) idx
{
    if(self.sourceType == UIImagePickerControllerSourceTypeCamera) {
        [UIView beginAnimations: @"ZBarHelp"
                context: nil];
        hlp.view.alpha = 0;
        [UIView commitAnimations];
    }
    else
        [hlp dismissModalViewControllerAnimated: YES];
}

- (BOOL) showsZBarControls
{
    return(showsZBarControls);
}

- (void) setShowsZBarControls: (BOOL) show
{
    if(show && !hasOverlay)
        [NSException raise: NSInvalidArgumentException
            format: @"ZBarReaderController cannot set showsZBarControls=YES for OS<3.1"];

    showsZBarControls = show;
}

// intercept delegate as readerDelegate

- (void) setDelegate: (id <UINavigationControllerDelegate,
                           UIImagePickerControllerDelegate>) delegate
{
    self.readerDelegate = (id <ZBarReaderDelegate>)delegate;
}

- (id <NSFastEnumeration>) scanImage: (CGImageRef) image
                                size: (CGSize) size
{
    timer_start;

    // limit the maximum number of scan passes
    int density;
    if(size.width > 480)
        density = (size.width / 240 + 1) / 2;
    else
        density = 1;
    [scanner setSymbology: 0
             config: ZBAR_CFG_X_DENSITY
             to: density];

    if(size.height > 480)
        density = (size.height / 240 + 1) / 2;
    else
        density = 1;
    [scanner setSymbology: 0
             config: ZBAR_CFG_Y_DENSITY
             to: density];

    ZBarImage *zimg = [[ZBarImage alloc]
                          initWithCGImage: image
                          size: size];
    int nsyms = [scanner scanImage: zimg];
    [zimg release];

    NSMutableArray *syms = nil;
    if(nsyms) {
        // quality/type filtering
        int max_quality = MIN_QUALITY;
        for(ZBarSymbol *sym in scanner.results) {
            zbar_symbol_type_t type = sym.type;
            int quality;
            if(type == ZBAR_QRCODE)
                quality = INT_MAX;
            else
                quality = sym.quality;

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

            [syms addObject: sym];
        }
    }

    zlog(@"read %d filtered symbols in %gs total\n",
          (!syms) ? 0 : [syms count], timer_elapsed(t_start, timer_now()));
    return(syms);
}

- (id <NSFastEnumeration>) scanImage: (CGImageRef) image
{
    CGSize size = CGSizeMake(CGImageGetWidth(image),
                             CGImageGetHeight(image));
    if(size.width > 1280 || size.height > 1280) {
        size.width /= 2;
        size.height /= 2;
    }

    id <NSFastEnumeration> syms =
        [self scanImage: image
              size: size];

    if(!syms) {
        // make one more attempt for close up, grainy images
        size.width /= 2;
        size.height /= 2;
        syms = [self scanImage: image
                     size: size];
    }

    return(syms);
}

@end
