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
    UIView *overlay, *controls;

    id <ZBarReaderDelegate> readerDelegate;
    BOOL hasOverlay, showsZBarControls, showsHelpOnFail;
}

// access to configure image scanner
@property (readonly, nonatomic) ZBarImageScanner *scanner;

// barcode result recipient
@property (nonatomic, assign) id <ZBarReaderDelegate> readerDelegate;

// whether to use alternate control set (FIXME broken for NO)
@property (nonatomic) BOOL showsZBarControls;

// whether to display helpful information when decoding fails
@property (nonatomic) BOOL showsHelpOnFail;

// direct scanner interface - scan UIImage and return something enumerable
- (id <NSFastEnumeration>) scanImage: (UIImage*) image;

@end

extern NSString* const ZBarReaderControllerResults;
