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
    eval { require LWP::UserAgent; } unless defined $LWP::UserAgent::VERSION;
    if ( ! defined $LWP::UserAgent::VERSION ) {
        # ok
    }
    if ( ! defined $ENV{MORE_TESTS} ) {
        plan skip_all => 'define $MORE_TESTS to test this.';
    }
    plan tests => 14;
    use_ok('XML::TreePP');

    my $name = 'HTTP::Lite';
    my $url = "http://www.kawa.net/works/perl/treepp/example/envxml.cgi";
    my $query = time();

    {
        my $tpp = XML::TreePP->new();
        my $http = HTTP::Lite->new();
        ok( ref $http, 'HTTP::Lite->new()' );
        $tpp->set( http_lite => $http );
        $tpp->set( user_agent => '' );
        &test_http_req( $tpp, $name, POST => $url, $query );   # use HTTP::Lite
    }

    {
        my $tpp = XML::TreePP->new();
        my $http = HTTP::Lite->new();
        ok( ref $http, 'HTTP::Lite->new()' );
        $tpp->set( http_lite => $http );
        $tpp->set( user_agent => '' );
        my $ret = &test_http_req( $tpp, $name, GET => "$url?$query" );
        is( $ret, $query, "QUERY_STRING: $query" );
    }
}
# ----------------------------------------------------------------
sub test_http_req {
    my $tpp = shift;
    my $name = shift;

    my( $tree, $xml, $code ) = $tpp->parsehttp( @_ );
    ok( ref $tree, "parsehttp: $_[1]" );

    my $decl = ( $xml =~ /(<\?xml[^>]+>)/ )[0];
    like( $xml, qr/(<\?xml[^>]+>)/, "XML Decl: $decl" );

    is( $code, 200, "HTTP Status: $code" );

    my $agent = $tree->{env}->{HTTP_USER_AGENT};
    ok( $agent, "User-Agent: $agent" );
    like( $agent, qr/\Q$name\E/, "Match: $name" );

    $tree->{env}->{QUERY_STRING};
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
