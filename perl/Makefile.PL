use 5.006;
use ExtUtils::MakeMaker;

WriteMakefile(
    NAME              => 'Barcode::Zebra',
    VERSION_FROM      => "Zebra.pm",
    ABSTRACT_FROM     => "Zebra.pm",
    AUTHOR            => 'Jeff Brown <spadix@users.sourceforge.net>',
    LIBS              => ['-lzebra'],
);
