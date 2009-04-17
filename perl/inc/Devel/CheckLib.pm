# $Id: CheckLib.pm,v 1.22 2008/03/12 19:52:50 drhyde Exp $

package Devel::CheckLib;

use strict;
use vars qw($VERSION @ISA @EXPORT);
$VERSION = '0.5';
use Config;

use File::Spec;
use File::Temp;

require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(assert_lib check_lib_or_exit);

# localising prevents the warningness leaking out of this module
local $^W = 1;    # use warnings is a 5.6-ism

_findcc(); # bomb out early if there's no compiler

=head1 NAME

Devel::CheckLib - check that a library is available

=head1 DESCRIPTION

Devel::CheckLib is a perl module that checks whether a particular C
library and its headers are available.

=head1 SYNOPSIS

    # in a Makefile.PL or Build.PL
    use lib qw(inc);
    use Devel::CheckLib;

    check_lib_or_exit( lib => 'jpeg', header => 'jpeglib.h' );
    check_lib_or_exit( lib => [ 'iconv', 'jpeg' ] );
  
    # or prompt for path to library and then do this:
    check_lib_or_exit( lib => 'jpeg', libpath => $additional_path );

=head1 HOW IT WORKS

You pass named parameters to a function, describing to it how to build
and link to the libraries.

It works by trying to compile this:

    int main(void) { return 0; }

and linking it to the specified libraries.  If something pops out the end
which looks executable, then we know that it worked.  That tiny program is
built once for each library that you specify, and (without linking) once
for each header file.

=head1 FUNCTIONS

All of these take the same named parameters and are exported by default.
To avoid exporting them, C<use Devel::CheckLib ()>.

=head2 assert_lib

This takes several named parameters, all of which are optional, and dies
with an error message if any of the libraries listed can
not be found.  B<Note>: dying in a Makefile.PL or Build.PL may provoke
a 'FAIL' report from CPAN Testers' automated smoke testers.  Use 
C<check_lib_or_exit> instead.

The named parameters are:

=over

=item lib

Must be either a string with the name of a single 
library or a reference to an array of strings of library names.  Depending
on the compiler found, library names will be fed to the compiler either as
C<-l> arguments or as C<.lib> file names.  (E.g. C<-ljpeg> or C<jpeg.lib>)

=item libpath

a string or an array of strings
representing additional paths to search for libraries.

=item LIBS

a C<ExtUtils::MakeMaker>-style space-seperated list of
libraries (each preceded by '-l') and directories (preceded by '-L').

=back

And libraries are no use without header files, so ...

=over

=item header

Must be either a string with the name of a single 
header file or a reference to an array of strings of header file names.

=item incpath

a string or an array of strings
representing additional paths to search for headers.

=item INC

a C<ExtUtils::MakeMaker>-style space-seperated list of
incpaths, each preceded by '-I'.

=back

=head2 check_lib_or_exit

This behaves exactly the same as C<assert_lib()> except that instead of
dieing, it warns (with exactly the same error message) and exits.
This is intended for use in Makefile.PL / Build.PL
when you might want to prompt the user for various paths and
things before checking that what they've told you is sane.

If any library or header is missing, it exits with an exit value of 0 to avoid
causing a CPAN Testers 'FAIL' report.  CPAN Testers should ignore this
result -- which is what you want if an external library dependency is not
available.

=cut

sub check_lib_or_exit {
    eval 'assert_lib(@_)';
    if($@) {
        warn $@;
        exit;
    }
}

sub assert_lib {
    my %args = @_;
    my (@libs, @libpaths, @headers, @incpaths);

    # FIXME: these four just SCREAM "refactor" at me
    @libs = (ref($args{lib}) ? @{$args{lib}} : $args{lib}) 
        if $args{lib};
    @libpaths = (ref($args{libpath}) ? @{$args{libpath}} : $args{libpath}) 
        if $args{libpath};
    @headers = (ref($args{header}) ? @{$args{header}} : $args{header}) 
        if $args{header};
    @incpaths = (ref($args{incpath}) ? @{$args{incpath}} : $args{incpath}) 
        if $args{incpath};

    # work-a-like for Makefile.PL's LIBS and INC arguments
    if(defined($args{LIBS})) {
        foreach my $arg (split(/\s+/, $args{LIBS})) {
            die("LIBS argument badly-formed: $arg\n") unless($arg =~ /^-l/i);
            push @{$arg =~ /^-l/ ? \@libs : \@libpaths}, substr($arg, 2);
        }
    }
    if(defined($args{INC})) {
        foreach my $arg (split(/\s+/, $args{INC})) {
            die("INC argument badly-formed: $arg\n") unless($arg =~ /^-I/);
            push @incpaths, substr($arg, 2);
        }
    }

    my @cc = _findcc();
    my @missing;

    # first figure out which headers we can't find ...
    for my $header (@headers) {
        my($ch, $cfile) = File::Temp::tempfile(
            'assertlibXXXXXXXX', SUFFIX => '.c'
        );
        print $ch qq{#include <$header>\nint main(void) { return 0; }\n};
        close($ch);
        my $exefile = File::Temp::mktemp( 'assertlibXXXXXXXX' ) . $Config{_exe};
        my @sys_cmd;
        # FIXME: re-factor - almost identical code later when linking
        if ( $Config{cc} eq 'cl' ) {                 # Microsoft compiler
            require Win32;
            @sys_cmd = (@cc, $cfile, "/Fe$exefile", (map { '/I'.Win32::GetShortPathName($_) } @incpaths));
        } elsif($Config{cc} =~ /bcc32(\.exe)?/) {    # Borland
            @sys_cmd = (@cc, (map { "-I$_" } @incpaths), "-o$exefile", $cfile);
        } else {                                     # Unix-ish
                                                     # gcc, Sun, AIX (gcc, cc)
            @sys_cmd = (@cc, $cfile, (map { "-I$_" } @incpaths), "-o", "$exefile");
        }
        warn "# @sys_cmd\n" if $args{debug};
        my $rv = $args{debug} ? system(@sys_cmd) : _quiet_system(@sys_cmd);
        push @missing, $header if $rv != 0 || ! -x $exefile; 
        _cleanup_exe($exefile);
        unlink $cfile;
    } 

    # now do each library in turn with no headers
    my($ch, $cfile) = File::Temp::tempfile(
        'assertlibXXXXXXXX', SUFFIX => '.c'
    );
    print $ch "int main(void) { return 0; }\n";
    close($ch);
    for my $lib ( @libs ) {
        my $exefile = File::Temp::mktemp( 'assertlibXXXXXXXX' ) . $Config{_exe};
        my @sys_cmd;
        if ( $Config{cc} eq 'cl' ) {                 # Microsoft compiler
            require Win32;
            my @libpath = map { 
                q{/libpath:} . Win32::GetShortPathName($_)
            } @libpaths; 
            @sys_cmd = (@cc, $cfile, "${lib}.lib", "/Fe$exefile", 
                        "/link", @libpath
            );
        } elsif($Config{cc} eq 'CC/DECC') {          # VMS
        } elsif($Config{cc} =~ /bcc32(\.exe)?/) {    # Borland
            my @libpath = map { "-L$_" } @libpaths;
            @sys_cmd = (@cc, "-o$exefile", "-l$lib", @libpath, $cfile);
        } else {                                     # Unix-ish
                                                     # gcc, Sun, AIX (gcc, cc)
            my @libpath = map { "-L$_" } @libpaths;
            @sys_cmd = (@cc, $cfile,  "-o", "$exefile", "-l$lib", @libpath);
        }
        warn "# @sys_cmd\n" if $args{debug};
        my $rv = $args{debug} ? system(@sys_cmd) : _quiet_system(@sys_cmd);
        push @missing, $lib if $rv != 0 || ! -x $exefile; 
        _cleanup_exe($exefile);
    } 
    unlink $cfile;

    my $miss_string = join( q{, }, map { qq{'$_'} } @missing );
    die("Can't link/include $miss_string\n") if @missing;
}

sub _cleanup_exe {
    my ($exefile) = @_;
    my $ofile = $exefile;
    $ofile =~ s/$Config{_exe}$/$Config{_o}/;
    unlink $exefile if -f $exefile;
    unlink $ofile if -f $ofile;
    unlink "$exefile\.manifest" if -f "$exefile\.manifest";
    return
}
    
sub _findcc {
    my @paths = split(/$Config{path_sep}/, $ENV{PATH});
    my @cc = split(/\s+/, $Config{cc});
    return @cc if -x $cc[0];
    foreach my $path (@paths) {
        my $compiler = File::Spec->catfile($path, $cc[0]) . $Config{_exe};
        return ($compiler, @cc[1 .. $#cc]) if -x $compiler;
    }
    die("Couldn't find your C compiler\n");
}

# code substantially borrowed from IPC::Run3
sub _quiet_system {
    my (@cmd) = @_;

    # save handles
    local *STDOUT_SAVE;
    local *STDERR_SAVE;
    open STDOUT_SAVE, ">&STDOUT" or die "CheckLib: $! saving STDOUT";
    open STDERR_SAVE, ">&STDERR" or die "CheckLib: $! saving STDERR";
    
    # redirect to nowhere
    local *DEV_NULL;
    open DEV_NULL, ">" . File::Spec->devnull 
        or die "CheckLib: $! opening handle to null device";
    open STDOUT, ">&" . fileno DEV_NULL
        or die "CheckLib: $! redirecting STDOUT to null handle";
    open STDERR, ">&" . fileno DEV_NULL
        or die "CheckLib: $! redirecting STDERR to null handle";

    # run system command
    my $rv = system(@cmd);

    # restore handles
    open STDOUT, ">&" . fileno STDOUT_SAVE
        or die "CheckLib: $! restoring STDOUT handle";
    open STDERR, ">&" . fileno STDERR_SAVE
        or die "CheckLib: $! restoring STDERR handle";

    return $rv;
}

=head1 PLATFORMS SUPPORTED

You must have a C compiler installed.  We check for C<$Config{cc}>,
both literally as it is in Config.pm and also in the $PATH.

It has been tested with varying degrees on rigourousness on:

=over

=item gcc (on Linux, *BSD, Mac OS X, Solaris, Cygwin)

=item Sun's compiler tools on Solaris

=item IBM's tools on AIX

=item Microsoft's tools on Windows

=item MinGW on Windows (with Strawberry Perl)

=item Borland's tools on Windows

=back

=head1 WARNINGS, BUGS and FEEDBACK

This is a very early release intended primarily for feedback from
people who have discussed it.  The interface may change and it has
not been adequately tested.

Feedback is most welcome, including constructive criticism.
Bug reports should be made using L<http://rt.cpan.org/> or by email.

When submitting a bug report, please include the output from running:

    perl -V
    perl -MDevel::CheckLib -e0

=head1 SEE ALSO

L<Devel::CheckOS>

L<Probe::Perl>

=head1 AUTHORS

David Cantrell E<lt>david@cantrell.org.ukE<gt>

David Golden E<lt>dagolden@cpan.orgE<gt>

Thanks to the cpan-testers-discuss mailing list for prompting us to write it
in the first place;

to Chris Williams for help with Borland support.

=head1 COPYRIGHT and LICENCE

Copyright 2007 David Cantrell. Portions copyright 2007 David Golden.

This module is free-as-in-speech software, and may be used, distributed,
and modified under the same conditions as perl itself.

=head1 CONSPIRACY

This module is also free-as-in-mason software.

=cut

1;
