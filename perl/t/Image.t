# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl Image.t'

use warnings;
use strict;
use Test::More tests => 16;

#########################

BEGIN { use_ok('Barcode::ZBar') }

Barcode::ZBar::set_verbosity(16);

#########################

my $image = Barcode::ZBar::Image->new();
isa_ok($image, 'Barcode::ZBar::Image', 'image');

#########################

my $scanner = Barcode::ZBar::ImageScanner->new();
isa_ok($scanner, 'Barcode::ZBar::ImageScanner', 'image scanner');

#########################

can_ok($image, qw(convert convert_resize
                  get_format get_size get_data
                  set_format set_size set_data));

#########################

can_ok($scanner, qw(set_config parse_config enable_cache scan_image));

#########################

$image->set_format('422P');
my $fmt = $image->get_format();
is($fmt, '422P', 'string format accessors');

#########################

ok($fmt == 0x50323234, 'numeric format accessors');

#########################

$image->set_size(114, 80);
is_deeply([$image->get_size()], [114, 80], 'size accessors');

#########################

# FIXME avoid skipping these (eg embed image vs ImageMagick)
SKIP: {
    eval { require Image::Magick };
    skip "Image::Magick not installed", 8 if $@;

    my $im = Image::Magick->new();
    my $err = $im->Read('t/barcode.png');
    die($err) if($err);

    $image->set_size($im->Get(qw(columns rows)));

    {
        my $data = $im->ImageToBlob(
            magick => 'YUV',
            'sampling-factor' => '4:2:2',
            interlace => 'Plane');
        $image->set_data($data);
    }

    is($scanner->scan_image($image), 1, 'scan result');

    #########################

    my @symbols = $image->get_symbols();
    is(scalar(@symbols), 1, 'result size');

    #########################

    my $sym = $symbols[0];
    isa_ok($sym, 'Barcode::ZBar::Symbol', 'symbol');

    #########################

    can_ok($sym, qw(get_type get_data get_count get_loc));

    #########################

    is($sym->get_type(), Barcode::ZBar::Symbol::EAN13, 'result type');

    #########################

    is($sym->get_data(), '9876543210128', 'result data');

    #########################

    my @loc = $sym->get_loc();
    is(scalar(@loc), 4, 'location size');

    #########################

    my $failure = undef;
    foreach my $pt (@loc) {
        if(ref($pt) ne 'ARRAY') {
            $failure = ("location entry is wrong type:" .
                        " expecting ARRAY ref, got " . ref($pt));
            last;
        }
        if(scalar(@{$pt}) != 2) {
            $failure = ("location coordinate has too many entries:" .
                        " expecting 2, got " . scalar(@{$pt}));
            last;
        }
    }
    ok(!defined($failure), 'location structure') or
      diag($failure);

    #########################
}

# FIXME more image tests
