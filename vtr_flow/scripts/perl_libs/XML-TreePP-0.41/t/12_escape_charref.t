# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 23;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
    my $tpp = XML::TreePP->new();
    my $source = '<root>
        <t1_zero>&#0;&#x00;</t1_zero>
        <t2_ctrl>&#1;&#31;&#x01;&#x1F;</t2_ctrl>
        <t3_tab_esc>[&#9;&#x09;]</t3_tab_esc>
        <t3_tab_raw>'."[\x09\x09]".'</t3_tab_raw>
        <t4_lf_esc>[&#10;&#x0A;]</t4_lf_esc>
        <t4_lf_raw>'."[\x0A\x0A]".'</t4_lf_raw>
        <t5_cr_esc>[&#13;&#x0D;]</t5_cr_esc>
        <t5_cr_raw>'."[\x0D\x0D]".'</t5_cr_raw>
        <t6_space>&#32;&#x20;</t6_space>
        <t7_ascii>&#33;&#127;&#x21;&#x7F;</t7_ascii>
        <t8_latin>&#128;&#255;&#x80;&#xFF;</t8_latin>
        <t9_kanji>&#28450;&#x6F22;</t9_kanji>
    </root>';
# ----------------------------------------------------------------
    my $tree = $tpp->parse( $source );

    # control chars are escaped/unescaped.
    is( $tree->{root}->{t1_zero}, "\x00\x00",           "parse: t1_zero" );
    is( $tree->{root}->{t2_ctrl}, "\x01\x1F\x01\x1F",   "parse: t2_ctrl" );

    # TAB,CR,LF are not unescaped but escaped.
    is( $tree->{root}->{t3_tab_esc}, "[\x09\x09]",      "parse: t3_tab_esc" );
    is( $tree->{root}->{t3_tab_raw}, "[\x09\x09]",      "parse: t3_tab_raw" );
    is( $tree->{root}->{t4_lf_esc},  "[\x0A\x0A]",      "parse: t4_lf_esc" );
    is( $tree->{root}->{t4_lf_raw},  "[\x0A\x0A]",      "parse: t4_lf_raw" );
    is( $tree->{root}->{t5_cr_esc},  "[\x0D\x0D]",      "parse: t5_cr_esc" );
    is( $tree->{root}->{t5_cr_raw},  "[\x0D\x0D]",      "parse: t5_cr_raw" );

    # ascii/latin chars are escaped/unescaped.
    is( $tree->{root}->{t6_space}, "\x20\x20",          "parse: t6_space" );
    is( $tree->{root}->{t7_ascii}, "\x21\x7F\x21\x7F",  "parse: t7_ascii" );

#   XML::TreePP 0.37 ignores between U+0080 and U+00FF without xml_deref
#   my $u80 = "\xC2\x80";   # is UTF-8 of "\x80"
#   my $uFF = "\xC3\xBF";   # is UTF-8 of "\xFF"
#   is( $tree->{root}->{t8_latin}, "$u80$uFF$u80$uFF",  "parse: t8_latin" );

    # CJK > 0xFF are not escaped/unescaped.
    is( $tree->{root}->{t9_kanji}, "&#28450;&#x6F22;",  "parse: t9_kanji" );
# ----------------------------------------------------------------
    my $back = $tpp->write( $tree );

    # control chars are escaped/unescaped.
    like( $back, qr/ <t1_zero>  &#0;&#0;            < /x,   "write: t1_zero" );
    like( $back, qr/ <t2_ctrl>  &#1;&#31;&#1;&#31;  < /x,   "write: t2_ctrl" );

    # TAB,CR,LF are not unescaped but escaped.
    like( $back, qr/ <t3_tab_esc>   \[\x09\x09\]    < /x,   "write: t3_tab_esc" );
    like( $back, qr/ <t3_tab_raw>   \[\x09\x09\]    < /x,   "write: t3_tab_raw" );
    like( $back, qr/ <t4_lf_esc>    \[\x0A\x0A\]    < /x,   "write: t4_lf_esc" );
    like( $back, qr/ <t4_lf_raw>    \[\x0A\x0A\]    < /x,   "write: t4_lf_raw" );
    like( $back, qr/ <t5_cr_esc>    \[\x0D\x0D\]    < /x,   "write: t5_cr_esc" );
    like( $back, qr/ <t5_cr_raw>    \[\x0D\x0D\]    < /x,   "write: t5_cr_raw" );

    # ascii/latin chars are escaped/unescaped.
    like( $back, qr/ <t6_space> \x20\x20            < /x,   "write: t6_space" );
    like( $back, qr/ <t7_ascii> !&#127;!&#127;      < /x,   "write: t7_ascii" );

#   XML::TreePP 0.37 ignores between U+0080 and U+00FF without xml_deref
#   like( $back, qr/ <t8_latin> $u80$uFF$u80$uFF    < /x,   "write: t8_latin" );

    # CJK > 0xFF are not escaped/unescaped.
    like( $back, qr/ <t9_kanji> &#28450;&#x6F22;    < /x,   "write: t9_kanji" );
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
