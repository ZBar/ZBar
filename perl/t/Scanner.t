# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl Scanner.t'

use warnings;
use strict;
use Test::More tests => 3;

#########################

BEGIN { use_ok('Barcode::Zebra') }

#########################

my $scanner = Barcode::Zebra::Scanner->new();
isa_ok($scanner, 'Barcode::Zebra::Scanner', 'scanner');

#########################

can_ok($scanner, qw(reset new_scan scan_y get_width get_color));

#########################

# FIXME more scanner tests
