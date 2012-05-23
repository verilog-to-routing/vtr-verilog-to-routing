# ----------------------------------------------------------------
    use strict;
    use Test::More;
# ----------------------------------------------------------------
SKIP: {
    local $@;
    eval { require LWP::UserAgent; } unless defined $LWP::UserAgent::VERSION;
    if ( ! defined $LWP::UserAgent::VERSION ) {
        plan skip_all => 'LWP::UserAgent is not loaded.';
    }
    if ( ! defined $ENV{MORE_TESTS} ) {
        plan skip_all => 'define $MORE_TESTS to test this.';
    }
    plan tests => 7;
    use_ok('XML::TreePP');

    my $tpp = XML::TreePP->new();
    my $name = ( $0 =~ m#([^/:\\]+)$# )[0];
    $tpp->set( user_agent => "$name " );

    &test_http_post( $tpp, $name );     # use LWP::UserAgent
    eval { require HTTP::Lite; };
    &test_http_post( $tpp, $name );     # use LWP::UserAgent again not HTTP::Lite
}
# ----------------------------------------------------------------
sub test_http_post {
    my $tpp = shift;
    my $name = shift;
    my $url = "http://www.kawa.net/works/perl/treepp/example/envxml.cgi";
    my( $tree, $xml ) = $tpp->parsehttp( POST => $url, '' );
    ok( ref $tree, $url );
    my $agent = $tree->{env}->{HTTP_USER_AGENT};
    like( $agent, qr/libwww-perl/, "$agent" );
    like( $agent, qr/^\Q$name\E/, "User-Agent has '$name'" );
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
