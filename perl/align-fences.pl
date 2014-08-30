#!/usr/bin/perl
package align_fences;
no warnings "experimental::lexical_subs";
use feature qw(lexical_subs say);
use Imager;

our sub parse_pngspec {
    my ($self, $dpath) = @_;

    $dpath =~ s/> *//;

    if ($dpath =~ /^(.*)\.(\d+)\.(\d+)(?:,(-?\d+))?(?:,(-?\d+))?$/) {
	my ($basepath, $yn, $xn, $xoff, $yoff) = ($1, $2, $3, $4, $5);

	return ($basepath, $yn, $xn, $xoff, $yoff);
    }
}

our sub transparentify {
    my ($self, $img) = @_;

    my $otrans = Imager::Color->new(231, 255, 255, 255);
    my $ntrans = Imager::Color->new(0, 0, 0, 0);
    for my $x (0..$img->getwidth-1) {
	for my $y (0..$img->getheight-1) {
	    my ($r,$g,$b) = $img->getpixel(x=>$x, y=>$y)->rgba();

	    if ($r == 231 and $g == 255 and $b == 255) {
		$img->setpixel(x=>$x, y=>$y, color=>$ntrans);
	    }
	}
    }
}

our sub detransparentify {
    my ($self, $img) = @_;

    my $otrans = Imager::Color->new(231, 255, 255, 255);
    my $ntrans = Imager::Color->new(0, 0, 0, 0);
    for my $x (0..$img->getwidth-1) {
	for my $y (0..$img->getheight-1) {
	    my ($r,$g,$b,$a) = $img->getpixel(x=>$x, y=>$y)->rgba();

	    if ($a == 0) {
		$img->setpixel(x=>$x, y=>$y, color=>$otrans);
	    }
	}
    }
}

our sub pngspec2image {
    my ($self, $pngspec) = @_;

    my ($basepath, $yn, $xn, $xoff, $yoff) = $self->parse_pngspec($pngspec);

    my $img = Imager->new(file=>$basepath.".png");
    die "$pngspec -> $basepath" unless $img;
    $img = $img->convert(preset=>"addalpha");

    $img = $img->crop(left=>$xn * $self->{resolution}, top=>$yn * $self->{resolution},
		      width=>$self->{resolution}, height=>$self->{resolution});

    warn "$xn $yn $xoff $yoff";

    $self->transparentify($img);

    return ($img,$xoff,$yoff);
}

sub assemble {
    my ($self, $pngspec) = @_;

    my ($inimg, $xoff, $yoff) = $self->pngspec2image($pngspec);
    if ($xoff == 32) {
	$self->{outimg}->compose(src=>$inimg, tx=>$xoff+32, ty=>$yoff, src_minx=>32);
    } else {
	$self->{outimg}->compose(src=>$inimg, tx=>$xoff, ty=>$yoff);
    }
}

sub get_img {
    my ($self) = @_;
    $self->detransparentify($self->{outimg});
    return $self->{outimg};
}

sub new {
    my ($class, $resolution) = @_;

    return bless { resolution => $resolution, outimg => Imager->new(xsize=>$resolution, ysize=>$resolution,channels=>4), }, $class;
}

package main;

my $resolution = 128;
for my $basename ("palissade", "grillage", "mur_beton", "mur_beton_trame", "muret_beton", "muret_briques", "plexi") {
    next;
    my $outimg = Imager->new(xsize=>$resolution*4, ysize=>$resolution*12,channels=>3);
    for my $i (0..15) {
	my $x = ($i&3);
	my $y = int($i/4);
	my $af = align_fences->new($resolution);

	$af->assemble("$basename.0.4,30,-15") if ($i & 1);
	$af->assemble("$basename.0.5,-30,-15") if ($i & 8);

	$outimg->compose(src=>$af->get_img(), tx=>$x*$resolution, ty=>$y*$resolution);
    }

    for my $i (0..15) {
	my $x = ($i&3);
	my $y = int($i/4)+4;
	my $af = align_fences->new($resolution);

	$af->assemble("$basename.0.4,-30,15") if ($i & 4);
	$af->assemble("$basename.0.5,30,15") if ($i & 2);

	$outimg->compose(src=>$af->get_img(), tx=>$x*$resolution, ty=>$y*$resolution);
    }

    for my $i (0..3) {
	my $x = $i;
	my $y = 8;

	my $af = align_fences->new($resolution);

	$af->assemble("$basename.3.4,30,-15") if ($i & 1);
	$af->assemble("$basename.3.4,-30,-15") if ($i & 2);

	$outimg->compose(src=>$af->get_img(), tx=>$x*$resolution, ty=>$y*$resolution);
    }

    for my $i (0..3) {
	my $x = $i;
	my $y = 10;

	my $af = align_fences->new($resolution);

	$af->assemble("$basename.3.5,30,-15") if ($i & 1);
	$af->assemble("$basename.3.5,-30,-15") if ($i & 2);

	$outimg->compose(src=>$af->get_img(), tx=>$x*$resolution, ty=>$y*$resolution);
    }

    $outimg->write(file=>"out-$basename.png");
}

sub assemble {
    my ($l, $front, $outimg, $yoff) = @_;
    my @l = @$l;

    for my $i (0..3) {
	my $x = ($i&3);
	my $y = int($i/4) + $yoff;
	my $af = align_fences->new($resolution);

	$af->assemble($l->[0]) if ($i&1); # ($i & 8);
	$af->assemble($l->[1]) if ($i&2); # ($i & 1);

	$outimg->compose(src=>$af->get_img(), tx=>$x*$resolution, ty=>$y*$resolution);
    }

    for my $i (0..3) {
	my $x = ($i&3);
	my $y = int($i/4)+1 + $yoff;
	my $af = align_fences->new($resolution);

	$af->assemble($l->[3]) if ($i&2); # ($i & 4);
	$af->assemble($l->[2]) if ($i&1); # ($i & 2);

	$outimg->compose(src=>$af->get_img(), tx=>$x*$resolution, ty=>$y*$resolution);
    }
}

for my $basename ("plexi", "plexinoground") {
    my $outimg = Imager->new(xsize=>$resolution*4, ysize=>$resolution*18,channels=>3);

    my $l = [
	"$basename.0.1,-32,-16",
	"$basename.0.0,32,-16",
	"$basename.0.1,32,16",
	"$basename.0.0,-32,16",
	];

    assemble($l, 0, $outimg, 0);

    $l = [
	"$basename.3.4,-32,-16",
	"$basename.0.0,32,-48",
	"$basename.3.4,32,16",
	"$basename.0.0,-32,16",
	];

    assemble($l, 0, $outimg, 2);

    $l = [
	"$basename.0.1,-32,-48",
	"$basename.3.5,32,-16",
	"$basename.0.1,32,16",
	"$basename.3.5,-32,16",
	];

    assemble($l, 0, $outimg, 4);
    $l = [
	"$basename.2.2,-32,-8",
	"$basename.0.0,32,-32",
	"$basename.2.2,32,24",
	"$basename.0.0,-32,16",
	];

    assemble($l, 0, $outimg, 6);

    $l = [
	"$basename.0.1,-32,-32",
	"$basename.2.3,32,-8",
	"$basename.0.1,32,16",
	"$basename.2.3,-32,24",
	];

    assemble($l, 0, $outimg, 8);

    $l = [
	"$basename.0.1,-32,-16",
	"$basename.3.2,32,-16",
	"$basename.0.1,32,-16",
	"$basename.3.2,-32,16",
	];

    assemble($l, 0, $outimg, 10);

    $l = [
	"$basename.3.3,-32,-16",
	"$basename.0.0,32,-16",
	"$basename.3.3,32,16",
	"$basename.0.0,-32,-16",
	];

    assemble($l, 0, $outimg, 12);

    $l = [
	"$basename.0.1,-32,-16",
	"$basename.0.4,32,-8",
	"$basename.0.1,32,0",
	"$basename.0.4,-32,24",
	];

    assemble($l, 0, $outimg, 14);

    $l = [
	"$basename.0.5,-32,-8",
	"$basename.0.0,32,-16",
	"$basename.0.5,32,24",
	"$basename.0.0,-32,0",
	];

    assemble($l, 0, $outimg, 16);

    $outimg->write(file=>"out-$basename.png");
    dispatch($basename);
}

for my $basename ("palissade") {
    my $outimg = Imager->new(xsize=>$resolution*4, ysize=>$resolution*18,channels=>3);

    my $l = [
	"$basename.0.1,-32,-16",
	"$basename.0.0,32,-16",
	"$basename.0.1,32,16",
	"$basename.0.0,-32,16",
	];

    assemble($l, 0, $outimg, 0);

    $l = [
	"$basename.3.4,-32,-16",
	"$basename.0.0,32,-48",
	"$basename.3.4,32,16",
	"$basename.0.0,-32,16",
	];

    assemble($l, 0, $outimg, 2);

    $l = [
	"$basename.0.1,-32,-48",
	"$basename.3.5,32,-16",
	"$basename.0.1,32,16",
	"$basename.3.5,-32,16",
	];

    assemble($l, 0, $outimg, 4);
    $l = [
	"$basename.0.2,-32,-8",
	"$basename.0.0,32,-32",
	"$basename.0.2,32,24",
	"$basename.0.0,-32,16",
	];

    assemble($l, 0, $outimg, 6);

    $l = [
	"$basename.0.1,-32,-32",
	"$basename.0.3,32,-8",
	"$basename.0.1,32,16",
	"$basename.0.3,-32,24",
	];

    assemble($l, 0, $outimg, 8);

    $l = [
	"$basename.0.1,-32,-16",
	"$basename.3.2,32,-16",
	"$basename.0.1,32,-16",
	"$basename.3.2,-32,16",
	];

    assemble($l, 0, $outimg, 10);

    $l = [
	"$basename.3.3,-32,-16",
	"$basename.0.0,32,-16",
	"$basename.3.3,32,16",
	"$basename.0.0,-32,-16",
	];

    assemble($l, 0, $outimg, 12);

    $l = [
	"$basename.0.1,-32,-16",
	"$basename.0.4,32,-8",
	"$basename.0.1,32,0",
	"$basename.0.4,-32,24",
	];

    assemble($l, 0, $outimg, 14);

    $l = [
	"$basename.0.5,-32,-8",
	"$basename.0.0,32,-16",
	"$basename.0.5,32,24",
	"$basename.0.0,-32,0",
	];

    assemble($l, 0, $outimg, 16);

    $outimg->write(file=>"out-$basename.png");
    dispatch($basename);
}

for my $basename ("blockwall") {
    my $outimg = Imager->new(xsize=>$resolution*4, ysize=>$resolution*18,channels=>3);

    my $l = [
	"$basename.0.0,-32,-16",
	"$basename.0.1,32,-16",
	"$basename.0.0,32,16",
	"$basename.0.1,-32,16",
	];

    assemble($l, 0, $outimg, 0);

    $l = [
	"$basename.3.0,-32,-16",
	"$basename.0.1,32,-48",
	"$basename.3.0,32,16",
	"$basename.0.1,-32,16",
	];

    assemble($l, 0, $outimg, 2);

    $l = [
	"$basename.0.0,-32,-48",
	"$basename.3.1,32,-16",
	"$basename.0.0,32,16",
	"$basename.3.1,-32,16",
	];

    assemble($l, 0, $outimg, 4);
    $l = [
	"$basename.3.2,-32,-8",
	"$basename.0.1,32,-32",
	"$basename.3.2,32,24",
	"$basename.0.1,-32,16",
	];

    assemble($l, 0, $outimg, 6);

    $l = [
	"$basename.0.0,-32,-32",
	"$basename.3.3,32,-8",
	"$basename.0.0,32,16",
	"$basename.3.3,-32,24",
	];

    assemble($l, 0, $outimg, 8);

    $l = [
	"$basename.0.0,-32,-16",
	"$basename.4.1,32,-16",
	"$basename.0.0,32,-16",
	"$basename.4.1,-32,16",
	];

    assemble($l, 0, $outimg, 10);

    $l = [
	"$basename.4.1,-32,-16",
	"$basename.0.1,32,-16",
	"$basename.4.1,32,16",
	"$basename.0.1,-32,-16",
	];

    assemble($l, 0, $outimg, 12);

    $l = [
	"$basename.0.0,-32,-16",
	"$basename.5.2,32,-8",
	"$basename.0.0,32,0",
	"$basename.5.2,-32,24",
	];

    assemble($l, 0, $outimg, 14);

    $l = [
	"$basename.5.3,-32,-8",
	"$basename.0.1,32,-16",
	"$basename.5.3,32,24",
	"$basename.0.1,-32,0",
	];

    assemble($l, 0, $outimg, 16);

    $outimg->write(file=>"out-$basename.png");
    dispatch($basename);
}

sub dispatch {
    my ($basename) = @_;
print <<EOF;
Obj=way-object
Name=NoiseProtectionWall$basename
copyright=James/Timothy Baldock
cost=90000
maintenance=400
topspeed=115
intro_year=1931
intro_month=5
waytype=invalid
own_waytype=road
cursor=blockwall.5.1
icon=> blockwall.5.0
EOF

    for my $i (0..15) {
	my $str = ((($i&1) ? "N" : "") .
		   (($i&4) ? "S" : "") .
		   (($i&2) ? "E" : "") .
		   (($i&8) ? "W" : ""));
	if ($str eq "") {
	    $str = "-";
	}

	print "is_fence = 1\n";

	print "BackImage[$str]=out-$basename." . 0 . "." . ((($i&8)?1:0) + (($i&1)?2:0)) . "\n";
	print "FrontImage[$str]=out-$basename." . 1 . "." . ((($i&2)?1:0) + (($i&4)?2:0)) . "\n";

	print "BackImageUpN[$str]=out-$basename." . 16 . "." . ((($i&8)?1:0) + (($i&1)?2:0)) . "\n";
	print "FrontImageUpN[$str]=out-$basename." . 17 . "." . ((($i&2)?1:0) + (($i&4)?2:0)) . "\n";

	print "BackImageUpS[$str]=out-$basename." . 6 . "." . ((($i&8)?1:0) + (($i&1)?2:0)) . "\n";
	print "FrontImageUpS[$str]=out-$basename." . 7 . "." . ((($i&2)?1:0) + (($i&4)?2:0)) . "\n";

	print "BackImageUpE[$str]=out-$basename." . 8 . "." . ((($i&8)?1:0) + (($i&1)?2:0)) . "\n";
	print "FrontImageUpE[$str]=out-$basename." . 9 . "." . ((($i&2)?1:0) + (($i&4)?2:0)) . "\n";

	print "BackImageUpW[$str]=out-$basename." . 14 . "." . ((($i&8)?1:0) + (($i&1)?2:0)) . "\n";
	print "FrontImageUpW[$str]=out-$basename." . 15 . "." . ((($i&2)?1:0) + (($i&4)?2:0)) . "\n";

	print "BackImageUp2N[$str]=out-$basename." . 12 . "." . ((($i&8)?1:0) + (($i&1)?2:0)) . "\n";
	print "FrontImageUp2N[$str]=out-$basename." . 13 . "." . ((($i&2)?1:0) + (($i&4)?2:0)) . "\n";

	print "BackImageUp2E[$str]=out-$basename." . 4 . "." . ((($i&8)?1:0) + (($i&1)?2:0)) . "\n";
	print "FrontImageUp2E[$str]=out-$basename." . 5 . "." . ((($i&2)?1:0) + (($i&4)?2:0)) . "\n";

	print "BackImageUp2S[$str]=out-$basename." . 2 . "." . ((($i&8)?1:0) + (($i&1)?2:0)) . "\n";
	print "FrontImageUp2S[$str]=out-$basename." . 3 . "." . ((($i&2)?1:0) + (($i&4)?2:0)) . "\n";

	print "BackImageUp2W[$str]=out-$basename." . 10 . "." . ((($i&8)?1:0) + (($i&1)?2:0)) . "\n";
	print "FrontImageUp2W[$str]=out-$basename." . 11 . "." . ((($i&2)?1:0) + (($i&4)?2:0)) . "\n";

    }
    print "-------\n";
}
