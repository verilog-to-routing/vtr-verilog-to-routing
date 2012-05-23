# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 49;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
    my $tpp = XML::TreePP->new();
    my $list = [
        '<root aaa="AAA" bbb ="BBB" zzz ccc= "CCC" ddd = "DDD" >XXX</root>',
        "<root aaa='AAA' bbb ='BBB' zzz ccc= 'CCC' ddd = 'DDD' >XXX</root>",
        '<root aaa="AAA" bbb ="BBB" zzz ccc= "CCC" ddd = "DDD">XXX</root>',
        "<root aaa='AAA' bbb ='BBB' zzz ccc= 'CCC' ddd = 'DDD'>XXX</root>",
        '<root aaa="AAA" bbb ="BBB" zzz ccc= "CCC" ddd = "DDD" ></root>',
        "<root aaa='AAA' bbb ='BBB' zzz ccc= 'CCC' ddd = 'DDD' ></root>",
        '<root aaa="AAA" bbb ="BBB" zzz ccc= "CCC" ddd = "DDD"></root>',
        "<root aaa='AAA' bbb ='BBB' zzz ccc= 'CCC' ddd = 'DDD'></root>",
        '<root aaa="AAA" bbb ="BBB" zzz ccc= "CCC" ddd = "DDD" />',
        "<root aaa='AAA' bbb ='BBB' zzz ccc= 'CCC' ddd = 'DDD' />",
        '<root aaa="AAA" bbb ="BBB" zzz ccc= "CCC" ddd = "DDD"/>',
        "<root aaa='AAA' bbb ='BBB' zzz ccc= 'CCC' ddd = 'DDD'/>",
    ];

    foreach my $source ( @$list ) {
        my $tree = $tpp->parse( $source );
        my $sep = ( $source =~ /(['"])/ )[0];
        is( $tree->{root}->{"-aaa"}, "AAA", "key=".$sep."val".$sep." (no space)" );
        is( $tree->{root}->{"-bbb"}, "BBB", "key =".$sep."val".$sep." (left space)" );
        is( $tree->{root}->{"-ccc"}, "CCC", "key= ".$sep."val".$sep." (right space)" );
        is( $tree->{root}->{"-ddd"}, "DDD", "key = ".$sep."val".$sep." (both space)" );
    }
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
