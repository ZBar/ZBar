# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl Zebra.t'

use warnings;
use strict;
use Test::More tests => 3;

#########################

BEGIN { use_ok('Barcode::Zebra') }

#########################

like(Barcode::Zebra::version(), qr<\d.\d>, 'version');

#########################

Barcode::Zebra::set_verbosity(16);
Barcode::Zebra::increase_verbosity();
pass('verbosity');

#########################
