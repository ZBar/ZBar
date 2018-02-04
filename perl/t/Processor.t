# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl Processor.t'

use warnings;
use strict;
use Test::More tests => 20;

#########################

BEGIN { use_ok('Barcode::ZBar') }

Barcode::ZBar::set_verbosity(32);

#########################

my $proc = Barcode::ZBar::Processor->new();
isa_ok($proc, 'Barcode::ZBar::Processor', 'processor');

#########################

can_ok($proc, qw(init set_config parse_config));

#########################

ok(!$proc->parse_config('enable'), 'configuration');

#########################

my $cnt = 0;
my $explicit_closure = 0;

$proc->set_data_handler(sub {

    ok(!$cnt, 'handler invocations');
    $cnt += 1;

    #########################

    is($_[0], $proc, 'handler processor');

    #########################

    my $image = $_[1];
    isa_ok($image, 'Barcode::ZBar::Image', 'image');

    #########################

    my @symbols = $image->get_symbols();
    is(scalar(@symbols), 1, 'result size');

    #########################

    my $sym = $symbols[0];
    isa_ok($sym, 'Barcode::ZBar::Symbol', 'symbol');

    #########################

    is($sym->get_type(), Barcode::ZBar::Symbol::EAN13, 'result type');

    #########################

    is($sym->get_data(), '9876543210128', 'result data');

    #########################

    ok($sym->get_quality() > 0, 'quality');

    #########################

    my @loc = $sym->get_loc();
    ok(scalar(@loc) >= 4, 'location size');

    # structure checked by Image.t

    ${$_[2]} += 1
}, \$explicit_closure);

#########################

$proc->init($ENV{VIDEO_DEVICE});
ok(!$proc->is_visible(), 'initial visibility');

#########################

$proc->set_visible();
ok($proc->is_visible(), 'enabled visiblity');

#########################

ok($proc->user_wait(1.1) >= 0, 'wait w/timeout');

#########################

SKIP: {
    # FIXME factor out image read utility
    eval { require Image::Magick };
    skip "Image::Magick not installed", 11 if $@;
    my $im = Image::Magick->new();
    my $err = $im->Read('t/barcode.png');
    die($err) if($err);
    my $image = Barcode::ZBar::Image->new();
    $image->set_format('422P');
    $image->set_size($im->Get(qw(columns rows)));
    $image->set_data($im->ImageToBlob(
        magick => 'YUV',
        'sampling-factor' => '4:2:2',
         interlace => 'Plane')
    );

    my $rc = $proc->process_image($image);
    ok(!$rc, 'process result');

    $proc->user_wait(.9);

    #########################

    is($explicit_closure, 1, 'handler explicit closure');
}

#########################

$proc->set_data_handler();
pass('unset handler');

#########################

# FIXME more processor tests

$proc = undef;
pass('cleanup');

#########################
