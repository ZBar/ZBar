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

@synthesize readerDelegate, showsFPS, tracksSymbols, session, captureReader;
@dynamic scanner, scanCrop, device;

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
    [self.layer addSublayer: preview];

    CGRect r = self.bounds;
    tracking = [CALayer new];
    tracking.frame = r;
    tracking.opacity = 0;
    tracking.borderWidth = 1;
    tracking.backgroundColor = [UIColor clearColor].CGColor;
    tracking.borderColor = [UIColor greenColor].CGColor;
    [self.layer addSublayer: tracking];

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
    [super dealloc];
}

- (ZBarImageScanner*) scanner
{
    return(captureReader.scanner);
}

- (CGRect) scanCrop
{
    return(captureReader.scanCrop);
}

- (void) setScanCrop: (CGRect) r
{
    captureReader.scanCrop = r;
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
    tracking.opacity = 0;
    tracking.frame = self.bounds;
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

- (void) start
{
    if(started)
        return;
    started = YES;

    [[UIDevice currentDevice]
        beginGeneratingDeviceOrientationNotifications];

    tracking.frame = self.bounds;
    tracking.opacity = 0;
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

- (void) updateTracking: (CALayer*) trk
             withSymbol: (ZBarSymbol*) sym
{
    if(!sym)
        return;

    CGRect b = sym.bounds;
    CGPoint c = CGPointMake(CGRectGetMidX(b), CGRectGetMidY(b));
    CGSize size = preview.bounds.size;
    CGPoint p = CGPointMake((480 - c.y) * size.width / 480,
                            c.x * size.height / 640);
    CGRect r = CGRectMake(0, 0,
                          b.size.height * size.width / 480,
                          b.size.width * size.height / 640);
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
    resize.duration = .3;
    resize.timingFunction = linear;
    resize.fillMode = kCAFillModeForwards;
    resize.removedOnCompletion = NO;

    CABasicAnimation *move =
        [CABasicAnimation animationWithKeyPath: @"position"];
    move.fromValue = [NSValue valueWithCGPoint: cp];
    move.toValue = [NSValue valueWithCGPoint: p];
    move.duration = .3;
    move.timingFunction = linear;
    move.fillMode = kCAFillModeForwards;
    move.removedOnCompletion = NO;

    CABasicAnimation *on =
        [CABasicAnimation animationWithKeyPath: @"opacity"];
    on.fromValue = [NSNumber numberWithDouble: current.opacity];
    on.toValue = [NSNumber numberWithDouble: 1];
    on.duration = .3;
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
