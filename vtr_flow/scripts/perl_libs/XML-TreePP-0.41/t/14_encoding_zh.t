# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 7;
# ----------------------------------------------------------------
    my $FILES = [qw(
        t/example/hello-zh-utf8.xml
        t/example/hello-zh-big5.xml
        t/example/hello-zh-gb2312.xml
    )];
# ----------------------------------------------------------------
SKIP: {
    skip( "Perl $]", 7 ) if ( $] < 5.008 );
    use_ok('XML::TreePP');
    &test_main();
}
# ----------------------------------------------------------------
sub test_main {
    my $tpp = XML::TreePP->new();
    my $prev;
    foreach my $file ( @$FILES ) {
        my $tree = $tpp->parsefile( $file );
        if ( defined $prev ) {
            is( $tree->{root}->{text}, $prev, $file );
        } else {
            like( $tree->{root}->{text}, qr/\S\!/, $file );
            $prev = $tree->{root}->{text};
        }
        my $xml = $tpp->write( $tree );
        like( $xml, qr/^\s*<\?xml[^<>]+encoding="UTF-8"/is, "write encoding" );
    }
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
