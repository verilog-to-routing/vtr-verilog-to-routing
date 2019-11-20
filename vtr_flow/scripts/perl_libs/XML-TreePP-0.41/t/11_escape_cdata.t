# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 7;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
    my $tpp = XML::TreePP->new();
    $tpp->set( cdata_scalar_ref => 1 );

    my $source = '<root><text>&lt;&gt;&amp;&gt;&lt;</text><cdata><![CDATA[&lt;&gt;&amp;&gt;&lt;]]></cdata><attr key="&lt;&gt;&amp;&gt;&lt;">BBB</attr></root>';

    my $tree = $tpp->parse( $source );

    is( $tree->{root}->{text}, '<>&><', "parse text node" );
	my $cdata = $tree->{root}->{cdata};
    is( $$cdata, '&lt;&gt;&amp;&gt;&lt;', "parse cdata node" );
    is( $tree->{root}->{attr}->{'-key'}, '<>&><', "parse attribute" );

    my $back = $tpp->write( $tree );

    like( $back, qr{ <text>\s* &lt;&gt;&amp;&gt;&lt; \s*</text> }sx, "write text node" );
    like( $back, qr{ <cdata><!\[CDATA\[&lt;&gt;&amp;&gt;&lt;\]\]></cdata> }sx, "write cdata node (as cdata)" );
    like( $back, qr{ <attr\s+key="&lt;&gt;&amp;&gt;&lt;" }sx, "write attribute" );
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
