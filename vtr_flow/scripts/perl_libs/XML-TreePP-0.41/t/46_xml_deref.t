# ----------------------------------------------------------------
#   this test script is written in utf8 but does not "use utf8" for 5.005-compatibility
# ----------------------------------------------------------------
    use strict;
    use Test::More;
# ----------------------------------------------------------------
{
    plan tests => 33;
    use_ok('XML::TreePP');
    &test_main();
}
# ----------------------------------------------------------------
sub test_main {

    my $src = {};

    $src->{Plain} = <<"EOT";
<root>
    <a>AA</a>
    <z>zz</z>
    <c>©©</c>
    <e>ëë</e>
    <n>んん</n>
    <k>漢漢</k>
</root>
EOT

    $src->{XMLref} = <<"EOT";
<root>
    <a>&#x41;&#65;</a>
    <z>&#x7A;&#122;</z>
    <c>&#xA9;&#169;</c>
    <e>&#xEB;&#235;</e>
    <n>&#x3093;&#12435;</n>
    <k>&#x6F22;&#28450;</k>
</root>
EOT

    $src->{Mixed} = <<"EOT";
<root>
    <a>A&#65;</a>
    <z>z&#122;</z>
    <c>©&#169;</c>
    <e>ë&#235;</e>
    <n>ん&#12435;</n>
    <k>漢&#28450;</k>
</root>
EOT

    foreach my $key ( keys %$src ) {
        phase2( "$key octets", $src->{$key} );
    }
}
# ----------------------------------------------------------------
sub phase2 {
    my $subject = shift;
    my $srcxml  = shift;
    my $srcref  = ( $srcxml =~ /&#\w+;/ );

    foreach my $xml_deref ( 0, 1 ) {
        my $subj3 = $subject .( $xml_deref ? ' xml_deref' : '' );
        my $opt = {
            xml_deref => $xml_deref,
        };
        my $tpp = XML::TreePP->new( %$opt );
        my $tree = $tpp->parse( $srcxml );
        if ( $xml_deref ) {
            check_octets( $subj3, $tree );
        } else {
            if ( $srcref ) {
                check_has_ref( $subj3, $tree );
            } else {
                check_no_ref( $subj3, $tree );
            }
        }
    }
}
# ----------------------------------------------------------------
sub check_has_ref {
    my $subject = shift;
    my $tree    = shift;
    my $root = $tree->{root};

    my $HAS_REF = qr/&#\w+;/;
#   \x00-\x7F is always dereferenced.
#   like( $root->{a}, $HAS_REF, "$subject has_ref: a" );
#   like( $root->{z}, $HAS_REF, "$subject has_ref: z" );
    like( $root->{c}, $HAS_REF, "$subject has_ref: c" );
    like( $root->{e}, $HAS_REF, "$subject has_ref: e" );
    like( $root->{n}, $HAS_REF, "$subject has_ref: n" );
    like( $root->{k}, $HAS_REF, "$subject has_ref: k" );
}
# ----------------------------------------------------------------
sub check_no_ref {
    my $subject = shift;
    my $tree    = shift;
    my $root = $tree->{root};

    my $HAS_REF = qr/&#\w+;/;
    unlike( $root->{a}, $HAS_REF, "$subject no_ref: a" );
    unlike( $root->{z}, $HAS_REF, "$subject no_ref: z" );
    unlike( $root->{c}, $HAS_REF, "$subject no_ref: c" );
    unlike( $root->{e}, $HAS_REF, "$subject no_ref: e" );
    unlike( $root->{n}, $HAS_REF, "$subject no_ref: n" );
    unlike( $root->{k}, $HAS_REF, "$subject no_ref: k" );
}
# ----------------------------------------------------------------
sub check_octets {
    my $subject = shift;
    my $tree    = shift;
    my $root = $tree->{root};

    is( $root->{a}, 'AA', "$subject ok: a" );
    is( $root->{z}, 'zz', "$subject ok: z" );
    is( $root->{c}, '©©', "$subject ok: c" );
    is( $root->{e}, 'ëë', "$subject ok: e" );
    is( $root->{n}, 'んん', "$subject ok: n" );
    is( $root->{k}, '漢漢', "$subject ok: k" );
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
