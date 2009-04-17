# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl ZBar.t'

use warnings;
use strict;
use Test::More tests => 3;

#########################

BEGIN { use_ok('Barcode::ZBar') }

#########################

like(Barcode::ZBar::version(), qr<\d.\d>, 'version');

#########################

Barcode::ZBar::set_verbosity(16);
Barcode::ZBar::increase_verbosity();
pass('verbosity');

#########################
