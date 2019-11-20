# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 9;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
    my $tpp = XML::TreePP->new();
    my $source = '<root><text>&lt;&gt;&amp;&quot;&apos;&amp;gt;&amp;lt;</text><cdata><![CDATA[&lt;&gt;&amp;&quot;&apos;&amp;gt;&amp;lt;]]></cdata><attr key="&lt;&gt;&amp;&quot;&apos;&amp;gt;&amp;lt;">BBB</attr></root>';
    my $tree = $tpp->parse( $source );

    is( $tree->{root}->{text}, '<>&"\'&gt;&lt;', "parse text node" );
    is( $tree->{root}->{cdata}, '&lt;&gt;&amp;&quot;&apos;&amp;gt;&amp;lt;', "parse cdata node" );
    is( $tree->{root}->{attr}->{'-key'}, '<>&"\'&gt;&lt;', "parse attribute" );

	$tree->{root}->{text_add}  = '&lt;&gt;&amp;&quot;&apos;&amp;gt;&amp;lt;';
	my $cdata_raw = $tree->{root}->{cdata};
	$tree->{root}->{cdata_ref} = \$cdata_raw;

    my $back = $tpp->write( $tree );

	my $text  = ( $back =~ m#<text>(.*)</text># )[0];
    is( $text,  '&lt;&gt;&amp;&quot;&apos;&amp;gt;&amp;lt;', "write text node" );
	my $cdata = ( $back =~ m#<cdata>(.*)</cdata># )[0];
    is( $cdata, '&amp;lt;&amp;gt;&amp;amp;&amp;quot;&amp;apos;&amp;amp;gt;&amp;amp;lt;', "write cdata node (as text node)" );
	my $attr  = ( $back =~ m#<attr\s+key="(.*?)"\s*># )[0];
    is( $attr,  '&lt;&gt;&amp;&quot;&apos;&amp;gt;&amp;lt;', "write attribute" );
	my $tadd  = ( $back =~ m#<text_add>(.*)</text_add># )[0];
    is( $tadd,  '&amp;lt;&amp;gt;&amp;amp;&amp;quot;&amp;apos;&amp;amp;gt;&amp;amp;lt;', "write new var" );
	my $cref  = ( $back =~ m#<cdata_ref>(.*)</cdata_ref># )[0];
    is( $cref,  '<![CDATA[&lt;&gt;&amp;&quot;&apos;&amp;gt;&amp;lt;]]>', "write cdata node (as cdata)" );
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
