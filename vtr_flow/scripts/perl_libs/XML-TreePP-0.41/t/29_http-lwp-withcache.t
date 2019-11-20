# ----------------------------------------------------------------
    use strict;
    use Test::More;
# ----------------------------------------------------------------
SKIP: {
    local $@;
    eval { require LWP::UserAgent::WithCache; } unless defined $LWP::UserAgent::WithCache::VERSION;
    if ( ! defined $LWP::UserAgent::WithCache::VERSION ) {
        plan skip_all => 'LWP::UserAgent::WithCache is not loaded.';
    }
    if ( ! defined $ENV{MORE_TESTS} ) {
        plan skip_all => 'define $MORE_TESTS to test this.';
    }
    plan tests => 6;
    use_ok('XML::TreePP');

    my $http = LWP::UserAgent::WithCache->new();
    ok( ref $http, 'LWP::UserAgent::WithCache' );
    my $name = ( $0 =~ m#([^/:\\]+)$# )[0];
    $http->agent( "$name " );

    my $tpp = XML::TreePP->new();
    $tpp->set( lwp_useragent => $http );

    &test_http_post( $tpp, $name );     # use LWP::UserAgent::WithCache
}
# ----------------------------------------------------------------
sub test_http_post {
    my $tpp = shift;
    my $name = shift;
    my $url = "http://www.kawa.net/works/perl/treepp/example/envxml.cgi";
    my( $tree, $xml ) = $tpp->parsehttp( POST => $url, '' );
    ok( ref $tree, $url );
    my $agent = $tree->{env}->{HTTP_USER_AGENT};
    ok( $agent, "User-Agent: $agent" );
    like( $agent, qr/libwww-perl/, "Test: libwww-perl" );
    like( $agent, qr/\Q$name\E/, "Test: $name" );
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
