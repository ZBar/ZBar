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

#import <ZBarSDK/ZBarReaderView.h>

#define MODULE ZBarReaderView
#import "debug.h"

// silence warning
@interface ZBarReaderViewImpl : NSObject
@end

@implementation ZBarReaderView

@synthesize readerDelegate, tracksSymbols, torchMode, showsFPS, zoom, scanCrop,
    previewTransform;
@dynamic scanner, allowsPinchZoom, enableCache, device, session, captureReader;

+ (id) alloc
{
    if(self == [ZBarReaderView class]) {
        // this is an abstract wrapper for implementation selected
        // at compile time.  replace with concrete subclass.
        return([ZBarReaderViewImpl alloc]);
    }
    return([super alloc]);
}

- (void) initSubviews
{
    assert(preview);

    CGRect r = preview.bounds;
    overlay = [CALayer new];
    overlay.frame = r;
    overlay.backgroundColor = [UIColor clearColor].CGColor;
    [preview addSublayer: overlay];

    tracking = [CALayer new];
    tracking.frame = r;
    tracking.opacity = 0;
    tracking.borderWidth = 1;
    tracking.backgroundColor = [UIColor clearColor].CGColor;
    tracking.borderColor = [UIColor greenColor].CGColor;
    [overlay addSublayer: tracking];

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

    self.zoom = 1.25;
}

- (id) initWithImageScanner: (ZBarImageScanner*) scanner
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
    torchMode = 2; // AVCaptureTorchModeAuto
    scanCrop = zoomCrop = CGRectMake(0, 0, 1, 1);
    imageScale = 1;
    previewTransform = CGAffineTransformIdentity;

    pinch = [[UIPinchGestureRecognizer alloc]
                initWithTarget: self
                action: @selector(handlePinch)];
    [self addGestureRecognizer: pinch];

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
    [preview removeFromSuperlayer];
    [preview release];
    preview = nil;
    [overlay release];
    overlay = nil;
    [tracking release];
    tracking = nil;
    [fpsLabel release];
    fpsLabel = nil;
    [fpsView release];
    fpsView = nil;
    [pinch release];
    pinch = nil;
    [super dealloc];
}

- (void) resetTracking
{
    [tracking removeAllAnimations];
    [CATransaction begin];
    [CATransaction setDisableActions: YES];
    CGSize size = overlay.bounds.size;
    CGRect crop = zoomCrop;
    tracking.frame = CGRectMake(crop.origin.x * size.width,
                                crop.origin.y * size.height,
                                crop.size.width * size.width,
                                crop.size.height * size.height);
    tracking.opacity = 0;
    [CATransaction commit];
}

- (void) setImageSize: (CGSize) size
{
    CGSize psize = preview.bounds.size;
    CGFloat scalex = size.width / psize.height;
    CGFloat scaley = size.height / psize.width;
    imageScale = (scalex > scaley) ? scalex : scaley;

    // match overlay to preview image
    overlay.bounds = CGRectMake(0, 0, size.width, size.height);
    CGFloat scale = 1 / imageScale;
    CATransform3D xform = CATransform3DMakeRotation(M_PI / 2, 0, 0, 1);
    overlay.transform = CATransform3DScale(xform, scale, scale, 1);
    tracking.borderWidth = imageScale / zoom;

    zlog(@"scaling: layer=%@ image=%@ scale=%g %c %g = 1/%g",
         NSStringFromCGSize(psize), NSStringFromCGSize(size),
         scalex, (scalex > scaley) ? '>' : '<', scaley, scale);
    [self resetTracking];
}

- (void) cropUpdate
{
    CGAffineTransform xfrm =
        CGAffineTransformMakeTranslation(.5, .5);
    CGFloat z = 1 / zoom;
    xfrm = CGAffineTransformScale(xfrm, z, z);
    xfrm = CGAffineTransformTranslate(xfrm, -.5, -.5);
    zoomCrop = CGRectApplyAffineTransform(scanCrop, xfrm);
    [self resetTracking];
}

- (void) setScanCrop: (CGRect) r
{
    if(CGRectEqualToRect(scanCrop, r))
        return;
    scanCrop = r;
    [self cropUpdate];
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

- (void) setAllowsPinchZoom: (BOOL) enabled
{
    pinch.enabled = enabled;
}

- (void) setShowsFPS: (BOOL) show
{
    if(show == showsFPS)
        return;
    fpsView.hidden = !show;
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
    tracking.borderWidth = imageScale / zoom;
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

    [self resetTracking];
    fpsLabel.text = @"--- fps ";

    [[UIDevice currentDevice]
        beginGeneratingDeviceOrientationNotifications];
}

- (void) stop
{
    if(!started)
        return;
    started = NO;

    [[UIDevice currentDevice]
        endGeneratingDeviceOrientationNotifications];
}

- (void) flushCache
{
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
        ZBarImageScanner *scanner = self.scanner;
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

- (void) updateTracking: (CALayer*) trk
             withSymbol: (ZBarSymbol*) sym
{
    if(!sym)
        return;

    CGRect r = sym.bounds;
    if(r.size.width <= 32 && r.size.height <= 32)
        return;
    r = CGRectInset(r, -24, -24);

    CALayer *current = trk.presentationLayer;
    CGPoint cp = current.position;
    CGPoint p = CGPointMake(CGRectGetMidX(r), CGRectGetMidY(r));
    p = CGPointMake((p.x * 3 + cp.x) / 4, (p.y * 3 + cp.y) / 4);

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

    CABasicAnimation *off = nil;
    if(!TARGET_IPHONE_SIMULATOR) {
        off = [CABasicAnimation animationWithKeyPath: @"opacity"];
        off.fromValue = [NSNumber numberWithDouble: 1];
        off.toValue = [NSNumber numberWithDouble: 0];
        off.beginTime = .5;
        off.duration = .5;
        off.timingFunction = linear;
    }

    CAAnimationGroup *group = [CAAnimationGroup animation];
    group.animations = [NSArray arrayWithObjects: resize, move, on, off, nil];
    group.duration = 1;
    group.fillMode = kCAFillModeForwards;
    group.removedOnCompletion = !TARGET_IPHONE_SIMULATOR;
    [trk addAnimation: group
         forKey: @"tracking"];
}

- (void) didTrackSymbols: (ZBarSymbolSet*) syms
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

@end
