# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 13;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
{
    my $cdatal = '<cdata><![CDATA[';
    my $test = 'bar &amp;lt; &lt;&amp;&gt; &amp;gt; <span><br/></span> bar';
    my $cdatar = ']]></cdata>';

    my $tpp = XML::TreePP->new();
    my $xml1 = join( "", $cdatal, $test, $cdatar );

    $tpp->set( cdata_scalar_ref => 1 );
    my $tree1 = $tpp->parse( $xml1 );
    my $cdata1 = $tree1->{cdata};
    ok( ref $cdata1, "cdata as reference" );
    is( $$cdata1, $test, "cdata escaping" );

    my $xml2 = $tpp->write( $tree1 );
    ok( $xml2 =~ /\Q$cdatal$test$cdatar\E/, "round trip: source" );

    $tpp->set( cdata_scalar_ref => undef );
    my $tree2 = $tpp->parse( $xml2 );
    my $cdata2 = $tree2->{cdata};
    ok( ! ref $cdata2, "round trip: cdata as scalar" );
    is( $cdata2, $test, "round trip: text node escaping" );

    $tree2->{cdata} = \$cdata2;
    my $xml3 = $tpp->write( $tree2 );
    ok( $xml2 =~ /\Q$cdatal$test$cdatar\E/, "round trip: again" );
}
# ----------------------------------------------------------------
{
    my $root1 = '<cdata attr="foo">';
    my $root2 = '<bar/>';
    my $cdatal = '<![CDATA[';
    my $test = 'bar &amp;lt; &lt;&amp;&gt; &amp;gt; <span><br/></span> bar';
    my $cdatar = ']]>';
    my $root3 = '</cdata>';

    my $tpp = XML::TreePP->new();
    my $xml1 = join( '', $root1, $root2, $cdatal, $test, $cdatar, $root3 );

    $tpp->set( cdata_scalar_ref => 1 );

    my $tree1 = $tpp->parse( $xml1 );

    my $cdata1 = $tree1->{cdata}{'#text'};
    ok( ref $cdata1, 'cdata as reference B' );
    is( $$cdata1, $test, 'cdata escaping B' );

    my $xml2 = $tpp->write( $tree1 );
    ok( $xml2 =~ /\Q$cdatal$test$cdatar\E/, 'round trip: source B' );

    $tpp->set( cdata_scalar_ref => undef );
    my $tree2 = $tpp->parse( $xml2 );
    my $cdata2 = $tree2->{cdata}{'#text'};
    ok( ! ref $cdata2, 'round trip: cdata as scalar B' );
    is( $cdata2, $test, 'round trip: text node escaping B' );

    $tree2->{cdata} = \$cdata2;
    my $xml3 = $tpp->write( $tree2 );
    ok( $xml2 =~ /\Q$cdatal$test$cdatar\E/, 'round trip: again B' );
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
