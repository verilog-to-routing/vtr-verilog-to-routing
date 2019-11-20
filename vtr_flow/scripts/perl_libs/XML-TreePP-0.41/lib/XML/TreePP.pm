=head1 NAME

XML::TreePP -- Pure Perl implementation for parsing/writing XML documents

=head1 SYNOPSIS

parse an XML document from file into hash tree:

    use XML::TreePP;
    my $tpp = XML::TreePP->new();
    my $tree = $tpp->parsefile( "index.rdf" );
    print "Title: ", $tree->{"rdf:RDF"}->{item}->[0]->{title}, "\n";
    print "URL:   ", $tree->{"rdf:RDF"}->{item}->[0]->{link}, "\n";

write an XML document as string from hash tree:

    use XML::TreePP;
    my $tpp = XML::TreePP->new();
    my $tree = { rss => { channel => { item => [ {
        title   => "The Perl Directory",
        link    => "http://www.perl.org/",
    }, {
        title   => "The Comprehensive Perl Archive Network",
        link    => "http://cpan.perl.org/",
    } ] } } };
    my $xml = $tpp->write( $tree );
    print $xml;

get a remote XML document by HTTP-GET and parse it into hash tree:

    use XML::TreePP;
    my $tpp = XML::TreePP->new();
    my $tree = $tpp->parsehttp( GET => "http://use.perl.org/index.rss" );
    print "Title: ", $tree->{"rdf:RDF"}->{channel}->{title}, "\n";
    print "URL:   ", $tree->{"rdf:RDF"}->{channel}->{link}, "\n";

get a remote XML document by HTTP-POST and parse it into hash tree:

    use XML::TreePP;
    my $tpp = XML::TreePP->new( force_array => [qw( item )] );
    my $cgiurl = "http://search.hatena.ne.jp/keyword";
    my $keyword = "ajax";
    my $cgiquery = "mode=rss2&word=".$keyword;
    my $tree = $tpp->parsehttp( POST => $cgiurl, $cgiquery );
    print "Link: ", $tree->{rss}->{channel}->{item}->[0]->{link}, "\n";
    print "Desc: ", $tree->{rss}->{channel}->{item}->[0]->{description}, "\n";

=head1 DESCRIPTION

XML::TreePP module parses an XML document and expands it for a hash tree.
This generates an XML document from a hash tree as the opposite way around.
This is a pure Perl implementation and requires no modules depended.
This can also fetch and parse an XML document from remote web server
like the XMLHttpRequest object does at JavaScript language.

=head1 EXAMPLES

=head2 Parse XML file

Sample XML document:

    <?xml version="1.0" encoding="UTF-8"?>
    <family name="Kawasaki">
        <father>Yasuhisa</father>
        <mother>Chizuko</mother>
        <children>
            <girl>Shiori</girl>
            <boy>Yusuke</boy>
            <boy>Kairi</boy>
        </children>
    </family>

Sample program to read a xml file and dump it:

    use XML::TreePP;
    use Data::Dumper;
    my $tpp = XML::TreePP->new();
    my $tree = $tpp->parsefile( "family.xml" );
    my $text = Dumper( $tree );
    print $text;

Result dumped:

    $VAR1 = {
        'family' => {
            '-name' => 'Kawasaki',
            'father' => 'Yasuhisa',
            'mother' => 'Chizuko',
            'children' => {
                'girl' => 'Shiori'
                'boy' => [
                    'Yusuke',
                    'Kairi'
                ],
            }
        }
    };

Details:

    print $tree->{family}->{father};        # the father's given name.

The prefix '-' is added on every attribute's name.

    print $tree->{family}->{"-name"};       # the family name of the family

The array is used because the family has two boys.

    print $tree->{family}->{children}->{boy}->[1];  # The second boy's name
    print $tree->{family}->{children}->{girl};      # The girl's name

=head2 Text node and attributes:

If a element has both of a text node and attributes
or both of a text node and other child nodes,
value of a text node is moved to C<#text> like child nodes.

    use XML::TreePP;
    use Data::Dumper;
    my $tpp = XML::TreePP->new();
    my $source = '<span class="author">Kawasaki Yusuke</span>';
    my $tree = $tpp->parse( $source );
    my $text = Dumper( $tree );
    print $text;

The result dumped is following:

    $VAR1 = {
        'span' => {
            '-class' => 'author',
            '#text'  => 'Kawasaki Yusuke'
        }
    };

The special node name of C<#text> is used because this elements
has attribute(s) in addition to the text node.
See also L</text_node_key> option.

=head1 METHODS

=head2 new

This constructor method returns a new XML::TreePP object with C<%options>.

    $tpp = XML::TreePP->new( %options );

=head2 set

This method sets a option value for C<option_name>.
If C<$option_value> is not defined, its option is deleted.

    $tpp->set( option_name => $option_value );

See OPTIONS section below for details.

=head2 get

This method returns a current option value for C<option_name>.

    $tpp->get( 'option_name' );

=head2 parse

This method reads an XML document by string and returns a hash tree converted.
The first argument is a scalar or a reference to a scalar.

        $tree = $tpp->parse( $source );

=head2 parsefile

This method reads an XML document by file and returns a hash tree converted.
The first argument is a filename.

    $tree = $tpp->parsefile( $file );

=head2 parsehttp

This method receives an XML document from a remote server via HTTP and
returns a hash tree converted.

    $tree = $tpp->parsehttp( $method, $url, $body, $head );

C<$method> is a method of HTTP connection: GET/POST/PUT/DELETE
C<$url> is an URI of an XML file.
C<$body> is a request body when you use POST method.
C<$head> is a request headers as a hash ref.
L<LWP::UserAgent> module or L<HTTP::Lite> module is required to fetch a file.

    ( $tree, $xml, $code ) = $tpp->parsehttp( $method, $url, $body, $head );

In array context, This method returns also raw XML document received
and HTTP response's status code.

=head2 write

This method parses a hash tree and returns an XML document as a string.

    $source = $tpp->write( $tree, $encode );

C<$tree> is a reference to a hash tree.

=head2 writefile

This method parses a hash tree and writes an XML document into a file.

    $tpp->writefile( $file, $tree, $encode );

C<$file> is a filename to create.
C<$tree> is a reference to a hash tree.

=head1 OPTIONS FOR PARSING XML

This module accepts option parameters following:

=head2 force_array

This option allows you to specify a list of element names which
should always be forced into an array representation.

    $tpp->set( force_array => [ 'rdf:li', 'item', '-xmlns' ] );

The default value is null, it means that context of the elements
will determine to make array or to keep it scalar or hash.
Note that the special wildcard name C<'*'> means all elements.

=head2 force_hash

This option allows you to specify a list of element names which
should always be forced into an hash representation.

    $tpp->set( force_hash => [ 'item', 'image' ] );

The default value is null, it means that context of the elements
will determine to make hash or to keep it scalar as a text node.
See also L</text_node_key> option below.
Note that the special wildcard name C<'*'> means all elements.

=head2 cdata_scalar_ref

This option allows you to convert a cdata section into a reference
for scalar on parsing an XML document.

    $tpp->set( cdata_scalar_ref => 1 );

The default value is false, it means that each cdata section is converted into a scalar.

=head2 user_agent

This option allows you to specify a HTTP_USER_AGENT string which
is used by parsehttp() method.

    $tpp->set( user_agent => 'Mozilla/4.0 (compatible; ...)' );

The default string is C<'XML-TreePP/#.##'>, where C<'#.##'> is
substituted with the version number of this library.

=head2 http_lite

This option forces pasrsehttp() method to use a L<HTTP::Lite> instance.

    my $http = HTTP::Lite->new();
    $tpp->set( http_lite => $http );

=head2 lwp_useragent

This option forces parsehttp() method to use a L<LWP::UserAgent> instance.

    my $ua = LWP::UserAgent->new();
    $ua->timeout( 60 );
    $ua->env_proxy;
    $tpp->set( lwp_useragent => $ua );

You may use this with L<LWP::UserAgent::WithCache>.

=head2 base_class

This blesses class name for each element's hashref.
Each class is named straight as a child class of it parent class.

    $tpp->set( base_class => 'MyElement' );
    my $xml  = '<root><parent><child key="val">text</child></parent></root>';
    my $tree = $tpp->parse( $xml );
    print ref $tree->{root}->{parent}->{child}, "\n";

A hash for <child> element above is blessed to C<MyElement::root::parent::child>
class. You may use this with L<Class::Accessor>.

=head2 elem_class

This blesses class name for each element's hashref.
Each class is named horizontally under the direct child of C<MyElement>.

    $tpp->set( base_class => 'MyElement' );
    my $xml  = '<root><parent><child key="val">text</child></parent></root>';
    my $tree = $tpp->parse( $xml );
    print ref $tree->{root}->{parent}->{child}, "\n";

A hash for <child> element above is blessed to C<MyElement::child> class.

=head2 xml_deref

This option dereferences the numeric character references, like &#xEB;,
&#28450;, etc., in an XML document when this value is true.

    $tpp->set( xml_deref => 1 );

Note that, for security reasons and your convenient,
this module dereferences the predefined character entity references,
&amp;, &lt;, &gt;, &apos; and &quot;, and the numeric character
references up to U+007F without xml_deref per default.

=head2 require_xml_decl

This option requires XML declaration at the top of XML document to parse.

    $tpp->set( require_xml_decl => 1 );

This will die when <?xml .../?> declration not found.

=head1 OPTIONS FOR WRITING XML

=head2 first_out

This option allows you to specify a list of element/attribute
names which should always appears at first on output XML document.

    $tpp->set( first_out => [ 'link', 'title', '-type' ] );

The default value is null, it means alphabetical order is used.

=head2 last_out

This option allows you to specify a list of element/attribute
names which should always appears at last on output XML document.

    $tpp->set( last_out => [ 'items', 'item', 'entry' ] );

=head2 indent

This makes the output more human readable by indenting appropriately.

    $tpp->set( indent => 2 );

This doesn't strictly follow the XML specification but does looks nice.

=head2 xml_decl

This module inserts an XML declaration on top of the XML document generated
per default. This option forces to change it to another or just remove it.

    $tpp->set( xml_decl => '' );

=head2 output_encoding

This option allows you to specify a encoding of the XML document generated
by write/writefile methods.

    $tpp->set( output_encoding => 'UTF-8' );

On Perl 5.8.0 and later, you can select it from every
encodings supported by Encode.pm. On Perl 5.6.x and before with
Jcode.pm, you can use C<Shift_JIS>, C<EUC-JP>, C<ISO-2022-JP> and
C<UTF-8>. The default value is C<UTF-8> which is recommended encoding.

=head1 OPTIONS FOR BOTH

=head2 utf8_flag

This makes utf8 flag on for every element's value parsed
and makes it on for the XML document generated as well.

    $tpp->set( utf8_flag => 1 );

Perl 5.8.1 or later is required to use this.

=head2 attr_prefix

This option allows you to specify a prefix character(s) which
is inserted before each attribute names.

    $tpp->set( attr_prefix => '@' );

The default character is C<'-'>.
Or set C<'@'> to access attribute values like E4X, ECMAScript for XML.
Zero-length prefix C<''> is available as well, it means no prefix is added.

=head2 text_node_key

This option allows you to specify a hash key for text nodes.

    $tpp->set( text_node_key => '#text' );

The default key is C<#text>.

=head2 ignore_error

This module calls Carp::croak function on an error per default.
This option makes all errors ignored and just returns.

    $tpp->set( ignore_error => 1 );

=head2 use_ixhash

This option keeps the order for each element appeared in XML.
L<Tie::IxHash> module is required.

    $tpp->set( use_ixhash => 1 );

This makes parsing performance slow.
(about 100% slower than default)

=head1 AUTHOR

Yusuke Kawasaki, http://www.kawa.net/

=head1 COPYRIGHT

The following copyright notice applies to all the files provided in
this distribution, including binary files, unless explicitly noted
otherwise.

Copyright 2006-2010 Yusuke Kawasaki

=head1 LICENSE

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

package XML::TreePP;
use strict;
use Carp;
use Symbol;

use vars qw( $VERSION );
$VERSION = '0.41';

my $XML_ENCODING      = 'UTF-8';
my $INTERNAL_ENCODING = 'UTF-8';
my $USER_AGENT        = 'XML-TreePP/'.$VERSION.' ';
my $ATTR_PREFIX       = '-';
my $TEXT_NODE_KEY     = '#text';
my $USE_ENCODE_PM     = ( $] >= 5.008 );
my $ALLOW_UTF8_FLAG   = ( $] >= 5.008001 );

sub new {
    my $package = shift;
    my $self    = {@_};
    bless $self, $package;
    $self;
}

sub die {
    my $self = shift;
    my $mess = shift;
    return if $self->{ignore_error};
    Carp::croak $mess;
}

sub warn {
    my $self = shift;
    my $mess = shift;
    return if $self->{ignore_error};
    Carp::carp $mess;
}

sub set {
    my $self = shift;
    my $key  = shift;
    my $val  = shift;
    if ( defined $val ) {
        $self->{$key} = $val;
    }
    else {
        delete $self->{$key};
    }
}

sub get {
    my $self = shift;
    my $key  = shift;
    $self->{$key} if exists $self->{$key};
}

sub writefile {
    my $self   = shift;
    my $file   = shift;
    my $tree   = shift or return $self->die( 'Invalid tree' );
    my $encode = shift;
    return $self->die( 'Invalid filename' ) unless defined $file;
    my $text = $self->write( $tree, $encode );
    if ( $ALLOW_UTF8_FLAG && utf8::is_utf8( $text ) ) {
        utf8::encode( $text );
    }
    $self->write_raw_xml( $file, $text );
}

sub write {
    my $self = shift;
    my $tree = shift or return $self->die( 'Invalid tree' );
    my $from = $self->{internal_encoding} || $INTERNAL_ENCODING;
    my $to   = shift || $self->{output_encoding} || $XML_ENCODING;
    my $decl = $self->{xml_decl};
    $decl = '<?xml version="1.0" encoding="' . $to . '" ?>' unless defined $decl;

    local $self->{__first_out};
    if ( exists $self->{first_out} ) {
        my $keys = $self->{first_out};
        $keys = [$keys] unless ref $keys;
        $self->{__first_out} = { map { $keys->[$_] => $_ } 0 .. $#$keys };
    }

    local $self->{__last_out};
    if ( exists $self->{last_out} ) {
        my $keys = $self->{last_out};
        $keys = [$keys] unless ref $keys;
        $self->{__last_out} = { map { $keys->[$_] => $_ } 0 .. $#$keys };
    }

    my $tnk = $self->{text_node_key} if exists $self->{text_node_key};
    $tnk = $TEXT_NODE_KEY unless defined $tnk;
    local $self->{text_node_key} = $tnk;

    my $apre = $self->{attr_prefix} if exists $self->{attr_prefix};
    $apre = $ATTR_PREFIX unless defined $apre;
    local $self->{__attr_prefix_len} = length($apre);
    local $self->{__attr_prefix_rex} = $apre;

    local $self->{__indent};
    if ( exists $self->{indent} && $self->{indent} ) {
        $self->{__indent} = ' ' x $self->{indent};
    }

    if ( ! UNIVERSAL::isa( $tree, 'HASH' )) {
        return $self->die( 'Invalid tree' );
    }

    my $text = $self->hash_to_xml( undef, $tree );
    if ( $from && $to ) {
        my $stat = $self->encode_from_to( \$text, $from, $to );
        return $self->die( "Unsupported encoding: $to" ) unless $stat;
    }

    return $text if ( $decl eq '' );
    join( "\n", $decl, $text );
}

sub parsehttp {
    my $self = shift;

    local $self->{__user_agent};
    if ( exists $self->{user_agent} ) {
        my $agent = $self->{user_agent};
        $agent .= $USER_AGENT if ( $agent =~ /\s$/s );
        $self->{__user_agent} = $agent if ( $agent ne '' );
    } else {
        $self->{__user_agent} = $USER_AGENT;
    }

    my $http = $self->{__http_module};
    unless ( $http ) {
        $http = $self->find_http_module(@_);
        $self->{__http_module} = $http;
    }
    if ( $http eq 'LWP::UserAgent' ) {
        return $self->parsehttp_lwp(@_);
    }
    elsif ( $http eq 'HTTP::Lite' ) {
        return $self->parsehttp_lite(@_);
    }
    else {
        return $self->die( "LWP::UserAgent or HTTP::Lite is required: $_[1]" );
    }
}

sub find_http_module {
    my $self = shift || {};

    if ( exists $self->{lwp_useragent} && ref $self->{lwp_useragent} ) {
        return 'LWP::UserAgent' if defined $LWP::UserAgent::VERSION;
        return 'LWP::UserAgent' if &load_lwp_useragent();
        return $self->die( "LWP::UserAgent is required: $_[1]" );
    }

    if ( exists $self->{http_lite} && ref $self->{http_lite} ) {
        return 'HTTP::Lite' if defined $HTTP::Lite::VERSION;
        return 'HTTP::Lite' if &load_http_lite();
        return $self->die( "HTTP::Lite is required: $_[1]" );
    }

    return 'LWP::UserAgent' if defined $LWP::UserAgent::VERSION;
    return 'HTTP::Lite'     if defined $HTTP::Lite::VERSION;
    return 'LWP::UserAgent' if &load_lwp_useragent();
    return 'HTTP::Lite'     if &load_http_lite();
    return $self->die( "LWP::UserAgent or HTTP::Lite is required: $_[1]" );
}

sub load_lwp_useragent {
    return $LWP::UserAgent::VERSION if defined $LWP::UserAgent::VERSION;
    local $@;
    eval { require LWP::UserAgent; };
    $LWP::UserAgent::VERSION;
}

sub load_http_lite {
    return $HTTP::Lite::VERSION if defined $HTTP::Lite::VERSION;
    local $@;
    eval { require HTTP::Lite; };
    $HTTP::Lite::VERSION;
}

sub load_tie_ixhash {
    return $Tie::IxHash::VERSION if defined $Tie::IxHash::VERSION;
    local $@;
    eval { require Tie::IxHash; };
    $Tie::IxHash::VERSION;
}

sub parsehttp_lwp {
    my $self   = shift;
    my $method = shift or return $self->die( 'Invalid HTTP method' );
    my $url    = shift or return $self->die( 'Invalid URL' );
    my $body   = shift;
    my $header = shift;

    my $ua = $self->{lwp_useragent} if exists $self->{lwp_useragent};
    if ( ! ref $ua ) {
        $ua = LWP::UserAgent->new();
        $ua->timeout(10);
        $ua->env_proxy();
        $ua->agent( $self->{__user_agent} ) if defined $self->{__user_agent};
    } else {
        $ua->agent( $self->{__user_agent} ) if exists $self->{user_agent};
    }

    my $req = HTTP::Request->new( $method, $url );
    my $ct = 0;
    if ( ref $header ) {
        foreach my $field ( sort keys %$header ) {
            my $value = $header->{$field};
            $req->header( $field => $value );
            $ct ++ if ( $field =~ /^Content-Type$/i );
        }
    }
    if ( defined $body && ! $ct ) {
        $req->header( 'Content-Type' => 'application/x-www-form-urlencoded' );
    }
    $req->content($body) if defined $body;
    my $res = $ua->request($req);
    my $code = $res->code();
    my $text;
    if ( $res->can( 'decoded_content' )) {
        $text = $res->decoded_content( charset => 'none' );
    } else {
        $text = $res->content();       # less than LWP 5.802
    }
    my $tree = $self->parse( \$text ) if $res->is_success();
    wantarray ? ( $tree, $text, $code ) : $tree;
}

sub parsehttp_lite {
    my $self   = shift;
    my $method = shift or return $self->die( 'Invalid HTTP method' );
    my $url    = shift or return $self->die( 'Invalid URL' );
    my $body   = shift;
    my $header = shift;

    my $http = HTTP::Lite->new();
    $http->method($method);
    my $ua = 0;
    if ( ref $header ) {
        foreach my $field ( sort keys %$header ) {
            my $value = $header->{$field};
            $http->add_req_header( $field, $value );
            $ua ++ if ( $field =~ /^User-Agent$/i );
        }
    }
    if ( defined $self->{__user_agent} && ! $ua ) {
        $http->add_req_header( 'User-Agent', $self->{__user_agent} );
    }
    $http->{content} = $body if defined $body;
    my $code = $http->request($url) or return;
    my $text = $http->body();
    my $tree = $self->parse( \$text );
    wantarray ? ( $tree, $text, $code ) : $tree;
}

sub parsefile {
    my $self = shift;
    my $file = shift;
    return $self->die( 'Invalid filename' ) unless defined $file;
    my $text = $self->read_raw_xml($file);
    $self->parse( \$text );
}

sub parse {
    my $self = shift;
    my $text = ref $_[0] ? ${$_[0]} : $_[0];
    return $self->die( 'Null XML source' ) unless defined $text;

    my $from = &xml_decl_encoding(\$text) || $XML_ENCODING;
    my $to   = $self->{internal_encoding} || $INTERNAL_ENCODING;
    if ( $from && $to ) {
        my $stat = $self->encode_from_to( \$text, $from, $to );
        return $self->die( "Unsupported encoding: $from" ) unless $stat;
    }

    local $self->{__force_array};
    local $self->{__force_array_all};
    if ( exists $self->{force_array} ) {
        my $force = $self->{force_array};
        $force = [$force] unless ref $force;
        $self->{__force_array} = { map { $_ => 1 } @$force };
        $self->{__force_array_all} = $self->{__force_array}->{'*'};
    }

    local $self->{__force_hash};
    local $self->{__force_hash_all};
    if ( exists $self->{force_hash} ) {
        my $force = $self->{force_hash};
        $force = [$force] unless ref $force;
        $self->{__force_hash} = { map { $_ => 1 } @$force };
        $self->{__force_hash_all} = $self->{__force_hash}->{'*'};
    }

    my $tnk = $self->{text_node_key} if exists $self->{text_node_key};
    $tnk = $TEXT_NODE_KEY unless defined $tnk;
    local $self->{text_node_key} = $tnk;

    my $apre = $self->{attr_prefix} if exists $self->{attr_prefix};
    $apre = $ATTR_PREFIX unless defined $apre;
    local $self->{attr_prefix} = $apre;

    if ( exists $self->{use_ixhash} && $self->{use_ixhash} ) {
        return $self->die( "Tie::IxHash is required." ) unless &load_tie_ixhash();
    }

    # Avoid segfaults when receving random input (RT #42441)
    if ( exists $self->{require_xml_decl} && $self->{require_xml_decl} ) {
        return $self->die( "XML declaration not found" ) unless looks_like_xml(\$text);
    }

    my $flat  = $self->xml_to_flat(\$text);
    my $class = $self->{base_class} if exists $self->{base_class};
    my $tree  = $self->flat_to_tree( $flat, '', $class );
    if ( ref $tree ) {
        if ( defined $class ) {
            bless( $tree, $class );
        }
        elsif ( exists $self->{elem_class} && $self->{elem_class} ) {
            bless( $tree, $self->{elem_class} );
        }
    }
    wantarray ? ( $tree, $text ) : $tree;
}

sub xml_to_flat {
    my $self    = shift;
    my $textref = shift;    # reference
    my $flat    = [];
    my $prefix = $self->{attr_prefix};
    my $ixhash = ( exists $self->{use_ixhash} && $self->{use_ixhash} );

    my $deref = \&xml_unescape;
    my $xml_deref = ( exists $self->{xml_deref} && $self->{xml_deref} );
    if ( $xml_deref ) {
        if (( exists $self->{utf8_flag} && $self->{utf8_flag} ) ||
            ( $ALLOW_UTF8_FLAG && utf8::is_utf8( $$textref ))) {
            $deref = \&xml_deref_string;
        } else {
            $deref = \&xml_deref_octet;
        }
    }

    while ( $$textref =~ m{
        ([^<]*) <
        ((
            \? ([^<>]*) \?
        )|(
            \!\[CDATA\[(.*?)\]\]
        )|(
            \!DOCTYPE\s+([^\[\]<>]*(?:\[.*?\]\s*)?)
        )|(
            \!--(.*?)--
        )|(
            ([^\!\?\s<>](?:"[^"]*"|'[^']*'|[^"'<>])*)
        ))
        > ([^<]*)
    }sxg ) {
        my (
            $ahead,     $match,    $typePI,   $contPI,   $typeCDATA,
            $contCDATA, $typeDocT, $contDocT, $typeCmnt, $contCmnt,
            $typeElem,  $contElem, $follow
          )
          = ( $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13 );
        if ( defined $ahead && $ahead =~ /\S/ ) {
            $ahead =~ s/([^\040-\076])/sprintf("\\x%02X",ord($1))/eg;
            $self->warn( "Invalid string: [$ahead] before <$match>" );
        }

        if ($typeElem) {                        # Element
            my $node = {};
            if ( $contElem =~ s#^/## ) {
                $node->{endTag}++;
            }
            elsif ( $contElem =~ s#/$## ) {
                $node->{emptyTag}++;
            }
            else {
                $node->{startTag}++;
            }
            $node->{tagName} = $1 if ( $contElem =~ s#^(\S+)\s*## );
            unless ( $node->{endTag} ) {
                my $attr;
                while ( $contElem =~ m{
                    ([^\s\=\"\']+)\s*=\s*(?:(")(.*?)"|'(.*?)')
                }sxg ) {
                    my $key = $1;
                    my $val = &$deref( $2 ? $3 : $4 );
                    if ( ! ref $attr ) {
                        $attr = {};
                        tie( %$attr, 'Tie::IxHash' ) if $ixhash;
                    }
                    $attr->{$prefix.$key} = $val;
                }
                $node->{attributes} = $attr if ref $attr;
            }
            push( @$flat, $node );
        }
        elsif ($typeCDATA) {    ## CDATASection
            if ( exists $self->{cdata_scalar_ref} && $self->{cdata_scalar_ref} ) {
                push( @$flat, \$contCDATA );    # as reference for scalar
            }
            else {
                push( @$flat, $contCDATA );     # as scalar like text node
            }
        }
        elsif ($typeCmnt) {                     # Comment (ignore)
        }
        elsif ($typeDocT) {                     # DocumentType (ignore)
        }
        elsif ($typePI) {                       # ProcessingInstruction (ignore)
        }
        else {
            $self->warn( "Invalid Tag: <$match>" );
        }
        if ( $follow =~ /\S/ ) {                # text node
            my $val = &$deref($follow);
            push( @$flat, $val );
        }
    }
    $flat;
}

sub flat_to_tree {
    my $self   = shift;
    my $source = shift;
    my $parent = shift;
    my $class  = shift;
    my $tree   = {};
    my $text   = [];

    if ( exists $self->{use_ixhash} && $self->{use_ixhash} ) {
        tie( %$tree, 'Tie::IxHash' );
    }

    while ( scalar @$source ) {
        my $node = shift @$source;
        if ( !ref $node || UNIVERSAL::isa( $node, "SCALAR" ) ) {
            push( @$text, $node );              # cdata or text node
            next;
        }
        my $name = $node->{tagName};
        if ( $node->{endTag} ) {
            last if ( $parent eq $name );
            return $self->die( "Invalid tag sequence: <$parent></$name>" );
        }
        my $elem = $node->{attributes};
        my $forcehash = $self->{__force_hash_all} || $self->{__force_hash}->{$name};
        my $subclass;
        if ( defined $class ) {
            my $escname = $name;
            $escname =~ s/\W/_/sg;
            $subclass = $class.'::'.$escname;
        }
        if ( $node->{startTag} ) {              # recursive call
            my $child = $self->flat_to_tree( $source, $name, $subclass );
            next unless defined $child;
            my $hasattr = scalar keys %$elem if ref $elem;
            if ( UNIVERSAL::isa( $child, "HASH" ) ) {
                if ( $hasattr ) {
                    # some attributes and some child nodes
                    %$elem = ( %$elem, %$child );
                }
                else {
                    # some child nodes without attributes
                    $elem = $child;
                }
            }
            else {
                if ( $hasattr ) {
                    # some attributes and text node
                    $elem->{$self->{text_node_key}} = $child;
                }
                elsif ( $forcehash ) {
                    # only text node without attributes
                    $elem = { $self->{text_node_key} => $child };
                }
                else {
                    # text node without attributes
                    $elem = $child;
                }
            }
        }
        elsif ( $forcehash && ! ref $elem ) {
            $elem = {};
        }
        # bless to a class by base_class or elem_class
        if ( ref $elem && UNIVERSAL::isa( $elem, "HASH" ) ) {
            if ( defined $subclass ) {
                bless( $elem, $subclass );
            } elsif ( exists $self->{elem_class} && $self->{elem_class} ) {
                my $escname = $name;
                $escname =~ s/\W/_/sg;
                my $elmclass = $self->{elem_class}.'::'.$escname;
                bless( $elem, $elmclass );
            }
        }
        # next unless defined $elem;
        $tree->{$name} ||= [];
        push( @{ $tree->{$name} }, $elem );
    }
    if ( ! $self->{__force_array_all} ) {
        foreach my $key ( keys %$tree ) {
            next if $self->{__force_array}->{$key};
            next if ( 1 < scalar @{ $tree->{$key} } );
            $tree->{$key} = shift @{ $tree->{$key} };
        }
    }
    my $haschild = scalar keys %$tree;
    if ( scalar @$text ) {
        if ( scalar @$text == 1 ) {
            # one text node (normal)
            $text = shift @$text;
        }
        elsif ( ! scalar grep {ref $_} @$text ) {
            # some text node splitted
            $text = join( '', @$text );
        }
        else {
            # some cdata node
            my $join = join( '', map {ref $_ ? $$_ : $_} @$text );
            $text = \$join;
        }
        if ( $haschild ) {
            # some child nodes and also text node
            $tree->{$self->{text_node_key}} = $text;
        }
        else {
            # only text node without child nodes
            $tree = $text;
        }
    }
    elsif ( ! $haschild ) {
        # no child and no text
        $tree = "";
    }
    $tree;
}

sub hash_to_xml {
    my $self      = shift;
    my $name      = shift;
    my $hash      = shift;
    my $out       = [];
    my $attr      = [];
    my $allkeys   = [ keys %$hash ];
    my $fo = $self->{__first_out} if ref $self->{__first_out};
    my $lo = $self->{__last_out}  if ref $self->{__last_out};
    my $firstkeys = [ sort { $fo->{$a} <=> $fo->{$b} } grep { exists $fo->{$_} } @$allkeys ] if ref $fo;
    my $lastkeys  = [ sort { $lo->{$a} <=> $lo->{$b} } grep { exists $lo->{$_} } @$allkeys ] if ref $lo;
    $allkeys = [ grep { ! exists $fo->{$_} } @$allkeys ] if ref $fo;
    $allkeys = [ grep { ! exists $lo->{$_} } @$allkeys ] if ref $lo;
    unless ( exists $self->{use_ixhash} && $self->{use_ixhash} ) {
        $allkeys = [ sort @$allkeys ];
    }
    my $prelen = $self->{__attr_prefix_len};
    my $pregex = $self->{__attr_prefix_rex};
    my $textnk = $self->{text_node_key};

    foreach my $keys ( $firstkeys, $allkeys, $lastkeys ) {
        next unless ref $keys;
        my $elemkey = $prelen ? [ grep { substr($_,0,$prelen) ne $pregex } @$keys ] : $keys;
        my $attrkey = $prelen ? [ grep { substr($_,0,$prelen) eq $pregex } @$keys ] : [];

        foreach my $key ( @$elemkey ) {
            my $val = $hash->{$key};
            if ( !defined $val ) {
                next if ($key eq $textnk);
                push( @$out, "<$key />" );
            }
            elsif ( UNIVERSAL::isa( $val, 'HASH' ) ) {
                my $child = $self->hash_to_xml( $key, $val );
                push( @$out, $child );
            }
            elsif ( UNIVERSAL::isa( $val, 'ARRAY' ) ) {
                my $child = $self->array_to_xml( $key, $val );
                push( @$out, $child );
            }
            elsif ( UNIVERSAL::isa( $val, 'SCALAR' ) ) {
                my $child = $self->scalaref_to_cdata( $key, $val );
                push( @$out, $child );
            }
            else {
                my $ref = ref $val;
                $self->warn( "Unsupported reference type: $ref in $key" ) if $ref;
                my $child = $self->scalar_to_xml( $key, $val );
                push( @$out, $child );
            }
        }

        foreach my $key ( @$attrkey ) {
            my $name = substr( $key, $prelen );
            my $val = &xml_escape( $hash->{$key} );
            push( @$attr, ' ' . $name . '="' . $val . '"' );
        }
    }
    my $jattr = join( '', @$attr );

    if ( defined $name && scalar @$out && ! grep { ! /^</s } @$out ) {
        # Use human-friendly white spacing
        if ( defined $self->{__indent} ) {
            s/^(\s*<)/$self->{__indent}$1/mg foreach @$out;
        }
        unshift( @$out, "\n" );
    }

    my $text = join( '', @$out );
    if ( defined $name ) {
        if ( scalar @$out ) {
            $text = "<$name$jattr>$text</$name>\n";
        }
        else {
            $text = "<$name$jattr />\n";
        }
    }
    $text;
}

sub array_to_xml {
    my $self  = shift;
    my $name  = shift;
    my $array = shift;
    my $out   = [];
    foreach my $val (@$array) {
        if ( !defined $val ) {
            push( @$out, "<$name />\n" );
        }
        elsif ( UNIVERSAL::isa( $val, 'HASH' ) ) {
            my $child = $self->hash_to_xml( $name, $val );
            push( @$out, $child );
        }
        elsif ( UNIVERSAL::isa( $val, 'ARRAY' ) ) {
            my $child = $self->array_to_xml( $name, $val );
            push( @$out, $child );
        }
        elsif ( UNIVERSAL::isa( $val, 'SCALAR' ) ) {
            my $child = $self->scalaref_to_cdata( $name, $val );
            push( @$out, $child );
        }
        else {
            my $ref = ref $val;
            $self->warn( "Unsupported reference type: $ref in $name" ) if $ref;
            my $child = $self->scalar_to_xml( $name, $val );
            push( @$out, $child );
        }
    }

    my $text = join( '', @$out );
    $text;
}

sub scalaref_to_cdata {
    my $self = shift;
    my $name = shift;
    my $ref  = shift;
    my $data = defined $$ref ? $$ref : '';
    $data =~ s#(]])(>)#$1]]><![CDATA[$2#g;
    my $text = '<![CDATA[' . $data . ']]>';
    $text = "<$name>$text</$name>\n" if ( $name ne $self->{text_node_key} );
    $text;
}

sub scalar_to_xml {
    my $self   = shift;
    my $name   = shift;
    my $scalar = shift;
    my $copy   = $scalar;
    my $text   = &xml_escape($copy);
    $text = "<$name>$text</$name>\n" if ( $name ne $self->{text_node_key} );
    $text;
}

sub write_raw_xml {
    my $self = shift;
    my $file = shift;
    my $fh   = Symbol::gensym();
    open( $fh, ">$file" ) or return $self->die( "$! - $file" );
    print $fh @_;
    close($fh);
}

sub read_raw_xml {
    my $self = shift;
    my $file = shift;
    my $fh   = Symbol::gensym();
    open( $fh, $file ) or return $self->die( "$! - $file" );
    local $/ = undef;
    my $text = <$fh>;
    close($fh);
    $text;
}

sub looks_like_xml {
    my $textref = shift;
    my $args = ( $$textref =~ /^(?:\s*\xEF\xBB\xBF)?\s*<\?xml(\s+\S.*)\?>/s )[0];
    if ( ! $args ) {
        return;
    }
    return $args;
}

sub xml_decl_encoding {
    my $textref = shift;
    return unless defined $$textref;
    my $args    = looks_like_xml($textref) or return;
    my $getcode = ( $args =~ /\s+encoding=(".*?"|'.*?')/ )[0] or return;
    $getcode =~ s/^['"]//;
    $getcode =~ s/['"]$//;
    $getcode;
}

sub encode_from_to {
    my $self   = shift;
    my $txtref = shift or return;
    my $from   = shift or return;
    my $to     = shift or return;

    unless ( defined $Encode::EUCJPMS::VERSION ) {
        $from = 'EUC-JP' if ( $from =~ /\beuc-?jp-?(win|ms)$/i );
        $to   = 'EUC-JP' if ( $to   =~ /\beuc-?jp-?(win|ms)$/i );
    }

    my $RE_IS_UTF8 = qr/^utf-?8$/i;
    if ( $from =~ $RE_IS_UTF8 ) {
        $$txtref =~ s/^\xEF\xBB\xBF//s;         # UTF-8 BOM (Byte Order Mark)
    }

    my $setflag = $self->{utf8_flag} if exists $self->{utf8_flag};
    if ( ! $ALLOW_UTF8_FLAG && $setflag ) {
        return $self->die( "Perl 5.8.1 is required for utf8_flag: $]" );
    }

    if ( $USE_ENCODE_PM ) {
        &load_encode();
        my $encver = ( $Encode::VERSION =~ /^([\d\.]+)/ )[0];
        my $check = ( $encver < 2.13 ) ? 0x400 : Encode::FB_XMLCREF();

        my $encfrom = Encode::find_encoding($from) if $from;
        return $self->die( "Unknown encoding: $from" ) unless ref $encfrom;
        my $encto   = Encode::find_encoding($to) if $to;
        return $self->die( "Unknown encoding: $to" ) unless ref $encto;

        if ( $ALLOW_UTF8_FLAG && utf8::is_utf8( $$txtref ) ) {
            if ( $to =~ $RE_IS_UTF8 ) {
                # skip
            } else {
                $$txtref = $encto->encode( $$txtref, $check );
            }
        } else {
            $$txtref = $encfrom->decode( $$txtref );
            if ( $to =~ $RE_IS_UTF8 && $setflag ) {
                # skip
            } else {
                $$txtref = $encto->encode( $$txtref, $check );
            }
        }
    }
    elsif ( (  uc($from) eq 'ISO-8859-1'
            || uc($from) eq 'US-ASCII'
            || uc($from) eq 'LATIN-1' ) && uc($to) eq 'UTF-8' ) {
        &latin1_to_utf8($txtref);
    }
    else {
        my $jfrom = &get_jcode_name($from);
        my $jto   = &get_jcode_name($to);
        return $to if ( uc($jfrom) eq uc($jto) );
        if ( $jfrom && $jto ) {
            &load_jcode();
            if ( defined $Jcode::VERSION ) {
                Jcode::convert( $txtref, $jto, $jfrom );
            }
            else {
                return $self->die( "Jcode.pm is required: $from to $to" );
            }
        }
        else {
            return $self->die( "Encode.pm is required: $from to $to" );
        }
    }
    $to;
}

sub load_jcode {
    return if defined $Jcode::VERSION;
    local $@;
    eval { require Jcode; };
}

sub load_encode {
    return if defined $Encode::VERSION;
    local $@;
    eval { require Encode; };
}

sub latin1_to_utf8 {
    my $strref = shift;
    $$strref =~ s{
        ([\x80-\xFF])
    }{
        pack( 'C2' => 0xC0|(ord($1)>>6),0x80|(ord($1)&0x3F) )
    }exg;
}

sub get_jcode_name {
    my $src = shift;
    my $dst;
    if ( $src =~ /^utf-?8$/i ) {
        $dst = 'utf8';
    }
    elsif ( $src =~ /^euc.*jp(-?(win|ms))?$/i ) {
        $dst = 'euc';
    }
    elsif ( $src =~ /^(shift.*jis|cp932|windows-31j)$/i ) {
        $dst = 'sjis';
    }
    elsif ( $src =~ /^iso-2022-jp/ ) {
        $dst = 'jis';
    }
    $dst;
}

sub xml_escape {
    my $str = shift;
    return '' unless defined $str;
    # except for TAB(\x09),CR(\x0D),LF(\x0A)
    $str =~ s{
        ([\x00-\x08\x0B\x0C\x0E-\x1F\x7F])
    }{
        sprintf( '&#%d;', ord($1) );
    }gex;
    $str =~ s/&(?!#(\d+;|x[\dA-Fa-f]+;))/&amp;/g;
    $str =~ s/</&lt;/g;
    $str =~ s/>/&gt;/g;
    $str =~ s/'/&apos;/g;
    $str =~ s/"/&quot;/g;
    $str;
}

sub xml_unescape {
    my $str = shift;
    my $map = {qw( quot " lt < gt > apos ' amp & )};
    $str =~ s{
        (&(?:\#(\d{1,3})|\#x([0-9a-fA-F]{1,2})|(quot|lt|gt|apos|amp));)
    }{
        $4 ? $map->{$4} : &code_to_ascii( $3 ? hex($3) : $2, $1 );
    }gex;
    $str;
}

sub xml_deref_octet {
    my $str = shift;
    my $map = {qw( quot " lt < gt > apos ' amp & )};
    $str =~ s{
        (&(?:\#(\d{1,7})|\#x([0-9a-fA-F]{1,6})|(quot|lt|gt|apos|amp));)
    }{
        $4 ? $map->{$4} : &code_to_utf8( $3 ? hex($3) : $2, $1 );
    }gex;
    $str;
}

sub xml_deref_string {
    my $str = shift;
    my $map = {qw( quot " lt < gt > apos ' amp & )};
    $str =~ s{
        (&(?:\#(\d{1,7})|\#x([0-9a-fA-F]{1,6})|(quot|lt|gt|apos|amp));)
    }{
        $4 ? $map->{$4} : pack( U => $3 ? hex($3) : $2 );
    }gex;
    $str;
}

sub code_to_ascii {
    my $code = shift;
    if ( $code <= 0x007F ) {
        return pack( C => $code );
    }
    return shift if scalar @_;      # default value
    sprintf( '&#%d;', $code );
}

sub code_to_utf8 {
    my $code = shift;
    if ( $code <= 0x007F ) {
        return pack( C => $code );
    }
    elsif ( $code <= 0x07FF ) {
        return pack( C2 => 0xC0|($code>>6), 0x80|($code&0x3F));
    }
    elsif ( $code <= 0xFFFF ) {
        return pack( C3 => 0xE0|($code>>12), 0x80|(($code>>6)&0x3F), 0x80|($code&0x3F));
    }
    elsif ( $code <= 0x10FFFF ) {
        return pack( C4 => 0xF0|($code>>18), 0x80|(($code>>12)&0x3F), 0x80|(($code>>6)&0x3F), 0x80|($code&0x3F));
    }
    return shift if scalar @_;      # default value
    sprintf( '&#x%04X;', $code );
}

1;
