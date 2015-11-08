package Archive::Eubar;

use strict;
use warnings;

if (!caller) {
    my $ar = __PACKAGE__->new(@ARGV);
    print $_, "\n" for $ar->files;
}

sub new {
    my $cls = shift;
    unshift @_, 'base' if @_ % 2;
    bless { @_ }, $cls;
}

sub file {
    my $self = shift;
    bless { @_ }, 'Archive::Eubar::File';
}

sub metadata {
    my ($self, $filter) = @_;
    my $metadata = $self->{'metadata'};
    if (!$metadata) {
        my $fhmeta = $self->_fh('.eum');
        my (%backup, %meta, @files);
        my $ofs = 0;
        while (<$fhmeta>) {
            last if /^$/;
            if (/^[@](\d+) [*](\d+)(?: [#]\S+)? (\S+)$/) {
                my ($pos, $size, $path) = ($1, $2, $3);
                die if !defined $meta{$path};
                $meta{$path}{'@'} = $ofs + $pos;
                next;
            }
            $ofs = $1, next if /^\@([0-9]+)$/;
            $self->{$1} = $2, next if /^(\$\S+) (.*)/;
            next if /^eubar:/;
            next if /^\#eubar/;
            m{\A(.)(.)( [^/]+) (/.*)$}
                or die "Unrecognized metadata line: $_";
            my ($action, $type, $props, $path) = ($1, $2, $3, $4);
            my %file = (
                't' => $type,
                'a' => $action,
                '/' => $path,
            );
            while ($props =~ m{\G ([@*%cdgimnpru])(\S+)}gc) {
                $file{$1} = $2;
            }
            die if !defined(pos $props) || pos $props < length($props);
            $file{'p'} = oct $file{'p'};
            $file{'@'} += $ofs if defined $file{'@'};
            next if $filter && !$filter->(\%file);
            $meta{$path} = $self->file(%file);
            push @files, $path;
        }
        $self->{'files'} = \@files;
        $metadata = $self->{'metadata'} = \%meta;
    }
    return $metadata;
}

sub files {
    my ($self) = @_;
    $self->metadata;
    return @{ $self->{'files'} ||= [] };
}

sub find {
    my ($self, $path) = @_;
    my $metadata = $self->metadata;
    return $metadata->{$path};
}

sub extract {
    my ($self, $file, $fh) = @_;
    my $pos = $file->{'@'};
    my $len = $file->{'*'};
    my $fhdata = $self->_fh('.eud');
    seek $fhdata, $pos, 0 or die "Can't seek to $pos: $!";
    my $buf = '';
    my $n = $len < 8192 ? $len : 8192;
    while ($n = read $fhdata, $buf, $n) {
        print $fh $buf;
        $len -= $n;
        return 1 if $len == 0;
        die if $len < 0;
        $n = $len < 8192 ? $len : 8192;
    }
    die "Unexpected EOF";
}

sub _fh {
    my ($self, $ext) = (shift, pop);
    my $mode = @_ ? shift : '<';
    my $file = $self->{'base'} . $ext;
    my $fh = $self->{$mode.$ext};
    return $fh if defined $fh;
    open $fh, $mode, $file or die "Can't open $file: $!";
    return $self->{$mode.$ext} = $fh;
}

package Archive::Eubar::File;

use Fcntl ':mode';

sub path        { @_ > 1 ? $_[0]->{'/'} = $_[1] : $_[0]->{'/'} }
sub position    { @_ > 1 ? $_[0]->{'@'} = $_[1] : $_[0]->{'@'} }
sub size        { @_ > 1 ? $_[0]->{'*'} = $_[1] : $_[0]->{'*'} }
sub hash        { @_ > 1 ? $_[0]->{'#'} = $_[1] : $_[0]->{'#'} }
sub action      { @_ > 1 ? $_[0]->{'a'} = $_[1] : $_[0]->{'a'} }
sub type        { @_ > 1 ? $_[0]->{'t'} = $_[1] : $_[0]->{'t'} }

sub ctime       { @_ > 1 ? $_[0]->{'c'} = $_[1] : $_[0]->{'c'} }
sub dev         { @_ > 1 ? $_[0]->{'d'} = $_[1] : $_[0]->{'d'} }
sub gid         { @_ > 1 ? $_[0]->{'g'} = $_[1] : $_[0]->{'g'} }
sub ino         { @_ > 1 ? $_[0]->{'i'} = $_[1] : $_[0]->{'i'} }
sub mtime       { @_ > 1 ? $_[0]->{'m'} = $_[1] : $_[0]->{'m'} }
sub nlink       { @_ > 1 ? $_[0]->{'n'} = $_[1] : $_[0]->{'n'} }
sub mode        { @_ > 1 ? $_[0]->{'p'} = $_[1] : $_[0]->{'p'} } # XXX
sub perm        { @_ > 1 ? $_[0]->{'p'} = $_[1] : $_[0]->{'p'} } # XXX
sub fmt         { @_ > 1 ? $_[0]->{'p'} = $_[1] : $_[0]->{'p'} } # XXX
sub rdev        { @_ > 1 ? $_[0]->{'r'} = $_[1] : $_[0]->{'r'} }
sub uid         { @_ > 1 ? $_[0]->{'u'} = $_[1] : $_[0]->{'u'} }

sub meta {
    my $self = shift;
    my $type = $self->{'t'};
    my $meta = sprintf('%s d%d i%d n%d p%o u%d g%d m%d c%d',
        @$self{qw(t d i n p u g m c)});
    $meta .= sprintf(' r%d', $self->{'r'}) if $type eq 'b' || $type eq 'c';
    $meta .= sprintf(' *%d', $self->{'*'} || 0) if $type eq 'f' || $type eq 'l';
    return $meta . ' ' . $self->{'/'};
}

1;

=pod

=head1 NAME

Archive::Eubar - read archives produced by eubar

=head2 SYNOPSIS

    $ar = Archive::Eubar->new($base);
    @files = $ar->files;
    $f = $ar->find($path);
    $ar->extract($f, \*STDOUT);

=head1 DESCRIPTION

Only reading is implemented, and even that is probably not working at this
point.

=head1 SEE ALSO

eubar(1)

=cut

