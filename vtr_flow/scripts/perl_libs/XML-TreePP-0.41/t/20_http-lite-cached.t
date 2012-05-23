# ----------------------------------------------------------------
    use strict;
    use Test::More;
# ----------------------------------------------------------------
SKIP: {
    local $@;
    eval { require HTTP::Lite; } unless defined $HTTP::Lite::VERSION;
    if ( ! defined $HTTP::Lite::VERSION ) {
        plan skip_all => 'HTTP::Lite is not loaded.';
    }
    if ( ! defined $ENV{MORE_TESTS} ) {
        plan skip_all => 'define $MORE_TESTS to test this.';
    }
    plan tests => 7;
    use_ok('XML::TreePP');

    my $tpp = XML::TreePP->new();
    my $name = ( $0 =~ m#([^/:\\]+)$# )[0];
    $tpp->set( user_agent => "$name " );

    &test_http_post( $tpp, $name );     # use HTTP::Lite
    eval { require LWP::UserAgent; };
    &test_http_post( $tpp, $name );     # use HTTP::Lite again not LWP::UserAgent
}
# ----------------------------------------------------------------
sub test_http_post {
    my $tpp = shift;
    my $name = shift;
    my $url = "http://www.kawa.net/works/perl/treepp/example/envxml.cgi";
    my( $tree, $xml ) = $tpp->parsehttp( POST => $url, '' );
    ok( ref $tree, $url );
    my $agent = $tree->{env}->{HTTP_USER_AGENT};
    unlike( $agent, qr/libwww-perl/, $agent );
    like( $agent, qr/^\Q$name\E/, "User-Agent has '$name'" );
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
