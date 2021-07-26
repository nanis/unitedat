#!/usr/bin/env perl

use strict;
use warnings;

use Carp qw( croak );
use Capture::Tiny qw( capture );
use Path::Tiny qw( path );
use Test2::Bundle::More;

my $program_name = 'unitedat' . (($^O eq 'MSWin32') ? '.exe' : '');
my $program_path = path(__FILE__)->parent->parent->child($program_name)->absolute;
my @test_files = map path(__FILE__)->parent->child($_), qw(file1.csv file2.csv);
my %test_content;

sub canonical_contents {
    $_[0] =~ s/\R/\n/gr;
}

sub number_of_lines {
    scalar($_[0] =~ tr/\n/\n/);
}

for my $file (@test_files) {
    $test_content{$file} = canonical_contents $file->slurp;
    note $test_content{$file};
}

sub contents_without_header {
    my $file = shift;
    my $n = shift // 0;
    my @contents = split /\R/, $test_content{$file};
    join("\n", @contents[$n .. $#contents], '');
}

sub test_no_files {
    my @cmd = ($program_path);
    note "@cmd";
    my ($out, $err, $status) = capture { system @cmd };
    isnt $status, 0, 'Exit code indicates failure';
    like $err, qr/^No files/, 'Error message mentions there were no files';
    is length($out), 0, 'Running with no file arguments produces no output';
}

sub test_no_files_with_n {
    my @cmd = ($program_path => '-n', '0');
    note "@cmd";
    my ($out, $err, $status) = capture { system @cmd };
    isnt $status, 0, 'Exit code indicates failure';
    like $err, qr/^No files/, 'Error message mentions there were no files';
    is length($out), 0, 'Running with no file arguments produces no output';
}

sub test_no_files_with_header_lines {
    my @cmd = ($program_path => '--header-lines', '0');
    note "@cmd";
    my ($out, $err, $status) = capture { system @cmd };
    isnt $status, 0, 'Exit code indicates failure';
    like $err, qr/^No files/, 'Error message mentions there were no files';
    is length($out), 0, 'Running with no file arguments produces no output';
}

sub test_all_files_with_no_header_lines {
    my @cmd = ($program_path => '-n', '0', @test_files);
    note "@cmd";
    my ($out, $err, $status) = capture { system @cmd };
    is $status, 0, 'Executes successfully';
    is length($err), 0, 'Produces no error messages';
    my $expected = join '', @test_content{@test_files};
    is $out, $expected, 'Outputs the full contents of all files';
}

sub test_all_files_with_one_header_line {
    my $header_lines = 1;
    my @cmd = ($program_path => @test_files);
    note "@cmd";

    my ($out, $err, $status) = capture { system @cmd };
    is $status, 0, 'Executes successfully';
    is length($err), 0, 'Produces no error messages';

    my $expected = contents_without_header($test_files[0]) . join('',
        map contents_without_header($_, $header_lines), @test_files[1 .. $#test_files],
    );

    is canonical_contents($out), $expected, 'Skips one line of header after the first file';
}

sub test_all_files_with_one_short_file {
    my $header_lines = number_of_lines($test_content{$test_files[0]});
    my @cmd = ($program_path => '-n', $header_lines, @test_files);
    note "@cmd";
    my ($out, $err, $status) = capture { system @cmd };
    isnt $status, 0, 'Fails';
    is number_of_lines($out), $header_lines, 'Has to output the first file before being able to detect the second file is too short';
    like $err, qr/no output/, 'Detects that when a file has too little content';
}

sub test_all_files_with_too_many_header_lines {
    my $header_lines = 1 + number_of_lines($test_content{$test_files[0]});
    my @cmd = ($program_path => '-n', $header_lines, @test_files);
    note "@cmd";
    my ($out, $err, $status) = capture { system @cmd };
    isnt $status, 0, 'Fails';
    is length($out), 0, 'Produces no output';
    like $err, qr/Ran out of input/, "Detects when it can't figure out data start offset";
}

test_no_files;
test_no_files_with_n;
test_no_files_with_header_lines;
test_all_files_with_no_header_lines;
test_all_files_with_one_header_line;
test_all_files_with_one_short_file;
test_all_files_with_too_many_header_lines;

done_testing;
