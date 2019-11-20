# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 5;
# ----------------------------------------------------------------
    my $FILES = [qw(
        t/example/hello-ko-euc.xml
        t/example/hello-ko-utf8.xml
    )];
# ----------------------------------------------------------------
SKIP: {
    skip( "Perl $]", 5 ) if ( $] < 5.008 );
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
