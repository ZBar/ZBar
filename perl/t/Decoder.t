# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl Decoder.t'

use warnings;
use strict;
use Test::More tests => 13;

#########################

BEGIN { use_ok('Barcode::ZBar') }

#########################

my $decoder = Barcode::ZBar::Decoder->new();
isa_ok($decoder, 'Barcode::ZBar::Decoder', 'decoder');

$decoder->parse_config('enable');

#########################

can_ok($decoder, qw(set_config parse_config reset new_scan decode_width
                    get_color get_data get_type set_handler));

#########################

my $sym = $decoder->decode_width(5);
is($sym, Barcode::ZBar::Symbol::NONE, 'enum/enum compare');

#########################

ok($sym == 0, 'enum/numeric compare');

#########################

is($sym, 'None', 'enum/string compare');

#########################

my $handler_type = 0;
my $explicit_closure = 0;

$decoder->set_handler(sub {
    if(!$handler_type) {
        is($_[0], $decoder, 'handler decoder');
    }

    my $type = $_[0]->get_type();
    $handler_type = $type
      if(!$handler_type or $type > Barcode::ZBar::Symbol::PARTIAL);

    ${$_[1]} += 1
}, \$explicit_closure);

#########################

$decoder->reset();
is($decoder->get_color(), Barcode::ZBar::SPACE, 'reset color');

#########################

$decoder->set_config(Barcode::ZBar::Symbol::QRCODE,
                     Barcode::ZBar::Config::ENABLE, 0);

my $encoded =
  '9 111 212241113121211311141132 11111 311213121312121332111132 111 9';

foreach my $width (split(/ */, $encoded)) {
    my $tmp = $decoder->decode_width($width);
    if($tmp > Barcode::ZBar::Symbol::PARTIAL) {
        $sym = ($sym == Barcode::ZBar::Symbol::NONE) ? $tmp : -1;
    }
}
is($sym, Barcode::ZBar::Symbol::EAN13, 'EAN-13 type');

#########################

is($decoder->get_data(), '6268964977804', 'EAN-13 data');

#########################

is($decoder->get_color(), Barcode::ZBar::BAR, 'post-scan color');

#########################

is($handler_type, Barcode::ZBar::Symbol::EAN13, 'handler type');

#########################

is($explicit_closure, 2, 'handler explicit closure');

#########################
