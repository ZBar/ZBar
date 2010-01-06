//------------------------------------------------------------------------
//  Copyright 2009-2010 (c) Jeff Brown <spadix@users.sourceforge.net>
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

#import <zbar/ZBarImage.h>
#import <UIKit/UIKit.h>
#import "debug.h"

static void image_cleanup(zbar_image_t *zimg)
{
    ZBarImage *image = zbar_image_get_userdata(zimg);
    [image cleanup];
}

@implementation ZBarImage

@dynamic format, sequence, size, data, dataLength, zbarImage;

+ (unsigned long) fourcc: (NSString*) format
{
    return(zbar_fourcc_parse([format UTF8String]));
}

- (id) initWithImage: (zbar_image_t*) image
{
    if(!image) {
        [self release];
        return(nil);
    }
    if(self = [super init]) {
        zimg = image;
        zbar_image_ref(image, 1);
        zbar_image_set_userdata(zimg, self);
    }
    return(self);
}

- (id) init
{
    zbar_image_t *image = zbar_image_create();
    self = [self initWithImage: image];
    zbar_image_ref(image, -1);
    return(self);
}

- (void) dealloc
{
    if(zimg) {
        zbar_image_ref(zimg, -1);
        zimg = NULL;
    }
    [super dealloc];
}

- (id) initWithUIImage: (UIImage*) image
                  size: (CGSize) size
{
    if(!(self = [self init]))
        return(nil);

    timer_start;

    unsigned int w = size.width + 0.5;
    unsigned int h = size.height + 0.5;

    unsigned long datalen = w * h;
    uint8_t *raw = malloc(datalen);
    // FIXME handle OOM
    assert(raw);

    zbar_image_set_data(zimg, raw, datalen, zbar_image_free_data);
    zbar_image_set_format(zimg, zbar_fourcc('Y','8','0','0'));
    zbar_image_set_size(zimg, w, h);

    // generate grayscale image data
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceGray();
    CGContextRef ctx =
        CGBitmapContextCreate(raw, w, h, 8, w, cs, kCGImageAlphaNone);
    CGColorSpaceRelease(cs);
    CGContextSetAllowsAntialiasing(ctx, 0);
    CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), image.CGImage);
    CGContextRelease(ctx);

    zlog(@"ZBarImage: converted UIImage %gx%g to %dx%d Y800 in %gs\n",
         image.size.width, image.size.height, w, h,
         timer_elapsed(t_start, timer_now()));

    return(self);
}

- (id) initWithUIImage: (UIImage*) image
{
    return([self initWithUIImage: image
                 size: image.size]);
}

- (zbar_image_t*) image
{
    return(zimg);
}

- (unsigned long) format
{
    return(zbar_image_get_format(zimg));
}

- (void) setFormat: (unsigned long) format
{
    zbar_image_set_format(zimg, format);
}

- (unsigned) sequence
{
    return(zbar_image_get_sequence(zimg));
}

- (void) setSequence: (unsigned) seq
{
    zbar_image_set_sequence(zimg, seq);
}

- (CGSize) size
{
    return(CGSizeMake(zbar_image_get_width(zimg),
                      zbar_image_get_height(zimg)));
}

- (void) setSize: (CGSize) size
{
    zbar_image_set_size(zimg, size.width + .5, size.height + .5);
}

- (ZBarSymbolSet*) symbols
{
    return([[[ZBarSymbolSet alloc]
                initWithSymbolSet: zbar_image_get_symbols(zimg)]
               autorelease]);
}

- (void) setSymbols: (ZBarSymbolSet*) symbols
{
    zbar_image_set_symbols(zimg, [symbols zbarSymbolSet]);
}

- (const void*) data
{
    return(zbar_image_get_data(zimg));
}

- (unsigned long) dataLength
{
    return(zbar_image_get_data_length(zimg));
}

- (void) setData: (const void*) data
      withLength: (unsigned long) length
{
    zbar_image_set_data(zimg, data, length, image_cleanup);
}

- (zbar_image_t*) zbarImage
{
    return(zimg);
}

- (void) cleanup
{
}

#if 0
- (ZBarImage*) convertToFormat: (unsigned long) format
{
    zbar_image_t *zdst = zbar_image_convert(zimg, format);
    ZBarImage *image = ;
    return([[[ZBarImage alloc] initWithImage: zdst] autorelease]);
}
#endif

@end
