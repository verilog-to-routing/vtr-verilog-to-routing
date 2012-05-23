# ----------------------------------------------------------------
    use strict;
    use Test::More;
# ----------------------------------------------------------------
{
    local $@;

    eval { require File::Temp; };
    plan skip_all => 'File::Temp is not loaded.' if $@;

    my $writable = &test_filetemp();
    plan skip_all => 'temp file is not writable.' unless $writable;

    plan tests => 7;
    use_ok('XML::TreePP');
    &test_writefile();
}
# ----------------------------------------------------------------
sub test_writefile {
    my $file = File::Temp->new->filename;
    ok( $file, "file:".$file );

    my $foo = 'Hello';
    my $bar = 'World!';
    my $tree = { root => { foo => $foo, bar => $bar }};

    my $tpp = XML::TreePP->new();
    $tpp->writefile( $file, $tree );
    ok( (-s $file), 'writefile' );

    my $check = $tpp->parsefile( $file );
    ok( ref $check, 'parsefile' );
    ok( ref $check->{root}, 'parsefile' );
    is( $check->{root}{foo}, $foo, 'foo' );
    is( $check->{root}{bar}, $bar, 'bar' );

    unlink( $file );
}
# ----------------------------------------------------------------
sub test_filetemp {
    my $file = File::Temp->new->filename or return;
    open( TEMP, "> $file" ) or return;
    print TEMP "EOT\n";
    close( TEMP );
    my $size = ( -s $file );
    unlink( $file );
    $size;
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
