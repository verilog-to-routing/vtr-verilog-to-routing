# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 3;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
    my $tpp = XML::TreePP->new();
    my $tree = $tpp->parsefile( 't/example/index.rdf' );

    my $title = $tree->{'rdf:RDF'}->{channel}->{title};
    like( $title, qr{ kawa.net }ix, '<title>' );

    my $about = $tree->{'rdf:RDF'}->{channel}->{'-rdf:about'};
    like( $about, qr{ ^http:// }x, '<channel rdf:about="">' );
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
