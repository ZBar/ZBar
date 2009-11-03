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
#import <QuartzCore/QuartzCore.h>
#import <zbar/ZBarSymbol.h>

@protocol ZBarReaderDelegate <UIImagePickerControllerDelegate>
@end

@interface ZBarReaderController : UIImagePickerController
                                < UINavigationControllerDelegate,
                                  UIImagePickerControllerDelegate,
                                  UIActionSheetDelegate >
{
    zbar_image_scanner_t *scanner;
    UIViewController *picker;
    UIView *overlay;

    id <ZBarReaderDelegate> readerDelegate;
    BOOL showsZBarControls, showsHelpOnFail;
}

@property (nonatomic, assign) id <ZBarReaderDelegate> readerDelegate;

// whether to use alternate control set
@property (nonatomic) BOOL showsZBarControls;

// whether to display helpful information when decoding fails
@property (nonatomic) BOOL showsHelpOnFail;

// decoder configuration
- (void) parseConfig: (NSString*) configStr;
- (void) setSymbology: (zbar_symbol_type_t) symbology
               config: (zbar_config_t) config
                   to: (int) value;

// direct scanner interface
- (id <NSFastEnumeration>) scanImage: (UIImage*) img;

@end

extern NSString* const ZBarReaderControllerResults;
