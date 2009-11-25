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

#import <zbar.h>
#import <UIKit/UIKit.h>
#import <zbar/ZBarImageScanner.h>

#ifdef __cplusplus
using namespace zbar;
#endif

typedef enum {
    ZBarReaderControllerCameraModeDefault = 0,
    ZBarReaderControllerCameraModeSampling,
} ZBarReaderControllerCameraMode;

@class ZBarReaderController, ZBarHelpController;

@protocol ZBarReaderDelegate <UIImagePickerControllerDelegate>
@optional

// called when no barcode is found in an image selected by the user.
// if retry is NO, the delegate *must* dismiss the controller
- (void) readerControllerDidFailToRead: (ZBarReaderController*) reader
                             withRetry: (BOOL) retry;

@end


@interface ZBarReaderController : UIImagePickerController
                                < UINavigationControllerDelegate,
                                  UIImagePickerControllerDelegate >
{
    ZBarImageScanner *scanner;
    ZBarHelpController *help;
    UIView *overlay, *boxView;
    CALayer *boxLayer;

    UIToolbar *toolbar;
    UIBarButtonItem *cancelBtn, *scanBtn, *space[3];

    id <ZBarReaderDelegate> readerDelegate;
    BOOL showsZBarControls, showsHelpOnFail, takesPicture;
    ZBarReaderControllerCameraMode cameraMode;

    BOOL hasOverlay, sampling;
    uint64_t t_frame;
    double dt_frame;

    ZBarSymbol *symbol;
}

// access to configure image scanner
@property (readonly, nonatomic) ZBarImageScanner *scanner;

// barcode result recipient (NB don't use delegate)
@property (nonatomic, assign) id <ZBarReaderDelegate> readerDelegate;

// whether to use alternate control set
@property (nonatomic) BOOL showsZBarControls;

// whether to display helpful information when decoding fails
@property (nonatomic) BOOL showsHelpOnFail;

// how to use the camera (when sourceType == ...Camera)
@property (nonatomic) ZBarReaderControllerCameraMode cameraMode;

// whether to automatically take a full picture when a barcode is detected
// (when cameraMode != ...Default)
@property (nonatomic) BOOL takesPicture;

// direct scanner interface - scan UIImage and return something enumerable
- (id <NSFastEnumeration>) scanImage: (CGImageRef) image;

@end

extern NSString* const ZBarReaderControllerResults;
