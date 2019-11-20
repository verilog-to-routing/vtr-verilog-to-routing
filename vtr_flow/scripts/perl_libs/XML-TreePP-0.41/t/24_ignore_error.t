    use strict;
    use Test::More tests => 7;
    BEGIN { use_ok('XML::TreePP') };

    &no_carp( \&invalid_tag, qr{Invalid tag sequence}i );
    &no_carp( \&no_such_file, qr{file-not-found}i );
    &no_carp( \&invalid_tree, qr{Invalid tree}i );

sub no_carp {
    my $sub = shift;
    my $err = shift;
    local $@;
    &$sub( ignore_error => 1 );
    ok( ! $@, 'ignore error' );
    eval {
        &$sub();
    };
    like( $@, $err, 'raise error' );
}

sub invalid_tag {
    my $tpp = XML::TreePP->new( @_ );
    my $xml = '<root><not_closed></invalid></root>';
    return $tpp->parse( $xml );
}

sub no_such_file {
    my $tpp = XML::TreePP->new( @_ );
    return $tpp->parsefile( 'file-not-found-'.$$ );
}

sub invalid_tree {
    my $tpp = XML::TreePP->new( @_ );
    return $tpp->write( undef );
}

;1;
