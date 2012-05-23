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
    plan tests => 6;
    use_ok('XML::TreePP');

    my $tpp = XML::TreePP->new();
    my $name = ( $0 =~ m#([^/:\\]+)$# )[0];
    $tpp->set( user_agent => "$name " );

    my $url = "http://www.kawa.net/works/perl/treepp/example/envxml.cgi";

    my $tree1 = $tpp->parsehttp( POST => $url, '' );
    ok( ref $tree1, "POST 1 $url" );
    is( $tree1->{env}->{CONTENT_TYPE}, 'application/x-www-form-urlencoded', 'Content-Type (1) default' );

    my $body = 'Hello, World!';
    my $head2 = {
        Hoge    =>  'Pomu'
    };
    my $tree2 = $tpp->parsehttp( POST => $url, $body, $head2 );
    ok( ref $tree2, "POST 2" );
    is( $tree2->{env}->{CONTENT_TYPE}, 'application/x-www-form-urlencoded', 'Content-Type (2) default' );
    is( $tree2->{env}->{HTTP_HOGE}, 'Pomu', "Original Header" );

#   HTTP::Lite ignores Content-Type header.
#   my $head3 = {
#       'Content-Type'  =>  'text/plain',
#   };
#   my $tree3 = $tpp->parsehttp( POST => $url, $body, $head3 );
#   ok( ref $tree3, "POST 3" );
#   is( $tree3->{env}->{CONTENT_TYPE}, 'text/plain', 'Content-Type (3) change' );
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
