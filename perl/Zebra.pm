#------------------------------------------------------------------------
#  Copyright 2008 (c) Jeff Brown <spadix@users.sourceforge.net>
#
#  This file is part of the Zebra Barcode Library.
#
#  The Zebra Barcode Library is free software; you can redistribute it
#  and/or modify it under the terms of the GNU Lesser Public License as
#  published by the Free Software Foundation; either version 2.1 of
#  the License, or (at your option) any later version.
#
#  The Zebra Barcode Library is distributed in the hope that it will be
#  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
#  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser Public License for more details.
#
#  You should have received a copy of the GNU Lesser Public License
#  along with the Zebra Barcode Library; if not, write to the Free
#  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
#  Boston, MA  02110-1301  USA
#
#  http://sourceforge.net/projects/zebra
#------------------------------------------------------------------------
package Zebra;

use 5.006;
use strict;
use warnings;
use Carp;

require Exporter;

our @ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

our $VERSION = '0.1';

require XSLoader;
XSLoader::load('Zebra', $VERSION);

package Zebra::Error;

use overload '""' => sub { return($_[0]->error_string()) };

1;
__END__

=head1 NAME

Zebra - Perl extension for 

=head1 SYNOPSIS

  use Zebra;

  my $reader = Zebra::Processor->new();

  $reader->parse_config('code39.disable');

  $reader->set_data_handler(my_handler);

  $reader->set_visible();

  $reader->process_one();

=head1 DESCRIPTION

Zebra is a library for scanning and decoding bar codes from various
sources such as video streams, image files or raw intensity sensors.
It supports EAN, UPC, Code 128, Code 39 and Interleaved 2 of 5.

These are the perl bindings for the base library.
Check the C API docs for details.

=head2 EXPORT

None by default.

=head2 Exportable constants

None.

=head2 Exportable functions

None.

=head1 SEE ALSO

zebraimg(1), zebracam(1)

http://zebra.sf.net

=head1 AUTHOR

Jeff Brown, E<lt>spadix@users.sourceforge.netE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright 2008 (c) Jeff Brown <spadix@users.sourceforge.net>

The Zebra Barcode Library is free software; you can redistribute it
and/or modify it under the terms of the GNU Lesser Public License as
published by the Free Software Foundation; either version 2.1 of
the License, or (at your option) any later version.

=cut
