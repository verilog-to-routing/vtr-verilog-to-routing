# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 4;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
    my $tpp = XML::TreePP->new();
    my $source = '<root attr="AAA">BBB</root>';
    my $tree = $tpp->parse( $source );

    is( $tree->{root}->{"#text"}, "BBB", "text node" );
    is( $tree->{root}->{"-attr"}, "AAA", "attributes" );

    my $back = $tpp->write( $tree );
    my $test = $source;
    $back =~ s/\s+//sg;
    $back =~ s/<\?.*?\?>//s;
    $test =~ s/\s+//sg;
    is( $back, $test, "parse and write" );
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
