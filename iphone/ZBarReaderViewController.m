//------------------------------------------------------------------------
//  Copyright 2010 (c) Jeff Brown <spadix@users.sourceforge.net>
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

#import <ZBarSDK/ZBarReaderViewController.h>
#import <ZBarSDK/ZBarReaderView.h>
#import <ZBarSDK/ZBarCaptureReader.h>
#import <ZBarSDK/ZBarHelpController.h>
#import <ZBarSDK/ZBarCameraSimulator.h>

#define MODULE ZBarReaderViewController
#import "debug.h"

@implementation ZBarReaderViewController

@synthesize scanner, readerDelegate, showsZBarControls,
    supportedOrientationsMask, tracksSymbols, enableCache, cameraOverlayView,
    cameraViewTransform, readerView, scanCrop;
@dynamic sourceType, allowsEditing, allowsImageEditing, showsCameraControls,
    showsHelpOnFail, cameraMode, takesPicture, maxScanDimension;

+ (BOOL) isSourceTypeAvailable: (UIImagePickerControllerSourceType) sourceType
{
    if(sourceType != UIImagePickerControllerSourceTypeCamera)
        return(NO);
    return(TARGET_IPHONE_SIMULATOR ||
           [UIImagePickerController isSourceTypeAvailable: sourceType]);
}

- (void) _init
{
    supportedOrientationsMask =
        ZBarOrientationMask(UIInterfaceOrientationPortrait);
    showsZBarControls = tracksSymbols = enableCache = YES;
    scanCrop = CGRectMake(0, 0, 1, 1);
    cameraViewTransform = CGAffineTransformIdentity;

    // create our own scanner to store configuration,
    // independent of whether view is loaded
    scanner = [ZBarImageScanner new];
    [scanner setSymbology: 0
             config: ZBAR_CFG_X_DENSITY
             to: 3];
    [scanner setSymbology: 0
             config: ZBAR_CFG_Y_DENSITY
             to: 3];
}

- (id) init
{
    if(!TARGET_IPHONE_SIMULATOR &&
       !NSClassFromString(@"AVCaptureSession")) {
        // fallback to old interface
        zlog(@"Falling back to ZBarReaderController");
        [self release];
        return([ZBarReaderController new]);
    }

    self = [super init];
    if(!self)
        return(nil);

    self.wantsFullScreenLayout = YES;
    [self _init];
    return(self);
}

- (id) initWithCoder: (NSCoder*) decoder
{
    self = [super initWithCoder: decoder];
    if(!self)
        return(nil);

    [self _init];
    return(self);
}

- (void) cleanup
{
    cameraSim.readerView = nil;
    [cameraSim release];
    cameraSim = nil;
    readerView.readerDelegate = nil;
    [readerView release];
    readerView = nil;
    [controls release];
    controls = nil;
}

- (void) dealloc
{
    [self cleanup];
    [cameraOverlayView release];
    cameraOverlayView = nil;
    [scanner release];
    scanner = nil;
    [super dealloc];
}

- (void) initControls
{
    if(!showsZBarControls && controls) {
        [controls removeFromSuperview];
        [controls release];
        controls = nil;
    }
    if(!showsZBarControls)
        return;

    UIView *view = self.view;
    if(controls) {
        assert(controls.superview == view);
        [view bringSubviewToFront: controls];
        return;
    }

    CGRect r = view.bounds;
    r.origin.y = r.size.height - 54;
    r.size.height = 54;
    controls = [[UIView alloc]
                   initWithFrame: r];
    controls.autoresizingMask =
        UIViewAutoresizingFlexibleWidth |
        UIViewAutoresizingFlexibleHeight |
        UIViewAutoresizingFlexibleTopMargin;
    controls.backgroundColor = [UIColor blackColor];

    UIToolbar *toolbar =
        [UIToolbar new];
    r.origin.y = 0;
    toolbar.frame = r;
    toolbar.barStyle = UIBarStyleBlackOpaque;
    toolbar.autoresizingMask =
        UIViewAutoresizingFlexibleWidth |
        UIViewAutoresizingFlexibleHeight;

    UIButton *info =
        [UIButton buttonWithType: UIButtonTypeInfoLight];
    [info addTarget: self
          action: @selector(info)
          forControlEvents: UIControlEventTouchUpInside];

    toolbar.items =
        [NSArray arrayWithObjects:
            [[[UIBarButtonItem alloc]
                 initWithBarButtonSystemItem: UIBarButtonSystemItemCancel
                 target: self
                 action: @selector(cancel)]
                autorelease],
            [[[UIBarButtonItem alloc]
                 initWithBarButtonSystemItem: UIBarButtonSystemItemFlexibleSpace
                 target: nil
                 action: nil]
                autorelease],
            [[[UIBarButtonItem alloc]
                 initWithCustomView: info]
                autorelease],
            nil];
    [controls addSubview: toolbar];
    [toolbar release];

    [view addSubview: controls];
}

- (void) loadView
{
    self.view = [[UIView alloc]
                    initWithFrame: CGRectMake(0, 0, 320, 480)];
}

- (void) viewDidLoad
{
    [super viewDidLoad];
    UIView *view = self.view;
    view.backgroundColor = [UIColor blackColor];
    view.autoresizingMask =
        UIViewAutoresizingFlexibleWidth |
        UIViewAutoresizingFlexibleHeight;

    readerView = [[ZBarReaderView alloc]
                     initWithImageScanner: scanner];
    CGRect bounds = view.bounds;
    CGRect r = bounds;
    NSUInteger autoresize =
        UIViewAutoresizingFlexibleWidth |
        UIViewAutoresizingFlexibleHeight;

    if(showsZBarControls ||
       self.parentViewController.modalViewController == self)
    {
        autoresize |= UIViewAutoresizingFlexibleBottomMargin;
        r.size.height -= 54;
    }
    readerView.frame = r;
    readerView.autoresizingMask = autoresize;

    readerView.readerDelegate = (id<ZBarReaderViewDelegate>)self;
    readerView.scanCrop = scanCrop;
    readerView.previewTransform = cameraViewTransform;
    readerView.tracksSymbols = tracksSymbols;
    readerView.enableCache = enableCache;
    [view addSubview: readerView];

    if(cameraOverlayView) {
        assert(!cameraOverlayView.superview);
        [cameraOverlayView removeFromSuperview];
        cameraOverlayView.frame = readerView.frame;
        [view addSubview: cameraOverlayView];
    }

    [self initControls];

    if(TARGET_IPHONE_SIMULATOR) {
        cameraSim = [[ZBarCameraSimulator alloc]
                        initWithViewController: self];
        cameraSim.readerView = readerView;
    }
}

- (void) viewDidUnload
{
    [cameraOverlayView removeFromSuperview];
    [self cleanup];
    [super viewDidUnload];
}

- (void) viewWillAppear: (BOOL) animated
{
    zlog(@"willAppear: anim=%d", animated);
    [self initControls];
    [super viewWillAppear: animated];

    [readerView start];

    UIApplication *app = [UIApplication sharedApplication];
    BOOL willHideStatusBar =
        !didHideStatusBar && self.wantsFullScreenLayout && !app.statusBarHidden;
    if(willHideStatusBar)
        [app setStatusBarHidden: YES
             withAnimation: UIStatusBarAnimationFade];
    didHideStatusBar = didHideStatusBar || willHideStatusBar;
}

- (void) dismissModalViewControllerAnimated: (BOOL) animated
{
    if(didHideStatusBar) {
        [[UIApplication sharedApplication]
            setStatusBarHidden: NO
            withAnimation: UIStatusBarAnimationFade];
        didHideStatusBar = NO;
    }
    [super dismissModalViewControllerAnimated: animated];
}

- (void) viewWillDisappear: (BOOL) animated
{
    [readerView stop];

    if(didHideStatusBar) {
        [[UIApplication sharedApplication]
            setStatusBarHidden: NO
            withAnimation: UIStatusBarAnimationFade];
        didHideStatusBar = NO;
    }

    [super viewWillDisappear: animated];
}

- (BOOL) shouldAutorotateToInterfaceOrientation: (UIInterfaceOrientation) orient
{
    return((supportedOrientationsMask >> orient) & 1);
}

- (void) willRotateToInterfaceOrientation: (UIInterfaceOrientation) orient
                                 duration: (NSTimeInterval) duration
{
    zlog(@"willRotate: orient=%d #%g", orient, duration);
    if(readerView)
        [readerView willRotateToInterfaceOrientation: orient
                    duration: duration];
}

- (void) willAnimateRotationToInterfaceOrientation: (UIInterfaceOrientation) orient
                                          duration: (NSTimeInterval) duration
{
    zlog(@"willAnimateRotation: orient=%d #%g", orient, duration);
    if(helpController)
        [helpController willAnimateRotationToInterfaceOrientation: orient
                        duration: duration];
    if(readerView)
        [readerView setNeedsLayout];
}

- (void) didRotateFromInterfaceOrientation: (UIInterfaceOrientation) orient
{
    zlog(@"didRotate: orient=%d", orient);
}

- (ZBarReaderView*) readerView
{
    // force view to load
    (void)self.view;
    assert(readerView);
    return(readerView);
}

- (void) setTracksSymbols: (BOOL) track
{
    tracksSymbols = track;
    if(readerView)
        readerView.tracksSymbols = track;
}

- (void) setEnableCache: (BOOL) enable
{
    enableCache = enable;
    if(readerView)
        readerView.enableCache = enable;
}

- (void) setScanCrop: (CGRect) r
{
    scanCrop = r;
    if(readerView)
        readerView.scanCrop = r;
}

- (void) setCameraOverlayView: (UIView*) newview
{
    UIView *oldview = cameraOverlayView;
    [oldview removeFromSuperview];

    cameraOverlayView = [newview retain];
    if([self isViewLoaded] && newview)
        [self.view addSubview: newview];

    [oldview release];
}

- (void) setCameraViewTransform: (CGAffineTransform) xfrm
{
    cameraViewTransform = xfrm;
    if(readerView)
        readerView.previewTransform = xfrm;
}

- (void) cancel
{
    if(!readerDelegate)
        return;
    SEL cb = @selector(imagePickerControllerDidCancel:);
    if([readerDelegate respondsToSelector: cb])
        [readerDelegate
            imagePickerControllerDidCancel: (UIImagePickerController*)self];
    else
        [self dismissModalViewControllerAnimated: YES];
}

- (void) info
{
    [self showHelpWithReason: @"INFO"];
}

- (void) showHelpWithReason: (NSString*) reason
{
    if(helpController)
        return;
    helpController = [[ZBarHelpController alloc]
                         initWithReason: reason];
    helpController.delegate = (id<ZBarHelpDelegate>)self;
    helpController.wantsFullScreenLayout = YES;
    UIView *helpView = helpController.view;
    helpView.alpha = 0;
    helpView.frame = self.view.bounds;
    [helpController viewWillAppear: YES];
    [self.view addSubview: helpView];
    [UIView beginAnimations: @"ZBarHelp"
            context: nil];
    helpController.view.alpha = 1;
    [UIView commitAnimations];
}

- (void) takePicture
{
    if(TARGET_IPHONE_SIMULATOR) {
        [cameraSim takePicture];
        // FIXME return selected image
    }
    else if(readerView)
        [readerView.captureReader captureFrame];
}

// ZBarHelpDelegate

- (void) helpControllerDidFinish: (ZBarHelpController*) help
{
    assert(help == helpController);
    [help viewWillDisappear: YES];
    [UIView beginAnimations: @"ZBarHelp"
            context: NULL];
    [UIView setAnimationDelegate: self];
    [UIView setAnimationDidStopSelector: @selector(removeHelp:done:context:)];
    help.view.alpha = 0;
    [UIView commitAnimations];
}

- (void) removeHelp: (NSString*) tag
               done: (NSNumber*) done
            context: (void*) ctx
{
    if([tag isEqualToString: @"ZBarHelp"] && helpController) {
        [helpController.view removeFromSuperview];
        [helpController release];
        helpController = nil;
    }
}

// ZBarReaderViewDelegate

- (void) readerView: (ZBarReaderView*) view
     didReadSymbols: (ZBarSymbolSet*) syms
          fromImage: (UIImage*) image
{
    [readerDelegate
        imagePickerController: (UIImagePickerController*)self
        didFinishPickingMediaWithInfo:
            [NSDictionary dictionaryWithObjectsAndKeys:
                image, UIImagePickerControllerOriginalImage,
                syms, ZBarReaderControllerResults,
                nil]];
}

// "deprecated" properties

#define DEPRECATED_PROPERTY(getter, setter, type, val, ignore) \
    - (type) getter                                    \
    {                                                  \
        return(val);                                   \
    }                                                  \
    - (void) setter: (type) v                          \
    {                                                  \
        NSAssert2(ignore || v == val,                  \
                  @"attempt to set unsupported value (%d)" \
                  @" for %@ property", val, @#getter); \
    }

DEPRECATED_PROPERTY(sourceType, setSourceType, UIImagePickerControllerSourceType, UIImagePickerControllerSourceTypeCamera, NO)
DEPRECATED_PROPERTY(allowsEditing, setAllowsEditing, BOOL, NO, NO)
DEPRECATED_PROPERTY(allowsImageEditing, setAllowsImageEditing, BOOL, NO, NO)
DEPRECATED_PROPERTY(showsCameraControls, setShowsCameraControls, BOOL, NO, NO)
DEPRECATED_PROPERTY(showsHelpOnFail, setShowsHelpOnFail, BOOL, NO, YES)
DEPRECATED_PROPERTY(cameraMode, setCameraMode, ZBarReaderControllerCameraMode, ZBarReaderControllerCameraModeSampling, NO)
DEPRECATED_PROPERTY(takesPicture, setTakesPicture, BOOL, NO, NO)
DEPRECATED_PROPERTY(maxScanDimension, setMaxScanDimension, NSInteger, 640, YES)

@end
