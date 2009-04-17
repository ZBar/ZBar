#!/usr/bin/perl
#------------------------------------------------------------------------
#  Copyright 2009 (c) Jeff Brown <spadix@users.sourceforge.net>
#
#  This file is part of the ZBar Bar Code Reader.
#
#  The ZBar Bar Code Reader is free software; you can redistribute it
#  and/or modify it under the terms of the GNU Lesser Public License as
#  published by the Free Software Foundation; either version 2.1 of
#  the License, or (at your option) any later version.
#
#  The ZBar Bar Code Reader is distributed in the hope that it will be
#  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
#  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser Public License for more details.
#
#  You should have received a copy of the GNU Lesser Public License
#  along with the ZBar Bar Code Reader; if not, write to the Free
#  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
#  Boston, MA  02110-1301  USA
#
#  http://sourceforge.net/projects/zbar
#------------------------------------------------------------------------
use warnings;
use strict;

use Barcode::ZBar;
use Image::Magick;

warn("no input files specified?\n") if(!@ARGV);

# running output document
my $out = undef;

# barcode scanner
my $scanner = Barcode::ZBar::ImageScanner->new();

foreach my $file (@ARGV) {
    print "scanning from \"$file\"\n";
    my $imseq = Image::Magick->new();
    my $err = $imseq->Read($file);
    warn($err) if($err);

    foreach my $page (@$imseq) {
        # convert ImageMagick page to ZBar image
        my $zimg = Barcode::ZBar::Image->new();
        $zimg->set_format('Y800');
        $zimg->set_size($page->Get(qw(columns rows)));
        $zimg->set_data($page->Clone()->ImageToBlob(magick => 'GRAY', depth => 8));

        # scan for barcodes
        if($scanner->scan_image($zimg)) {
            # write out previous document
            $out->write() if($out);

            # use first symbol found to name next image (FIXME sanitize)
            my $data = ($zimg->get_symbols())[0]->get_data();
            my $idx = $page->Get('scene') + 1;
            print "splitting $data from page $idx\n";

            # create new output document
            $out = Image::Magick->new(filename => $data);
        }

        # append this page to current output
        push(@$out, $page) if($out);
    }

    # write out final document
    $out->write() if($out);
}
