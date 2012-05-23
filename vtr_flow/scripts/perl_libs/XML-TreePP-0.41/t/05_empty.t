# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 13;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
    my $tpp = XML::TreePP->new( force_array => [qw( one two three )] );
    my $source = '<root> <e1/> <e2 foo="bar"/> <e3></e3> <e4 foo="bar"></e4> <e5> </e5> </root>';
    my $tree = $tpp->parse( $source );

    ok( exists $tree->{root}->{e1}, "empty element" );
    ok( ref $tree->{root}->{e2},    "empty element with attribute" );
    ok( exists $tree->{root}->{e3}, "no child nodes" );
    ok( ref $tree->{root}->{e4},    "attribute" );
    ok( exists $tree->{root}->{e5}, "white space" );

    my $xml = $tpp->write( $tree );
    my $round = $tpp->parse( $xml );

    ok( exists $round->{root}->{e1}, "round trip: empty element" );
    ok( ref $round->{root}->{e2},    "round trip: empty element with attribute" );
    ok( exists $round->{root}->{e3}, "round trip: no child nodes" );
    ok( ref $round->{root}->{e4},    "round trip: attribute" );
    ok( exists $round->{root}->{e5}, "round trip: white space" );

    is( $tree->{root}->{e2}->{"-foo"}, $round->{root}->{e2}->{"-foo"}, "round trip: attribute 1" );
    is( $tree->{root}->{e4}->{"-foo"}, $round->{root}->{e4}->{"-foo"}, "round trip: attribute 2" );
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
