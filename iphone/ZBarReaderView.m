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

#import <zbar/ZBarReaderView.h>
#import <zbar/ZBarCaptureReader.h>

#define MODULE ZBarReaderView
#import "debug.h"

@implementation ZBarReaderView

@synthesize readerDelegate, tracksSymbols, showsFPS, zoom, scanCrop,
    previewTransform, session, captureReader;
@dynamic scanner, allowsPinchZoom, enableCache, device;

- (id) initWithImageScanner: (ZBarImageScanner*) _scanner
{
    self = [super initWithFrame: CGRectMake(0, 0, 320, 426)];
    if(!self)
        return(nil);

    self.backgroundColor = [UIColor blackColor];
    self.contentMode = UIViewContentModeScaleAspectFill;
    self.autoresizingMask =
        UIViewAutoresizingFlexibleWidth |
        UIViewAutoresizingFlexibleHeight;

    tracksSymbols = YES;
    scanCrop = CGRectMake(0, 0, 1, 1);
    previewTransform = CGAffineTransformIdentity;

    session = [AVCaptureSession new];
    NSNotificationCenter *notify =
        [NSNotificationCenter defaultCenter];
    [notify addObserver: self
            selector: @selector(onVideoError:)
            name: AVCaptureSessionRuntimeErrorNotification
            object: session];
    [notify addObserver: self
            selector: @selector(onVideoStart:)
            name: AVCaptureSessionDidStartRunningNotification
            object: session];
    [notify addObserver: self
            selector: @selector(onVideoStop:)
            name: AVCaptureSessionDidStopRunningNotification
            object: session];
    [notify addObserver: self
            selector: @selector(onVideoStop:)
            name: AVCaptureSessionWasInterruptedNotification
            object: session];
    [notify addObserver: self
            selector: @selector(onVideoStart:)
            name: AVCaptureSessionInterruptionEndedNotification
            object: session];

    self.device = [AVCaptureDevice
                      defaultDeviceWithMediaType: AVMediaTypeVideo];

    captureReader = [[ZBarCaptureReader alloc]
                        initWithImageScanner: _scanner];
    captureReader.captureDelegate = (id<ZBarCaptureDelegate>)self;
    [session addOutput: captureReader.captureOutput];

    preview = [[AVCaptureVideoPreviewLayer
                   layerWithSession: session]
                  retain];
    preview.frame = self.bounds;
    preview.videoGravity = AVLayerVideoGravityResizeAspectFill;
    [self.layer addSublayer: preview];

    CGRect r = preview.bounds;
    tracking = [CALayer new];
    tracking.frame = r;
    tracking.opacity = 0;
    tracking.borderWidth = 1;
    tracking.backgroundColor = [UIColor clearColor].CGColor;
    tracking.borderColor = [UIColor greenColor].CGColor;
    [preview addSublayer: tracking];

    r.origin.x = 3 * r.size.width / 4;
    r.origin.y = r.size.height - 32;
    r.size.width = r.size.width - r.origin.x + 12;
    r.size.height = 32 + 12;
    fpsView = [[UIView alloc]
                   initWithFrame: r];
    fpsView.backgroundColor = [UIColor colorWithWhite: 0
                                       alpha: .333];
    fpsView.layer.cornerRadius = 12;
    fpsView.hidden = YES;
    [self addSubview: fpsView];

    fpsLabel = [[UILabel alloc]
                   initWithFrame: CGRectMake(0, 0,
                                             r.size.width - 12,
                                             r.size.height - 12)];
    fpsLabel.backgroundColor = [UIColor clearColor];
    fpsLabel.textColor = [UIColor colorWithRed: .333
                                  green: .666
                                  blue: 1
                                  alpha: 1];
    fpsLabel.font = [UIFont systemFontOfSize: 18];
    fpsLabel.textAlignment = UITextAlignmentRight;
    [fpsView addSubview: fpsLabel];

    pinch = [[UIPinchGestureRecognizer alloc]
                initWithTarget: self
                action: @selector(handlePinch)];
    [self addGestureRecognizer: pinch];

    self.zoom = 1.25;

    return(self);
}

- (id) init
{
    ZBarImageScanner *scanner =
        [[ZBarImageScanner new]
            autorelease];
    self = [self initWithImageScanner: scanner];
    if(!self)
        return(nil);

    [scanner setSymbology: 0
             config: ZBAR_CFG_X_DENSITY
             to: 3];
    [scanner setSymbology: 0
             config: ZBAR_CFG_Y_DENSITY
             to: 3];
    return(self);
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter]
        removeObserver: self];
    if(showsFPS) {
        @try {
            [captureReader removeObserver: self
                           forKeyPath: @"framesPerSecond"];
        }
        @catch(...) { }
    }
    [fpsLabel release];
    fpsLabel = nil;
    [fpsView release];
    fpsView = nil;
    captureReader.captureDelegate = nil;
    [captureReader release];
    captureReader = nil;
    [device release];
    device = nil;
    [input release];
    input = nil;
    [preview removeFromSuperlayer];
    [preview release];
    preview = nil;
    [session release];
    session = nil;
    [tracking release];
    tracking = nil;
    [pinch release];
    pinch = nil;
    [super dealloc];
}

- (ZBarImageScanner*) scanner
{
    return(captureReader.scanner);
}

- (void) resetTracking
{
    tracking.opacity = 0;
    CGSize size = preview.bounds.size;
    CGRect crop = captureReader.scanCrop;
    tracking.frame = CGRectMake(crop.origin.y * size.width,
                                crop.origin.x * size.height,
                                crop.size.height * size.width,
                                crop.size.width * size.height);
}

- (void) cropUpdate
{
    CGAffineTransform xfrm =
        CGAffineTransformMakeTranslation(.5, .5);
    CGFloat z = 1 / zoom;
    xfrm = CGAffineTransformScale(xfrm, z, z);
    xfrm = CGAffineTransformTranslate(xfrm, -.5, -.5);
    captureReader.scanCrop = CGRectApplyAffineTransform(scanCrop, xfrm);
    [self resetTracking];
}

- (void) setScanCrop: (CGRect) r
{
    if(CGRectEqualToRect(scanCrop, r))
        return;
    scanCrop = r;
    [self cropUpdate];
}

- (AVCaptureDevice*) device
{
    return(device);
}

- (void) setDevice: (AVCaptureDevice*) newdev
{
    id olddev = device;
    AVCaptureInput *oldinput = input;
    assert(!olddev == !oldinput);

    NSError *error = nil;
    device = [newdev retain];
    if(device) {
        assert([device hasMediaType: AVMediaTypeVideo]);
        input = [[AVCaptureDeviceInput alloc]
                    initWithDevice: newdev
                    error: &error];
        assert(input);
    }
    else
        input = nil;

    [session beginConfiguration];
    if(oldinput)
        [session removeInput: input];
    if(input)
        [session addInput: input];
    [session commitConfiguration];

    [olddev release];
    [oldinput release];
}

- (void) setTracksSymbols: (BOOL) track
{
    if(track == tracksSymbols)
        return;
    tracksSymbols = track;
    [self resetTracking];
}

- (BOOL) allowsPinchZoom
{
    return(pinch.enabled);
}

- (BOOL) enableCache
{
    return(captureReader.enableCache);
}

- (void) setEnableCache: (BOOL) enable
{
    captureReader.enableCache = enable;
}

- (void) setAllowsPinchZoom: (BOOL) enabled
{
    pinch.enabled = enabled;
}

- (void) setShowsFPS: (BOOL) show
{
    if(show == showsFPS)
        return;
    fpsView.hidden = !show;
    @try {
        if(show)
            [captureReader addObserver: self
                           forKeyPath: @"framesPerSecond"
                           options: 0
                           context: NULL];
        else
            [captureReader removeObserver: self
                           forKeyPath: @"framesPerSecond"];
    }
    @catch(...) { }
}

- (void) setZoom: (CGFloat) z
{
    if(z < 1.0)
        z = 1.0;
    if(z > 2.0)
        z = 2.0;
    if(z == zoom)
        return;
    zoom = z;

    [CATransaction begin];
    [CATransaction setAnimationDuration: .1];
    [CATransaction setAnimationTimingFunction:
        [CAMediaTimingFunction functionWithName:
            kCAMediaTimingFunctionLinear]];
    [preview setAffineTransform:
        CGAffineTransformScale(previewTransform, z, z)];
    [CATransaction commit];

    [self cropUpdate];
}

- (void) setPreviewTransform: (CGAffineTransform) xfrm
{
    previewTransform = xfrm;
    [preview setAffineTransform:
        CGAffineTransformScale(previewTransform, zoom, zoom)];
}

- (void) start
{
    if(started)
        return;
    started = YES;

    [[UIDevice currentDevice]
        beginGeneratingDeviceOrientationNotifications];

    [self resetTracking];
    fpsLabel.text = @"";

    [captureReader willStartRunning];
    [session startRunning];
}

- (void) stop
{
    if(!started)
        return;

    [captureReader willStopRunning];
    [session stopRunning];

    [[UIDevice currentDevice]
        endGeneratingDeviceOrientationNotifications];
    started = NO;
}

- (void) flushCache
{
    [captureReader flushCache];
}

// AVCaptureSession notifications

- (void) onVideoStart: (NSNotification*) note
{
    zlog(@"onVideoStart: running=%d %@", running, note);
    if(running)
        return;
    running = YES;

    // lock device and set focus mode
    NSError *error = nil;
    if([device lockForConfiguration: &error])
        device.focusMode = AVCaptureFocusModeContinuousAutoFocus;
    else
        zlog(@"failed to lock device: %@", error);
}

- (void) onVideoStop: (NSNotification*) note
{
    zlog(@"onVideoStop: %@", note);
    if(!running)
        return;

    [device unlockForConfiguration];
    running = NO;
}

- (void) onVideoError: (NSNotification*) note
{
    zlog(@"onVideoError: %@", note);
    if(running) {
        // FIXME does session always stop on error?
        running = started = NO;
        [device unlockForConfiguration];
    }
    NSError *err =
        [note.userInfo objectForKey: AVCaptureSessionErrorKey];
    NSLog(@"ZBarReaderView: ERROR during capture: %@: %@",
          [err localizedDescription],
          [err localizedFailureReason]);
}

// UIGestureRecognizer callback

- (void) handlePinch
{
    if(pinch.state == UIGestureRecognizerStateBegan)
        zoom0 = zoom;
    CGFloat z = zoom0 * pinch.scale;
    self.zoom = z;

    if((zoom < 1.5) != (z < 1.5)) {
        int d = (z < 1.5) ? 3 : 2;
        ZBarImageScanner *scanner = captureReader.scanner;
        @synchronized(scanner) {
            [scanner setSymbology: 0
                     config: ZBAR_CFG_X_DENSITY
                     to: d];
            [scanner setSymbology: 0
                     config: ZBAR_CFG_Y_DENSITY
                     to: d];
        }
    }
}

// NSKeyValueObserving

- (void) observeValueForKeyPath: (NSString*) path
                       ofObject: (id) obj
                         change: (NSDictionary*) info
                        context: (void*) ctx
{
    if(obj == captureReader &&
       [path isEqualToString: @"framesPerSecond"])
        fpsLabel.text = [NSString stringWithFormat: @"%.2ffps ",
                                  captureReader.framesPerSecond];
}

// ZBarCaptureDelegate

- (void) updateTracking: (CALayer*) trk
             withSymbol: (ZBarSymbol*) sym
{
    if(!sym)
        return;

    CGRect b = sym.bounds;
    CGPoint c = CGPointMake(CGRectGetMidX(b), CGRectGetMidY(b));
    CGSize isize = captureReader.size;
    CGSize psize = preview.bounds.size;
    CGPoint p = CGPointMake((isize.height - c.y) * psize.width / isize.height,
                            c.x * psize.height / isize.width);
    CGRect r = CGRectMake(0, 0,
                          b.size.height * psize.width / isize.height,
                          b.size.width * psize.height / isize.width);
    if(r.size.width <= 24 && r.size.height <= 24)
        return;
    r = CGRectInset(r, -16, -16);

    CALayer *current = trk.presentationLayer;
    CGPoint cp = current.position;
    p = CGPointMake((p.x * 3 + cp.x) / 4,
                    (p.y * 3 + cp.y) / 4);

    CGRect cr = current.bounds;
    r.origin = cr.origin;
    r.size.width = (r.size.width * 3 + cr.size.width) / 4;
    r.size.height = (r.size.height * 3 + cr.size.height) / 4;

    CAMediaTimingFunction *linear =
        [CAMediaTimingFunction functionWithName: kCAMediaTimingFunctionLinear];

    CABasicAnimation *resize =
        [CABasicAnimation animationWithKeyPath: @"bounds"];
    resize.fromValue = [NSValue valueWithCGRect: cr];
    resize.toValue = [NSValue valueWithCGRect: r];
    resize.duration = .2;
    resize.timingFunction = linear;
    resize.fillMode = kCAFillModeForwards;
    resize.removedOnCompletion = NO;

    CABasicAnimation *move =
        [CABasicAnimation animationWithKeyPath: @"position"];
    move.fromValue = [NSValue valueWithCGPoint: cp];
    move.toValue = [NSValue valueWithCGPoint: p];
    move.duration = .2;
    move.timingFunction = linear;
    move.fillMode = kCAFillModeForwards;
    move.removedOnCompletion = NO;

    CABasicAnimation *on =
        [CABasicAnimation animationWithKeyPath: @"opacity"];
    on.fromValue = [NSNumber numberWithDouble: current.opacity];
    on.toValue = [NSNumber numberWithDouble: 1];
    on.duration = .2;
    on.timingFunction = linear;
    on.fillMode = kCAFillModeForwards;
    on.removedOnCompletion = NO;

    CABasicAnimation *off =
        [CABasicAnimation animationWithKeyPath: @"opacity"];
    off.fromValue = [NSNumber numberWithDouble: 1];
    off.toValue = [NSNumber numberWithDouble: 0];
    off.beginTime = .5;
    off.duration = .5;
    off.timingFunction = linear;

    CAAnimationGroup *group = [CAAnimationGroup animation];
    group.animations = [NSArray arrayWithObjects: resize, move, on, off, nil];
    group.duration = 1;
    [trk addAnimation: group
         forKey: @"tracking"];
}

- (void) captureReader: (ZBarCaptureReader*) reader
       didTrackSymbols: (ZBarSymbolSet*) syms
{
    if(!tracksSymbols)
        return;

    int n = syms.count;
    assert(n);
    if(!n)
        return;

    ZBarSymbol *sym = nil;
    for(ZBarSymbol *s in syms)
        if(!sym || s.type == ZBAR_QRCODE || s.quality > sym.quality)
            sym = s;
    assert(sym);
    if(!sym)
        return;

    [self updateTracking: tracking
          withSymbol: sym];
}

- (void)       captureReader: (ZBarCaptureReader*) reader
  didReadNewSymbolsFromImage: (ZBarImage*) zimg
{
    zlog(@"scanned %d symbols: %@", zimg.symbols.count, zimg);
    if(!readerDelegate)
        return;

    UIImageOrientation orient;
    switch([UIDevice currentDevice].orientation)
    {
    case UIDeviceOrientationPortraitUpsideDown:
        orient = UIImageOrientationLeft;
        break;
    case UIDeviceOrientationLandscapeLeft:
        orient = UIImageOrientationUp;
        break;
    case UIDeviceOrientationLandscapeRight:
        orient = UIImageOrientationDown;
        break;
    default:
        orient = UIImageOrientationRight;
        break;
    }

    UIImage *uiimg = [zimg UIImageWithOrientation: orient];
    [readerDelegate
        readerView: self
        didReadSymbols: zimg.symbols
        fromImage: uiimg];
}

@end
