#!/usr/bin/perl
use warnings;
use strict;
require Image::Magick;
require Barcode::ZBar;

$ARGV[0] || die;

# create a reader
my $scanner = Barcode::ZBar::ImageScanner->new();

# configure the reader
$scanner->parse_config("enable");

# obtain image data
my $magick = Image::Magick->new();
$magick->Read($ARGV[0]) && die;
my $raw = $magick->ImageToBlob(magick => 'GRAY');

# wrap image data
my $image = Barcode::ZBar::Image->new();
$image->set_format('Y800');
$image->set_size($magick->Get(qw(columns rows)));
$image->set_data($raw);

# scan the image for barcodes
my $n = $scanner->scan_image($image);

# extract results
foreach my $symbol ($image->get_symbols()) {
    # do something useful with results
    print('decoded ' . $symbol->get_type() .
          ' symbol "' . $symbol->get_data() . "\"\n");
}

# clean up
undef($image);
