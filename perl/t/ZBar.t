# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl ZBar.t'

use warnings;
use strict;
use Test::More tests => 37;

#########################

BEGIN { use_ok('Barcode::ZBar') }

#########################

like(Barcode::ZBar::version(), qr<\d.\d>, 'version');

#########################

Barcode::ZBar::set_verbosity(16);
Barcode::ZBar::increase_verbosity();
pass('verbosity');

#########################

# performs (2 * n) tests
sub test_enum {
    my $name = shift;
    foreach my $test (@_) {
        my $enum = $test->[0];

        is($enum, $test->[1], "$name enum/string compare");

        #########################

        ok($enum == $test->[2], "$name enum/numeric compare");
    }
}

test_enum('config',
    [Barcode::ZBar::Config::ENABLE,      'enable',        0],
    [Barcode::ZBar::Config::ADD_CHECK,   'add-check',     1],
    [Barcode::ZBar::Config::EMIT_CHECK,  'emit-check',    2],
    [Barcode::ZBar::Config::ASCII,       'ascii',         3],
    [Barcode::ZBar::Config::MIN_LEN,     'min-length',   32],
    [Barcode::ZBar::Config::MAX_LEN,     'max-length',   33],
    [Barcode::ZBar::Config::UNCERTAINTY, 'uncertainty',  64],
    [Barcode::ZBar::Config::POSITION,    'position',    128],
    [Barcode::ZBar::Config::X_DENSITY,   'x-density',   256],
    [Barcode::ZBar::Config::Y_DENSITY,   'y-density',   257],
);

#########################

test_enum('modifier',
    [Barcode::ZBar::Modifier::GS1, 'GS1', 0],
    [Barcode::ZBar::Modifier::AIM, 'AIM', 1],
);

#########################

test_enum('orientation',
    [Barcode::ZBar::Orient::UNKNOWN, 'UNKNOWN', -1],
    [Barcode::ZBar::Orient::UP,      'UP',       0],
    [Barcode::ZBar::Orient::RIGHT,   'RIGHT',    1],
    [Barcode::ZBar::Orient::DOWN,    'DOWN',     2],
    [Barcode::ZBar::Orient::LEFT,    'LEFT',     3],
);

#########################
