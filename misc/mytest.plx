#!/usr/bin/perl
use strict;
use lib 'blib/lib', 'blib/arch';
use feature 'say';
use Benchmark qw/timethis timethese/;
use Panda::URI qw/uri encode_uri_component encodeURIComponent decodeURIComponent :const/;
use Data::Dumper qw/Dumper/;
use Storable qw/freeze thaw dclone/;
use JSON::XS;
use URI;
say "START";

use Devel::Peek;

my $u = Panda::URI->new("http://ya.ru/path?a=b&c=d#jjj");
my $uu = URI->new("http://ya.ru/path?a=b&c=d#jjj");

timethis(-1, sub { $u->query });
timethis(-1, sub { my %a = $uu->query_form });



while (1) {
    my $uri = uri("http://ya.ru");
}

my $uri = uri("http://ya.ru/path?a=b&c=d#jjj");
my $f = freeze($uri);
my $c = thaw($f);


say "END";
