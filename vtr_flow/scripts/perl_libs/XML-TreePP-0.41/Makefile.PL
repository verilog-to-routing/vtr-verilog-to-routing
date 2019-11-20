use ExtUtils::MakeMaker;

my $opt = {
    NAME                =>  'XML::TreePP',
    VERSION_FROM        =>  'lib/XML/TreePP.pm',
    PREREQ_PM           =>  {
        'Test::More'        =>  '0',
#       'LWP::UserAgent'    =>  '0',
#       'HTTP::Lite'        =>  '0',
#       'Jcode'             =>  '0', # on Perl 5.005/5.6.x
    },
};

my $mm = $ExtUtils::MakeMaker::VERSION;
$mm =~ s/[^\d\.]+//g;
$opt->{LICENSE} = 'perl' if ( $mm >= 6.3001 );

my $PERL581 = 1 if ( $] >= 5.008001 );
$opt->{PREREQ_PM}->{Jcode} = '0' unless $PERL581;

eval { require 'LWP/UserAgent.pm'; };
# LWP.pm 5.802 required for decoded_content method
$opt->{PREREQ_PM}->{'LWP'} = '5.802' if $LWP::UserAgent::VERSION;
$opt->{PREREQ_PM}->{'HTTP::Lite'} = '0' unless $LWP::UserAgent::VERSION;

WriteMakefile( %$opt );
