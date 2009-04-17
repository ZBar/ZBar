#------------------------------------------------------------------------
#  Copyright 2008-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
#
#  This file is part of the ZBar Bar Code Reader.
#
#  The ZBar Bar Code Reader is free software; you can redistribute it
#  and/or modify it under the terms of the GNU Lesser Public License as
#  published by the Free Software Foundation; either version 2.1 of
#  the License, or (at your option) any later version.
#
#  The ZBar Bar Code Reader is distributed in the hope that it will be
#  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
#  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser Public License for more details.
#
#  You should have received a copy of the GNU Lesser Public License
#  along with the ZBar Bar Code Reader; if not, write to the Free
#  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
#  Boston, MA  02110-1301  USA
#
#  http://sourceforge.net/projects/zbar
#------------------------------------------------------------------------
package Barcode::ZBar;

use 5.006;
use strict;
use warnings;
use Carp;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(SPACE BAR
                    version increase_verbosity set_verbosity);

our $VERSION = '0.02';

require XSLoader;
XSLoader::load('Barcode::ZBar', $VERSION);

package Barcode::ZBar::Error;

use overload '""' => sub { return($_[0]->error_string()) };

1;
__END__


=head1 NAME

Barcode::ZBar - Perl interface to the ZBar Barcode Reader


=head1 SYNOPSIS

setup:

    use Barcode::ZBar;
    
    my $reader = Barcode::ZBar::Processor->new();
    $reader->init();
    $reader->set_data_handler(\&my_handler);

scan an image:

    my $image = Barcode::ZBar::Image->new();
    $image->set_format('422P');
    $image->set_size(114, 80);
    $image->set_data($raw_bits);
    $reader->process_image($image);

scan from video:

    $reader->set_visible();
    $reader->set_active();
    $reader->user_wait();

collect results:

    my @symbols = $image->get_symbols();
    foreach my $sym (@symbols) {
        print("decoded: " . $sym->get_type() . ":" . $sym->get_data());
    }


=head1 DESCRIPTION

The ZBar Bar Code Reader is a library for scanning and decoding bar
codes from various sources such as video streams, image files or raw
intensity sensors.  It supports EAN, UPC, Code 128, Code 39 and
Interleaved 2 of 5.

These are the bindings for interacting directly with the library from
Perl.


=head1 REFERENCE

=head2 Functions

=over 4

=item version()

Returns the version of the zbar library as "I<major>.I<minor>".

=item increase_verbosity()

Increases global library debug by one level.

=item set_verbosity(I<level>)

Sets global library debug to the indicated level.  Higher numbers give
more verbosity.

=item parse_config(I<configstr>)

Parse a decoder configuration setting into a list containing the
symbology constant, config constant, and value to set.  See the
documentation for C<zbarcam>/C<zbarimg> for available configuration
options.

=back

=head2 Constants

Width stream "color" constants:

=over 4

=item SPACE

Light area or space between bars.

=item BAR

Dark area or colored bar segment.

=back


=head1 SEE ALSO

Barcode::ZBar::Processor, Barcode::ZBar::ImageScanner,
Barcode::ZBar::Image, Barcode::ZBar::Symbol,
Barcode::ZBar::Scanner, Barcode::ZBar::Decoder

zbarimg(1), zbarcam(1)

http://zbar.sf.net


=head1 AUTHOR

Jeff Brown, E<lt>spadix@users.sourceforge.netE<gt>


=head1 COPYRIGHT AND LICENSE

Copyright 2008-2009 (c) Jeff Brown E<lt>spadix@users.sourceforge.netE<gt>

The ZBar Bar Code Reader is free software; you can redistribute it
and/or modify it under the terms of the GNU Lesser Public License as
published by the Free Software Foundation; either version 2.1 of
the License, or (at your option) any later version.

=cut
