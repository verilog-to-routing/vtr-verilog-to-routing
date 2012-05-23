# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 5;
# ----------------------------------------------------------------
SKIP: {
    skip( "Perl $]", 5 ) if ( $] < 5.008 );
    use_ok('XML::TreePP');
    use_ok('Encode');
    &test_main();
}
# ----------------------------------------------------------------
sub test_main {
    my $tpp = XML::TreePP->new();

    my $xml1 = "<?xml version='1.0' encoding='windows-1250'?><root>FOO</root>";
    my $tree1 = $tpp->parse( $xml1 );
    is( $tree1->{root}, 'FOO', 'windows-1250' );

#   0xxxxxxx                    (00-7f)
#   110xxxxx 10xxxxxx           (c0-df)(80-bf)
#   1110xxxx 10xxxxxx 10xxxxxx  (e0-ef)(80-bf)(80-bf)

    my $soa = "\xEA\x92\xB1";   # is a valid Shift_JIS string
    my $xml3 = "<?xml version='1.0' encoding='Shift_JIS'?><root>$soa</root>";
    my $tree3 = $tpp->parse( $xml3 );
    Encode::from_to( $soa, 'Shift_JIS', 'utf8' );
    is( $tree3->{root}, $soa, 'Shift_JIS' );

    my $xml2 = "<?xml version='1.0' encoding='INVALID_ENCODING'?><root>BAR</root>";
    local $@;
    eval { $tpp->parse( $xml2 ); };
    like( $@, qr/INVALID_ENCODING/, 'INVALID_ENCODING' );
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
