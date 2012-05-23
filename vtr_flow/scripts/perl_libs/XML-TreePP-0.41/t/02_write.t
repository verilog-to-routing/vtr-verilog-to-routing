# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 6;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
    my $tpp = XML::TreePP->new();
    my $tree = { rss => { channel => { item => [ {
        title   => "The Perl Directory",
        link    => "http://www.perl.org/",
    }, { 
        title   => "The Comprehensive Perl Archive Network",
        link    => "http://cpan.perl.org/",
    } ] } } };
    my $xml = $tpp->write( $tree );
    like( $xml, qr{^<\?xml version="1.0" encoding="UTF-8"}, "xmldecl" );
    like( $xml, qr{<rss>.*</rss>}s, "rss" );

    my $back = $tpp->parse( $xml );
    is_deeply( $tree, $back, "write and parse" );

#   2006/08/13 added

    $tpp->set( xml_decl => '' );
    my $nodecl = $tpp->write( $back );
    unlike( $nodecl, qr{^<\?xml}, "xml_decl is null" );

    my $decl = '<?xml version="1.0" ?>';
    $tpp->set( xml_decl => $decl );
    my $setdecl = $tpp->write( $back );
    like( $setdecl, qr{^\Q$decl\E}, "xml_decl is set" );
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
