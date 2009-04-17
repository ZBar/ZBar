#!/usr/bin/env perl
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

use 5.006;
use warnings;
use strict;

use Barcode::ZBar;

Barcode::ZBar::set_verbosity(15);

my $proc = Barcode::ZBar::Processor->new(1);

$proc->init($ARGV[0] || '/dev/video0');

$proc->set_visible();
$proc->user_wait(2);

$proc->set_active();
$proc->user_wait(5);

$proc->set_active(0);
$proc->user_wait(2);

$proc->process_one();
$proc->user_wait(1);
