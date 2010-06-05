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

#import <UIKit/UIKit.h>
#import <zbar/ZBarImageScanner.h>

@class AVCaptureSession, AVCaptureDevice, AVCaptureInput;
@class AVCaptureVideoPreviewLayer;
@class CALayer;
@class ZBarReaderView, ZBarCaptureReader;

// delegate is notified of decode results.

@protocol ZBarReaderViewDelegate < NSObject >

- (void) readerView: (ZBarReaderView*) readerView
     didReadSymbols: (ZBarSymbolSet*) symbols
          fromImage: (UIImage*) image;

@end

// read barcodes from the displayed video preview.  the view maintains
// a complete video capture session feeding a ZBarCaptureReader and
// presents the associated preview with symbol tracking annotations.

@interface ZBarReaderView
    : UIView
{
    id<ZBarReaderViewDelegate> readerDelegate;
    AVCaptureSession *session;
    AVCaptureDevice *device;
    BOOL tracksSymbols, showsFPS;

    AVCaptureInput *input;
    ZBarCaptureReader *captureReader;
    AVCaptureVideoPreviewLayer *preview;
    CALayer *tracking;
    UIView *fpsView;
    UILabel *fpsLabel;
    BOOL started, running;
}

// supply a pre-configured image scanner.
- (id) initWithImageScanner: (ZBarImageScanner*) imageScanner;

// start the video stream and barcode reader.
- (void) start;

// stop the video stream and barcode reader.
- (void) stop;

// delegate is notified of decode results.
@property (nonatomic, assign) id<ZBarReaderViewDelegate> readerDelegate;

// access to image scanner for configuration.
@property (nonatomic, readonly) ZBarImageScanner *scanner;

// whether to display the tracking annotation for uncertain barcodes
// (default YES).
@property (nonatomic) BOOL tracksSymbols;

// whether to display the frame rate for debug/configuration
// (default NO).
@property (nonatomic) BOOL showsFPS;

// the region of the image that will be scanned.  normalized coordinates.
@property (nonatomic) CGRect scanCrop;

// specify an alternate capture device.
@property (nonatomic, retain) AVCaptureDevice *device;

// direct access to the capture session.  warranty void if opened...
@property (nonatomic, readonly) AVCaptureSession *session;
@property (nonatomic, readonly) ZBarCaptureReader *captureReader;

@end
