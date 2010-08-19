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

#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <ZBarSDK/ZBarReaderView.h>
#import <ZBarSDK/ZBarCaptureReader.h>

#define MODULE ZBarReaderView
#import "debug.h"

// protected APIs
@interface ZBarReaderView()
- (void) initSubviews;
- (void) cropUpdate;
- (void) setImageSize: (CGSize) size;
- (void) didTrackSymbols: (ZBarSymbolSet*) syms;
@end

@interface ZBarReaderViewImpl
    : ZBarReaderView
{
    AVCaptureSession *session;
    AVCaptureDevice *device;
    AVCaptureInput *input;
    ZBarCaptureReader *captureReader;
}

@end

@implementation ZBarReaderViewImpl

@synthesize device, session, captureReader;

- (id) initWithImageScanner: (ZBarImageScanner*) scanner
{
    self = [super initWithImageScanner: scanner];
    if(!self)
        return(nil);

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
                        initWithImageScanner: scanner];
    captureReader.captureDelegate = (id<ZBarCaptureDelegate>)self;
    [session addOutput: captureReader.captureOutput];

    if([session canSetSessionPreset: AVCaptureSessionPreset640x480])
        session.sessionPreset = AVCaptureSessionPreset640x480;

    [captureReader addObserver: self
                   forKeyPath: @"size"
                   options: 0
                   context: NULL];

    [self initSubviews];
    return(self);
}

- (void) initSubviews
{
    AVCaptureVideoPreviewLayer *videoPreview =
        [[AVCaptureVideoPreviewLayer
             layerWithSession: session]
            retain];
    videoPreview.videoGravity = AVLayerVideoGravityResizeAspectFill;
    preview = videoPreview;
    preview.frame = self.bounds;
    [self.layer addSublayer: preview];

    [super initSubviews];

    // camera image is rotated
    overlay.transform = CATransform3DMakeRotation(M_PI / 2, 0, 0, 1);
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
    @try {
        [captureReader removeObserver: self
                       forKeyPath: @"size"];
    }
    @catch(...) { }
    captureReader.captureDelegate = nil;
    [captureReader release];
    captureReader = nil;
    [device release];
    device = nil;
    [input release];
    input = nil;
    [session release];
    session = nil;
    [super dealloc];
}

- (void) cropUpdate
{
    [super cropUpdate];
    captureReader.scanCrop = zoomCrop;
}

- (ZBarImageScanner*) scanner
{
    return(captureReader.scanner);
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

- (BOOL) enableCache
{
    return(captureReader.enableCache);
}

- (void) setEnableCache: (BOOL) enable
{
    captureReader.enableCache = enable;
}

- (void) setTorchMode: (NSInteger) mode
{
    [super setTorchMode: mode];
    if(running && [device isTorchModeSupported: mode])
        @try {
            device.torchMode = mode;
        }
        @catch(...) { }
}

- (void) setShowsFPS: (BOOL) show
{
    [super setShowsFPS: show];
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
    [super start];

    [captureReader willStartRunning];
    [session startRunning];
}

- (void) stop
{
    if(!started)
        return;
    [super stop];

    [captureReader willStopRunning];
    [session stopRunning];
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
    if([device lockForConfiguration: &error]) {
        if([device isFocusModeSupported: AVCaptureFocusModeContinuousAutoFocus])
            device.focusMode = AVCaptureFocusModeContinuousAutoFocus;
        if([device isTorchModeSupported: torchMode])
            device.torchMode = torchMode;
    }
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

// NSKeyValueObserving

- (void) observeValueForKeyPath: (NSString*) path
                       ofObject: (id) obj
                         change: (NSDictionary*) info
                        context: (void*) ctx
{
    if(obj == captureReader &&
       [path isEqualToString: @"size"]) {
        // adjust tracking overlay to match image size
        CGSize size = captureReader.size;
        overlay.bounds = CGRectMake(0, 0, size.width, size.height);

        CGSize rsize = CGSizeMake(size.height, size.width);
        [self setImageSize: rsize];
        CGFloat scale = 1 / imageScale;
        CATransform3D xform = CATransform3DMakeRotation(M_PI / 2, 0, 0, 1);
        overlay.transform = CATransform3DScale(xform, scale, scale, 1);
    }
    else if(obj == captureReader &&
       [path isEqualToString: @"framesPerSecond"])
        fpsLabel.text = [NSString stringWithFormat: @"%.2ffps ",
                                  captureReader.framesPerSecond];
}

// ZBarCaptureDelegate

- (void) captureReader: (ZBarCaptureReader*) reader
       didTrackSymbols: (ZBarSymbolSet*) syms
{
    [self didTrackSymbols: syms];
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
