# ----------------------------------------------------------------
    use strict;
    use Test::More;
# ----------------------------------------------------------------
{
    local $@;

    eval { require Jcode; };
    plan skip_all => 'Jcode is not loaded.' if $@;

    eval { require File::Temp; };
    plan skip_all => 'File::Temp is not loaded.' if $@;

    my $writable = &test_filetemp();
    plan skip_all => 'temp file is not writable.' unless $writable;

    plan tests => 31;
    use_ok('XML::TreePP');
    &test_writefile( 'UTF-8',     'utf8' );
    &test_writefile( 'Shift_JIS', 'sjis' );
    &test_writefile( 'EUC-JP',    'euc'  );
}
# ----------------------------------------------------------------
sub test_writefile {
    my $encode = shift;
    my $jcode  = shift;
    ok( $encode, 'encode:'.$encode );

    my $file = File::Temp->new->filename;
    ok( $file, "file:".$file );

    my $foo_utf8 = 'こんにちは世界';    # UTF-8
    my $bar_utf8 = '8÷4=2±0';         # UTF-8
    my $foo_test = $foo_utf8;
    my $bar_test = $bar_utf8;
    Jcode::convert( \$foo_test, $jcode, 'utf8' );
    Jcode::convert( \$bar_test, $jcode, 'utf8' );
    ok( length($foo_test), 'foo length' );
    ok( length($bar_test), 'bar length' );

    my $tpp = XML::TreePP->new();
    my $tree = { root => { foo => $foo_utf8, bar => $bar_utf8 }};
    $tpp->writefile( $file, $tree, $encode );
    ok( (-s $file), 'writefile' );

    my $out = &read_file( $file );
    like( $out, qr/\Q$foo_test\E/, 'foo raw' );
    like( $out, qr/\Q$bar_test\E/, 'bar raw' );

    my $check = $tpp->parsefile( $file );
    ok( ref $check, 'parsefile' );
    is( $check->{root}{foo}, $foo_utf8, 'foo tree' );
    is( $check->{root}{bar}, $bar_utf8, 'bar tree' );

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
