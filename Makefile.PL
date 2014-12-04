use strict;
use Panda::Install;

write_makefile(
    NAME      => 'Panda::URI',
    PREREQ_PM => {'Panda::Export' => 0},
    CPLUS     => 1,
    SRC       => 'src',
    INC       => '-Isrc -I/usr/local/include',
    TYPEMAPS  => 'typemap',
    BIN_DEPS  => 'Panda::Lib',
    BIN_SHARE => {
        INCLUDE  => {'src' => '/'},
        TYPEMAPS => {'typemap' => ''},
    },
    OPTIMIZE => '-g -O2',
);
