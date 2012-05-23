# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 41;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
    my $xml1 = '<root><text>aaa<child attr="bar"/>bbb</text></root>';
    my $xml2 = '<root><text attr="foo">ccc<child attr="bar"/>ddd</text></root>';
    my $xml3 = '<root><text><![CDATA[eee]]><child attr="bar"/><![CDATA[fff]]></text></root>';
    my $xml4 = '<root><text attr="foo"><![CDATA[ggg]]><child attr="bar"/><![CDATA[hhh]]></text></root>';
    my $xml5 = '<root><text><![CDATA[iii]]>jjj<![CDATA[kkk]]></text></root>';
    my $xml6 = '<root><text>lll<![CDATA[mmm]]>nnn</text></root>';

    my $tpp = XML::TreePP->new();

foreach my $cdata ( 1, 0 ) {
    $tpp->set( cdata_scalar_ref => $cdata );

    $tpp->set( multi_text_nodes => 0 );
    my $tree1 = $tpp->parse( $xml1 );
    my $tree2 = $tpp->parse( $xml2 );
    my $tree3 = $tpp->parse( $xml3 );
    my $tree4 = $tpp->parse( $xml4 );
    my $tree5 = $tpp->parse( $xml5 );
    my $tree6 = $tpp->parse( $xml6 );

    ok( ! ref $tree1->{root}{text}{'#text'}, '1 parse' );
    ok( ! ref $tree2->{root}{text}{'#text'}, '2 parse' );
    if ( $cdata ) {
        is( ref $tree3->{root}{text}{'#text'}, 'SCALAR', '3 parse cdata' );
        is( ref $tree4->{root}{text}{'#text'}, 'SCALAR', '4 parse cdata' );
        is( ref $tree5->{root}{text},          'SCALAR', '5 parse cdata' );
        is( ref $tree6->{root}{text},          'SCALAR', '6 parse cdata' );
    }
    else {
        ok( ! ref $tree3->{root}{text}{'#text'}, '3 parse' );
        ok( ! ref $tree4->{root}{text}{'#text'}, '4 parse' );
        ok( ! ref $tree5->{root}{text},          '5 parse' );
        ok( ! ref $tree6->{root}{text},          '6 parse' );
    }

    is( $tree1->{root}{text}{'#text'}, 'aaabbb', '1 aaa-bbb' );
    is( $tree2->{root}{text}{'#text'}, 'cccddd', '2 ccc-ddd' );
    if ( $cdata ) {
        is( ref $tree3->{root}{text}{'#text'}, 'SCALAR', '3 eee-fff ref' );
        is( ref $tree4->{root}{text}{'#text'}, 'SCALAR', '4 ggg-hhh ref' );
        is( ref $tree5->{root}{text},          'SCALAR', '5 iii-jjj-kkk ref' );
        is( ref $tree6->{root}{text},          'SCALAR', '6 lll-mmm-nnn ref' );
        is( ${$tree3->{root}{text}{'#text'}}, 'eeefff', '3 eee-fff cdata' );
        is( ${$tree4->{root}{text}{'#text'}}, 'ggghhh', '4 ggg-hhh cdata' );
        is( ${$tree5->{root}{text}}, 'iiijjjkkk', '5 iii-jjj-kkk cdata' );
        is( ${$tree6->{root}{text}}, 'lllmmmnnn', '6 lll-mmm-nnn cdata' );
    }
    else {
        is( $tree3->{root}{text}{'#text'}, 'eeefff', '3 eee-fff' );
        is( $tree4->{root}{text}{'#text'}, 'ggghhh', '4 ggg-hhh' );
        is( $tree5->{root}{text}, 'iiijjjkkk', '5 iii-jjj-kkk' );
        is( $tree6->{root}{text}, 'lllmmmnnn', '6 lll-mmm-nnn' );
    }

    my $write1 = $tpp->write( $tree1 );
    my $write2 = $tpp->write( $tree2 );
    my $write3 = $tpp->write( $tree3 );
    my $write4 = $tpp->write( $tree4 );
    my $write5 = $tpp->write( $tree5 );
    my $write6 = $tpp->write( $tree6 );

    like( $write1, qr/>aaabbb</s, '1 back' );
    like( $write2, qr/>cccddd</s, '2 back' );
    if ( $cdata ) {
        like( $write3, qr/<!\[CDATA\[eeefff\]\]>/s,    '3 write cdata' );
        like( $write4, qr/<!\[CDATA\[ggghhh\]\]>/s,    '4 write cdata' );
        like( $write5, qr/<!\[CDATA\[iiijjjkkk\]\]>/s, '5 write cdata' );
        like( $write6, qr/<!\[CDATA\[lllmmmnnn\]\]>/s, '6 write cdata' );
    }
    else {
        like( $write3, qr/>eeefff</s, '3 write' );
        like( $write4, qr/>ggghhh</s, '4 write' );
        like( $write5, qr/>iiijjjkkk</s, '5 write' );
        like( $write6, qr/>lllmmmnnn</s, '6 write' );
    }
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
