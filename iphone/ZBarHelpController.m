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

#import "ZBarHelpController.h"

enum { BTN_GIVEUP, BTN_TRYAGAIN };

UIButton *_ZBarButton (CGFloat size,
                       CGFloat red,
                       CGFloat green,
                       CGFloat blue)
{
    UIButton *btn = [UIButton buttonWithType: UIButtonTypeCustom];
    UILabel *label = btn.titleLabel;
    label.shadowColor = [UIColor lightGrayColor];
    label.shadowOffset = CGSizeMake(-1,1);
    label.font = [UIFont boldSystemFontOfSize: size * 5 / 12];

    CGRect r = CGRectMake(0, 0, 48, 48);
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx =
        CGBitmapContextCreate(NULL, r.size.width, r.size.height,
                              8, r.size.width * 4, cs,
                              kCGBitmapByteOrderDefault |
                              kCGImageAlphaPremultipliedFirst);
    CGColorSpaceRelease(cs);

    CGContextSetRGBFillColor(ctx, red, green, blue, 1);

    UIImage *buttonset = [UIImage imageNamed: @"zbar-button.png"];
    CGImageRef setimg = buttonset.CGImage;
    CGImageRef up = CGImageCreateWithImageInRect(setimg, r);
    r.origin.x = 48;
    CGImageRef down = CGImageCreateWithImageInRect(setimg, r);
    r.origin.x = 96;
    CGImageRef mask = CGImageCreateWithImageInRect(setimg, r);

    r.origin.x = 0;
    CGContextClipToMask(ctx, r, mask);
    CGImageRelease(mask);

    CGContextFillRect(ctx, r);
    CGContextDrawImage(ctx, r, up);
    CGImageRelease(up);

    CGImageRef img = CGBitmapContextCreateImage(ctx);
    [btn setBackgroundImage:
             [[UIImage imageWithCGImage: img]
                 stretchableImageWithLeftCapWidth: 24
                 topCapHeight: 24]
         forState: UIControlStateNormal];
    CGImageRelease(img);

    CGContextFillRect(ctx, r);
    CGContextDrawImage(ctx, r, down);
    CGImageRelease(down);

    img = CGBitmapContextCreateImage(ctx);
    [btn setBackgroundImage:
             [[UIImage imageWithCGImage: img]
                 stretchableImageWithLeftCapWidth: 24
                 topCapHeight: 24]
         forState: UIControlStateHighlighted];
    CGImageRelease(img);

    CGContextRelease(ctx);
    return(btn);
}

@implementation ZBarHelpController

@synthesize delegate;

- (UILabel*) addLabelAt: (CGRect) frame
               withText: (NSString*) text
                   font: (UIFont*) font
{
    UILabel *label = [[[UILabel alloc] initWithFrame: frame] autorelease];
    label.autoresizingMask = (UIViewAutoresizingFlexibleWidth |
                              UIViewAutoresizingFlexibleHeight);
    label.backgroundColor = [UIColor clearColor];
    label.textColor = [UIColor whiteColor];
    label.font = font;
    label.text = text;
    [self.view addSubview: label];
    return(label);
}

- (void) viewDidLoad
{
    [super viewDidLoad];

    UIView *view = self.view;
    view.backgroundColor = [UIColor colorWithWhite: .125f
                                    alpha: 1 ];

    CGSize size = view.bounds.size;
    CGRect r;
    r.origin.x = 0;
    r.origin.y = 0;
    r.size.width = size.width;
    r.size.height = size.height * 19 / 115;
    UILabel *label =
        [self addLabelAt: r
              withText: @"No Barcode Found"
              font: [UIFont boldSystemFontOfSize: size.height * 12 / 115]];
    label.textAlignment = UITextAlignmentCenter;
    label.adjustsFontSizeToFitWidth = YES;
    label.minimumFontSize = size.height * 5 / 115;
    label.textColor = [UIColor colorWithRed: 1.0f green: 0.4f blue: 0.4f
                               alpha: 1.0f];
    label.shadowColor = [UIColor colorWithRed: 0.5f green: 0.2f blue: 0.2f
                               alpha: 1.0f];
    label.shadowOffset = CGSizeMake(-1.5,1.5);

    UIFont *bigFont = [UIFont boldSystemFontOfSize: size.height * 7 / 115];
    UIFont *smallFont = [UIFont systemFontOfSize: size.height * 4 / 115];

    r.origin.x = size.width * .05f;
    r.size.width = size.width * 0.9f;

    r.origin.y = r.size.height;
    r.size.height = size.height * 8 / 115;
    [self addLabelAt: r
          withText: @"Hints for successful scanning:"
          font: smallFont]
        .textAlignment = UITextAlignmentCenter;

    r.origin.x = r.size.width * 7 / 20;
    r.size.width = r.size.width * 13 / 20;

    r.origin.y = size.height * 31 / 115;
    r.size.height = size.height * 6 / 115;
    [self addLabelAt: r
          withText: @"ensure there is plenty of"
          font: smallFont];

    r.origin.y = size.height * 39 / 115;
    r.size.height = size.height * 8 / 115;
    [self addLabelAt: r
          withText: @"Light"
          font: bigFont]
        .textColor = [UIColor colorWithRed: 1.0f green: 1.0f blue: 0.333f
                              alpha: 1];

    r.origin.y = size.height * 55 / 115;
    r.size.height = size.height * 8 / 115;
    [self addLabelAt: r
          withText: @"Wait"
          font: bigFont]
        .textColor = [UIColor colorWithRed: 0.5f green: 1.0f blue: 0.5f
                              alpha: 1];

    r.origin.y = size.height * 65 / 115;
    r.size.height = size.height * 6 / 115;
    [self addLabelAt: r
          withText: @"for the camera to focus"
          font: smallFont];

    r.origin.y = size.height * 79 / 115;
    r.size.height = size.height * 8 / 115;
    [self addLabelAt: r
          withText: @"Hold Still"
          font: bigFont]
        .textColor = [UIColor colorWithRed: 0.583f green: 0.583f blue: 1.0f
                              alpha: 1];

    r.origin.y = size.height * 89 / 115;
    r.size.height = size.height * 6 / 115;
    [self addLabelAt: r
          withText: @"while taking the picture"
          font: smallFont];

    r.origin.x = size.width / 20;
    r.size.width = r.size.height = size.height * 16 / 115;

    UIImage *iconset = [UIImage imageNamed: @"zbar-helpicons.png"];
    CGImageRef setimg = iconset.CGImage;

    r.origin.y = size.height * 31 / 115;
    UIImageView *icon = [[UIImageView alloc] initWithFrame: r];
    CGImageRef img =
        CGImageCreateWithImageInRect(setimg, CGRectMake(0, 0, 64, 64));
    icon.image = [UIImage imageWithCGImage: img];
    CGImageRelease(img);
    [view addSubview: icon];

    r.origin.y = size.height * 55 / 115;
    icon = [[UIImageView alloc] initWithFrame: r];
    img = CGImageCreateWithImageInRect(setimg, CGRectMake(64, 0, 64, 64));
    icon.image = [UIImage imageWithCGImage: img];
    CGImageRelease(img);
    [view addSubview: icon];

    r.origin.y = size.height * 79 / 115;
    icon = [[UIImageView alloc] initWithFrame: r];
    img = CGImageCreateWithImageInRect(setimg, CGRectMake(128, 0, 64, 64));
    icon.image = [UIImage imageWithCGImage: img];
    CGImageRelease(img);
    [view addSubview: icon];

    r.origin.x = size.width / 40;
    r.origin.y = size.height * 103 / 115;
    r.size.width = size.width * 9 / 20;
    r.size.height = size.height * 12 / 115;

    UIButton *btn = _ZBarButton(48, .85, .15, .15);
    btn.frame = r;
    btn.tag = BTN_GIVEUP;
    [btn addTarget: self
         action: @selector(buttonPressed:)
         forControlEvents: UIControlEventTouchUpInside];
    [btn setTitle: @"Give Up"
         forState: UIControlStateNormal];
    [view addSubview: btn];

    btn = _ZBarButton(48, .1, .7, .1);
    r.origin.x = size.width * 21 / 40;
    btn.frame = r;
    btn.tag = BTN_TRYAGAIN;
    [btn addTarget: self
         action: @selector(buttonPressed:)
         forControlEvents: UIControlEventTouchUpInside];
    [btn setTitle: @"Try Again"
         forState: UIControlStateNormal];
    [view addSubview: btn];
}

- (void) buttonPressed: (UIButton*) button
{
    int idx = button.tag;
    [delegate actionSheet: (UIActionSheet*)self
              clickedButtonAtIndex: idx];
    if(idx)
        [self dismissModalViewControllerAnimated: YES];
}

@end
