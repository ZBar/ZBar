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
    return(*(uint32_t*)[format UTF8String]);
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

- (id) initWithCGImage: (CGImageRef) image
                  crop: (CGRect) crop
                  size: (CGSize) size
{
    if(!(self = [self init]))
        return(nil);
    uint64_t t_start = timer_now();

    unsigned int w = size.width + 0.5;
    unsigned int h = size.height + 0.5;

    unsigned long datalen = w * h;
    uint8_t *raw = malloc(datalen);
    // FIXME handle OOM
    assert(raw);

    zbar_image_set_data(zimg, raw, datalen, zbar_image_free_data);
    zbar_image_set_format(zimg, *(int*)"Y800");
    zbar_image_set_size(zimg, w, h);

    // scale and crop simultaneously
    CGFloat scale = size.width / crop.size.width;
    crop.origin.x *= -scale;
    crop.size.width = scale * (CGFloat)CGImageGetWidth(image);
    scale = size.height / crop.size.height;
    CGFloat height = CGImageGetHeight(image);
    // compensate for wacky origin
    crop.origin.y = height - crop.origin.y - crop.size.height;
    crop.origin.y *= -scale;
    crop.size.height = scale * height;

    // generate grayscale image data
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceGray();
    CGContextRef ctx =
        CGBitmapContextCreate(raw, w, h, 8, w, cs, kCGImageAlphaNone);
    CGColorSpaceRelease(cs);
    CGContextSetAllowsAntialiasing(ctx, 0);

    CGContextDrawImage(ctx, crop, image);

#if 0
    zlog(@"convert image %dx%d: crop %g,%g %gx%g size %gx%g (%dx%d)",
         CGImageGetWidth(image), CGImageGetHeight(image),
         crop.origin.x, crop.origin.y, crop.size.width, crop.size.height,
         size.width, size.height, w, h);
    CGImageRef cgdump = CGBitmapContextCreateImage(ctx);
    UIImage *uidump = [[UIImage alloc]
                          initWithCGImage: cgdump];
    CGImageRelease(cgdump);
    UIImageWriteToSavedPhotosAlbum(uidump, nil, nil, NULL);
    [uidump release];
#endif

    CGContextRelease(ctx);

    t_convert = timer_elapsed(t_start, timer_now());
    return(self);
}

- (id) initWithCGImage: (CGImageRef) image
                  size: (CGSize) size
{
    CGRect crop = CGRectMake(0, 0,
                             CGImageGetWidth(image),
                             CGImageGetHeight(image));
    return([self initWithCGImage: image
                 crop: crop
                 size: size]);
}

- (id) initWithCGImage: (CGImageRef) image
{
    CGRect crop = CGRectMake(0, 0,
                             CGImageGetWidth(image),
                             CGImageGetHeight(image));
    return([self initWithCGImage: image
                 crop: crop
                 size: crop.size]);
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
