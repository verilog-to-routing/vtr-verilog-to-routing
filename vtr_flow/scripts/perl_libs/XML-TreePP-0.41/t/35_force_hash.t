# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 23;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
    my $source = <<"EOT";
<root>
    <zero>ZZZ</zero>
    <one>AAA</one>
    <two>BBB</two><two attr="CCC"></two>
    <three attr="DDD">EEE</three><three></three><three/>
</root>
EOT
# ----------------------------------------------------------------
{
    my $tpp = XML::TreePP->new( force_hash => [qw( one two three )] );
    my $tree = $tpp->parse( $source );
    ok( ! ref $tree->{root}->{zero},     "A: zero" );
    ok( ref $tree->{root}->{one},        "A: one" );
    ok( ref $tree->{root}->{two}->[0],   "A: two with text node" );
    ok( ref $tree->{root}->{two}->[1],   "A: two with attribute" );
    ok( ref $tree->{root}->{three}->[0], "A: three with both text node and attribute" );
    ok( ref $tree->{root}->{three}->[1], "A: three empty node 1" );
    ok( ref $tree->{root}->{three}->[2], "A: three empty node 2" );
    is( $tree->{root}->{zero},                  'ZZZ', "A: ZZZ" );
    is( $tree->{root}->{one}->{'#text'},        'AAA', "A: AAA" );
    is( $tree->{root}->{two}->[0]->{'#text'},   'BBB', "A: BBB" );
    is( $tree->{root}->{three}->[0]->{'#text'}, 'EEE', "A: EEE" );
}
# ----------------------------------------------------------------
{
    my $tpp = XML::TreePP->new( force_hash => '*' );
    my $tree = $tpp->parse( $source );
    ok( ref $tree->{root}->{zero},       "B: zero" );
    ok( ref $tree->{root}->{one},        "B: one" );
    ok( ref $tree->{root}->{two}->[0],   "B: two with text node" );
    ok( ref $tree->{root}->{two}->[1],   "B: two with attribute" );
    ok( ref $tree->{root}->{three}->[0], "B: three with both text node and attribute" );
    ok( ref $tree->{root}->{three}->[1], "B: three empty node 1" );
    ok( ref $tree->{root}->{three}->[2], "B: three empty node 2" );
    is( $tree->{root}->{zero}->{'#text'},       'ZZZ', "B: ZZZ" );
    is( $tree->{root}->{one}->{'#text'},        'AAA', "B: AAA" );
    is( $tree->{root}->{two}->[0]->{'#text'},   'BBB', "B: BBB" );
    is( $tree->{root}->{three}->[0]->{'#text'}, 'EEE', "B: EEE" );
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
