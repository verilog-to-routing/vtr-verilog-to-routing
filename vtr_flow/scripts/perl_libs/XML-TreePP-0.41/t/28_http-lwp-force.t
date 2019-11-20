# ----------------------------------------------------------------
    use strict;
    use Test::More;
# ----------------------------------------------------------------
SKIP: {
    local $@;
    eval { require HTTP::Lite; } unless defined $HTTP::Lite::VERSION;
    if ( ! defined $HTTP::Lite::VERSION ) {
        # ok
    }
    eval { require LWP::UserAgent; } unless defined $LWP::UserAgent::VERSION;
    if ( ! defined $LWP::UserAgent::VERSION ) {
        plan skip_all => 'LWP::UserAgent is not loaded.';
    }
    if ( ! defined $ENV{MORE_TESTS} ) {
        plan skip_all => 'define $MORE_TESTS to test this.';
    }
    plan tests => 26;
    use_ok('XML::TreePP');

    my $name = ( $0 =~ m#([^/:\\]+)$# )[0];
    my $url = "http://www.kawa.net/works/perl/treepp/example/envxml.cgi";
    my $query = time();

    {
        my $tpp = XML::TreePP->new();
        my $http = LWP::UserAgent->new();
        ok( ref $http, 'LWP::UserAgent->new()' );
        $tpp->set( lwp_useragent => $http );
        &test_http_req( $tpp, 'libwww-perl', POST => $url, $query );
    }
    {
        my $tpp = XML::TreePP->new();
        my $http = LWP::UserAgent->new();
        ok( ref $http, 'LWP::UserAgent->new()' );
        $tpp->set( lwp_useragent => $http );
        $http->agent( "$name " );
        &test_http_req( $tpp, $name, POST => $url, $query );
    }
    {
        my $tpp = XML::TreePP->new();
        my $http = LWP::UserAgent->new();
        ok( ref $http, 'LWP::UserAgent->new()' );
        $tpp->set( user_agent => "$name " );
        &test_http_req( $tpp, $name, POST => $url, $query );
    }
    {
        my $tpp = XML::TreePP->new();
        my $http = LWP::UserAgent->new();
        ok( ref $http, 'LWP::UserAgent->new()' );
        $tpp->set( user_agent => "$name " );
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
