%hash;

$lineno = 0;

while (<>) {
    $i = 2;
    $foo = $_;
    chomp($foo);
    if ($foo eq 'Unknown') {
	$foo = sprintf("$foo%03d", $lineno);
    }
    while ($hash{$foo}) {
	$foo = $_;
	chomp($foo);
	$foo = $foo . $i++;
    }
    $hash{$foo} = 1;

    print "$foo\n";
    $lineno++;
}
