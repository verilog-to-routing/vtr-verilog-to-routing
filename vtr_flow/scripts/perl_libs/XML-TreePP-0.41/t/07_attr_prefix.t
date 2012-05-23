# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 15;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
    my $source = '<root><foo bar="hoge" /></root>';

    my $tpp = XML::TreePP->new();

    my $tree1 = $tpp->parse( $source );
    is( $tree1->{root}->{foo}->{'-bar'}, 'hoge', "parse: default" );

    my $test = $source;
    $test =~ s/\s+//sg;

    foreach my $prefix ( '-', '@', '__', '?}{][)(', '$*@^%+&', '0' ) {
        my $vprefix = defined $prefix ? ( length($prefix) ? $prefix : '""' ) : 'undef';
        $tpp->set( attr_prefix => $prefix );
        my $tree = $tpp->parse( $source );
        is( $tree->{root}->{foo}->{$prefix.'bar'}, 'hoge', "parse: $vprefix" );

        my $back = $tpp->write( $tree );
        $back =~ s/\s+//sg;
        $back =~ s/<\?.*?\?>//s;
        is( $test, $back, "write: $vprefix" );
    }

    $tpp->set( "attr_prefix" );               # remove attr_prefix
    my $tree2 = $tpp->parse( $source );
    is( $tree2->{root}->{foo}->{'-bar'}, 'hoge', "parse: default (again)" );
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
