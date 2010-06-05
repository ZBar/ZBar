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

#import <libkern/OSAtomic.h>
#import <zbar/ZBarCaptureReader.h>
#import <zbar/ZBarImageScanner.h>
#import "ZBarCVImage.h"

#define MODULE ZBarCaptureReader
#import "debug.h"

@implementation ZBarCaptureReader

@synthesize captureOutput, captureDelegate, scanner, scanCrop, framesPerSecond;

- (void) initResult
{
    [result release];
    result = [ZBarCVImage new];
    result.format = [ZBarImage fourcc: @"CV2P"];
}

- (id) initWithImageScanner: (ZBarImageScanner*) _scanner
{
    self = [super init];
    if(!self)
        return(nil);

    t_fps = t_frame = timer_now();

    scanner = [_scanner retain];
    scanCrop = CGRectMake(0, 0, 1, 1);
    image = [ZBarImage new];
    image.format = [ZBarImage fourcc: @"Y800"];
    [self initResult];

    captureOutput = [AVCaptureVideoDataOutput new];
    captureOutput.alwaysDiscardsLateVideoFrames = YES;
    captureOutput.videoSettings = 
        [NSDictionary
            dictionaryWithObject:
                [NSNumber numberWithInt:
                    kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange]
            forKey: (NSString*)kCVPixelBufferPixelFormatTypeKey];

    queue = dispatch_queue_create("ZBarCaptureReader", NULL);
    [captureOutput setSampleBufferDelegate:
                       (id<AVCaptureVideoDataOutputSampleBufferDelegate>)self
                   queue: queue];

    return(self);
}

- (id) init
{
    self = [self initWithImageScanner:
               [[ZBarImageScanner new]
                   autorelease]];
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
    captureDelegate = nil;

    // queue continues to run after stopping (NB even after DidStopRunning!);
    // ensure released delegate is not called. (also NB that the queue
    // may not be null, even in this case...)
    [captureOutput setSampleBufferDelegate: nil
                   queue: queue];
    [captureOutput release];
    captureOutput = nil;
    dispatch_release(queue);

    [image release];
    image = nil;
    [result release];
    result = nil;
    [scanner release];
    scanner = nil;
    [super dealloc];
}

- (void) willStartRunning
{
    if(!OSAtomicCompareAndSwap32Barrier(0, 1, &running))
        return;
    @synchronized(scanner) {
        scanner.enableCache = YES;
    }
}

- (void) willStopRunning
{
    if(!OSAtomicAnd32OrigBarrier(0, (void*)&running))
        return;
}

- (void) didTrackSymbols: (ZBarSymbolSet*) syms
{
    [captureDelegate
        captureReader: self
        didTrackSymbols: syms];
}

- (void) didReadNewSymbolsFromImage: (ZBarImage*) img
{
    timer_start;
    [captureDelegate
        captureReader: self
        didReadNewSymbolsFromImage: img];
    OSAtomicCompareAndSwap32Barrier(2, 1, &running);
    zlog(@"latency: delegate=%gs total=%gs",
         timer_elapsed(t_start, timer_now()),
         timer_elapsed(t_scan, timer_now()));
}

- (void) setFramesPerSecond: (CGFloat) fps
{
    framesPerSecond = fps;
}

- (void) setFPS: (NSNumber*) val
{
    [self setFramesPerSecond: val.doubleValue];
}

- (void)  captureOutput: (AVCaptureOutput*) output
  didOutputSampleBuffer: (CMSampleBufferRef) samp
         fromConnection: (AVCaptureConnection*) conn
{
    // queue is apparently not flushed when stopping;
    // only process when running
    if(!OSAtomicCompareAndSwap32Barrier(1, 1, &running))
        return;

    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    image.sequence = framecnt++;

    uint64_t now = timer_now();
    double dt = timer_elapsed(t_frame, now);
    t_frame = now;
    if(dt > 2) {
        t_fps = now;
        dt_frame = 0;
    }
    else if(!dt_frame)
        dt_frame = dt;
    dt_frame = (dt_frame + dt) / 2;
    if(timer_elapsed(t_fps, now) >= 1) {
        [self performSelectorOnMainThread: @selector(setFPS:)
              withObject: [NSNumber numberWithDouble: 1 / dt_frame]
              waitUntilDone: NO];
        t_fps = now;
    }

    CVImageBufferRef buf = CMSampleBufferGetImageBuffer(samp);
    if(CMSampleBufferGetNumSamples(samp) != 1 ||
       !CMSampleBufferIsValid(samp) ||
       !CMSampleBufferDataIsReady(samp) ||
       !buf) {
        zlog(@"ERROR: invalid sample");
        goto error;
    }

    OSType format = CVPixelBufferGetPixelFormatType(buf);
    int planes = CVPixelBufferGetPlaneCount(buf);

    if(format != kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange ||
       !planes) {
        zlog(@"ERROR: invalid buffer format");
        goto error;
    }

    int w = CVPixelBufferGetBytesPerRowOfPlane(buf, 0);
    int h = CVPixelBufferGetHeightOfPlane(buf, 0);
    CVReturn rc =
        CVPixelBufferLockBaseAddress(buf, kCVPixelBufferLock_ReadOnly);
    if(!w || !h || rc) {
        zlog(@"ERROR: invalid buffer data");
        goto error;
    }

    void *data = CVPixelBufferGetBaseAddressOfPlane(buf, 0);
    if(data) {
        int yc = h * scanCrop.origin.y + .5;
        if(yc < 0)
            yc = 0;
        if(yc >= h)
            yc = h - 1;
        int hc = h * scanCrop.size.height + .5;
        if(hc <= 0)
            hc = 1;
        if(yc + hc > h)
            hc = h - yc;
        image.size = CGSizeMake(w, hc);
        [image setData: data + w * yc
               withLength: w * hc];

        int ngood = 0;
        ZBarSymbolSet *syms = nil;
        @synchronized(scanner) {
            ngood = [scanner scanImage: image];
            syms = scanner.results;
        }
        now = timer_now();

        if(ngood >= 0) {
            // return unfiltered results for tracking feedback
            syms.filterSymbols = NO;
            int nraw = syms.count;
            if(nraw > 0)
                zlog(@"scan image: %dx%d ngood=%d nraw=%d",
                     w, hc, ngood, nraw);

            SEL cb = @selector(captureReader:didReadNewSymbolsFromImage:);
            if(ngood && [captureDelegate respondsToSelector: cb]) {
                // copy image data so we can release the buffer
                result.size = CGSizeMake(w, h);
                result.pixelBuffer = buf;
                result.symbols = syms;
                t_scan = now;
                OSAtomicCompareAndSwap32Barrier(1, 2, &running);
                [self performSelectorOnMainThread:
                          @selector(didReadNewSymbolsFromImage:)
                      withObject: result
                      waitUntilDone: NO];
                [self initResult];
            }

            cb = @selector(captureReader:didTrackSymbols:);
            if(nraw && [captureDelegate respondsToSelector: cb])
                [self performSelectorOnMainThread:
                          @selector(didTrackSymbols:)
                      withObject: syms
                      waitUntilDone: NO];
        }
        [image setData: NULL
               withLength: 0];
    }
    else
        zlog(@"ERROR: invalid data");
    CVPixelBufferUnlockBaseAddress(buf, kCVPixelBufferLock_ReadOnly);

 error:
    [pool release];
}

@end
