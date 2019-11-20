#   26_attr_prefix_null.t.t

use strict;
use Test::More tests => 6;
BEGIN { use_ok('XML::TreePP') };

my $tpp = XML::TreePP->new();
$tpp->set( attr_prefix => '' );

my $source = '<root><foo bar="hoge"/></root>';
my $expect = '<root><foo><bar>hoge</bar></foo></root>';

my $parse1 = $tpp->parse( $source );
is( $parse1->{root}->{foo}->{bar}, 'hoge', 'parse 1' );

my $write1 = $tpp->write( $parse1 );
$write1 =~ s/\s+//sg;
$write1 =~ s/<\?.*?\?>//s;
is( $write1, $expect, 'write 1' );

my $tree1 = {
    root    =>  {
        foo =>  {
            '@attr' =>  'atmark',
            '-attr' =>  'minus',
            'attr'  =>  'null',
        },
    },
};
my $write2 = $tpp->write( $tree1 );
my $parse2 = $tpp->parse( $write2 );
is( $parse2->{root}->{foo}->{'@attr'}, 'atmark', 'write 2' );
is( $parse2->{root}->{foo}->{'-attr'}, 'minus',  'write 3' );
is( $parse2->{root}->{foo}->{'attr'},  'null',   'write 4' );

1;
