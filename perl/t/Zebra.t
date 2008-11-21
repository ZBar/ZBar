# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl Zebra.t'

use warnings;
use strict;
use Test::More tests => 3;

#########################

BEGIN { use_ok('Zebra') }

#########################

like(Zebra::version(), qr<\d.\d>, 'version');

#########################

Zebra::set_verbosity(16);
Zebra::increase_verbosity();
pass('verbosity');

#########################
