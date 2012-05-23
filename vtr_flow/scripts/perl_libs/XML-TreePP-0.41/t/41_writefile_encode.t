# ----------------------------------------------------------------
    use strict;
    use Test::More;
# ----------------------------------------------------------------
{
    local $@;

    eval { require 5.008001; };
    plan skip_all => 'Perl 5.8.1 is required.' if $@;

    eval { require Encode; };
    plan skip_all => 'Encode is not loaded.' if $@;

    eval { require File::Temp; };
    plan skip_all => 'File::Temp is not loaded.' if $@;

    my $writable = &test_filetemp();
    plan skip_all => 'temp file is not writable.' unless $writable;

    plan tests => 19;
    use_ok('XML::TreePP');
    &test_writefile();
    &test_writefile( utf8_flag => 1 );
}
# ----------------------------------------------------------------
sub test_writefile {
    my $opt  = { @_ };
    my $file = File::Temp->new->filename;
    ok( $file, "file:".$file );

    my $foo = 'Ελληνικά/Español/Français';   # UTF-8
    my $bar = 'Русский/Türkçe/日本語';       # UTF-8
    my $tree = { root => { foo => $foo, bar => $bar }};

    my $tpp  = XML::TreePP->new( %$opt );
    $tpp->writefile( $file, $tree );
    ok( (-s $file), 'writefile' );

    my $out = &read_file( $file );
    like( $out, qr/\Q$foo\E/, 'foo raw' );
    like( $out, qr/\Q$bar\E/, 'bar raw' );

    my $check = $tpp->parsefile( $file );
    ok( ref $check, 'parsefile' );

    if ( $opt->{utf8_flag} ) {
        ok(   utf8::is_utf8($check->{root}{foo}), 'foo string' );
        ok(   utf8::is_utf8($check->{root}{bar}), 'bar string' );
        utf8::decode( $foo );
        utf8::decode( $bar );
        is( $check->{root}{foo}, $foo, 'foo tree string' );
        is( $check->{root}{bar}, $bar, 'bar tree string' );
    } else {
        ok( ! utf8::is_utf8($check->{root}{foo}), 'foo octets' );
        ok( ! utf8::is_utf8($check->{root}{bar}), 'bar octets' );
        is( $check->{root}{foo}, $foo, 'foo tree octets' );
        is( $check->{root}{bar}, $bar, 'bar tree octets' );
    }

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
sub read_file {
    my $file = shift or return;
    open( TEMP, $file ) or return;
    local $/ = undef;
    my $body = <TEMP>;
    close( TEMP );
    $body;
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
