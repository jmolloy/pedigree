#!/usr/bin/perl

use strict;
use warnings;

my $gcc_version = "4.3.0";
my $binutils_version = "2.18";
my $nasm_version = "2.02";


my @download = ( {'url' => "ftp://ftp.gnu.org/gnu/gcc/gcc-$gcc_version/gcc-core-$gcc_version.tar.bz2",
                  'name' => 'Gcc core files',
                  'filename' => "gcc-core-$gcc_version.tar.bz2",
                  'extract' => "tar -xjf gcc-core-$gcc_version.tar.bz2",
                  'arch' => 'all'},
                 {'url' => "ftp://ftp.gnu.org/gnu/gcc/gcc-$gcc_version/gcc-g++-$gcc_version.tar.bz2",
                  'name' => 'G++',
                  'filename' => "gcc-g++-$gcc_version.tar.bz2",
                  'extract' => "tar -xjf gcc-g++-$gcc_version.tar.bz2",
                  'arch' => 'all'},
                 {'url' => "ftp://ftp.gnu.org/gnu/binutils/binutils-$binutils_version.tar.bz2",
                  'name' => 'Binutils',
                  'filename' => "binutils-$binutils_version.tar.bz2",
                  'extract' => "tar -xjf binutils-$binutils_version.tar.bz2",
                  'arch' => 'all'},
                 {'url' => "http://downloads.sourceforge.net/nasm/nasm-$nasm_version.tar.gz",
                  'name' => 'Nasm',
                  'filename' => "nasm-$nasm_version.tar.gz",
                  'extract' => "tar -xzf nasm-$nasm_version.tar.gz",
                  'arch' => 'i686-elf amd64-elf'} );

my @patch = ( {'cwd' => "gcc-$gcc_version",
               'name' => 'Dwarf extra info',
               'flags' => '-p1',
               'input' => 'gcc_dwarf.patch',
               'arch' => 'ppc-elf amd64-elf mips64el-elf i686-elf'},
              {'cwd' => "gcc-$gcc_version",
               'name' => 'Amd64',
               'flags' => '-p0',
               'input' => 'gcc_amd64.patch',
               'arch' => 'amd64-elf'},
              {'cwd' => "binutils-$binutils_version",
               'name' => 'Makeinfo',
               'flags' => '',
               'input' => 'binutils-2.18-makeinfo.patch',
               'arch' => 'all'} );

my @compile = ( {'dir' => "nasm-$nasm_version",
                 'inplace' => 1, # Nasm should be build inside the source tree.
                 'name' => "Nasm",
                 'configure' => "--prefix=\$PREFIX",
                 'make' => "",
                 'install' => 'install',
                 'arch' => 'i686-elf amd64-elf',
                 'test' => './bin/nasm' }, 
		{'dir' => "binutils-$binutils_version",
                 'name' => "Binutils",
                 'configure' => "--target=\$TARGET --prefix=\$PREFIX --disable-nls",
                 'make' => "all",
                 'install' => "install",
                 'arch' => 'all',
                 'test' => './bin/!TARGET-objdump'},
                {'dir' => "gcc-$gcc_version",
                 'name' => "Gcc",
                 'configure' => "--target=\$TARGET --prefix=\$PREFIX --disable-nls --enable-languages=c,c++ --without-headers --without-newlib",
                 'make' => "all-gcc all-target-libgcc",
                 'install' => "install-gcc install-target-libgcc",
                 'arch' => 'i686-elf amd64-elf arm-elf ppc-elf',
                 'test' => './bin/!TARGET-gcc'},
                {'dir' => "gcc-$gcc_version",
                 'name' => "Gcc (mips)",
                 'configure' => "--target=\$TARGET --prefix=\$PREFIX --disable-nls --enable-languages=c,c++ --without-headers --without-newlib --with-llsc=yes",
                 'make' => "all-gcc all-target-libgcc",
                 'install' => "install-gcc install-target-libgcc",
                 'arch' => 'mips64el-elf',
                 'test' => './bin/!TARGET-gcc'} );

###################################################################################
# Script start.

die ("No target given!") unless scalar @ARGV > 0;

my $target = $ARGV[0];
$ENV{CC} = "";
$ENV{CXX} = "";
$ENV{AS} = "";
$ENV{CPP} = "";
$ENV{CFLAGS} = "";
$ENV{CXXFLAGS} = "";
$ENV{LDFLAGS} = "";
$ENV{ASFLAGS} = "";

my $prefix = `pwd`;
chomp $prefix;

# Firstly, find out where to store the compilers.
unless (-l "./compilers/dir") {
  print "This appears to be the first time you've compiled this checkout. Where should I look for / store my compilers?\n";
  my $dir = <STDIN>;
  chomp $dir;
 `mkdir -p $dir`;
  my $stdout = `ln -s $dir ./compilers/dir`;
  if (length $stdout) {
    print "That directory wasn't valid.\n";
    exit 1;
  }
}

# Are there any compile targets to make?
my $all_installed = 1;
foreach (@compile) {
  my %compile = %$_;

  if ($compile{arch} =~ m/($target)|(all)/i) {
    # Already installed?
    my $str = "./compilers/dir/$compile{test}";
    $str =~ s/!TARGET/$target/;
    open my $fd, ">", "/tmp/cunt";
    print $fd "Fuck a doodle doo : $str\n";
    close $fd;
    $all_installed = 0 unless (-f $str);
  }
}

exit 0 if $all_installed;

`mkdir -p ./compilers/dir/dl_cache`;
`mkdir -p ./compilers/dir/build_tmp`;

print "Downloading/extracting: ";
# Download everything we need to.
foreach (@download) {
  my %download = %$_;

  if ($download{arch} =~ m/($target)|(all)/i) {
    # Download applies to us.
    print "$download{name} ";
    unless (-f "./compilers/dir/dl_cache/$download{filename}") {
      `cd ./compilers/dir/dl_cache; wget $download{url} 2>&1`;
      if ($? != 0) {
        print "Failed (download).\n";
        exit 1;
      }
    }
    `cd ./compilers/dir/build_tmp; cp ../dl_cache/$download{filename} ./; $download{extract}`;
    if ($? != 0) {
      print "Failed (extract).\n";
      exit 1;
    }
  }
}
print "\n";

# Patch everything we need to.
print "Patching: ";
foreach (@patch) {
  my %patch = %$_;

  if ($patch{arch} =~ m/($target)|(all)/i) {
    print "$patch{name} ";
    my $stdout = `cd ./compilers/dir/build_tmp/$patch{cwd}; patch $patch{flags} < $prefix/compilers/$patch{input} 2>&1`;
    if ($? != 0) {
      print "\nFailed - output:\n$stdout";
      `rm -r ./compilers/dir/build_tmp`;
      exit 1;
    }
  }
}

print "\n";

# Compile everything we need to.
print "Compiling:\n";
foreach (@compile) {
  my %compile = %$_;

  if ($compile{arch} =~ m/($target)|(all)/i) {
    # Already installed?
    my $str = "./compilers/dir/$compile{test}";
    $str =~ s/!TARGET/$target/;
    if (-f $str) {
      print "    $compile{name}: Already installed.\n";
      next;
    }
    print "    $compile{name}: Configuring ";
    my $build_dir = "./compilers/dir/build_tmp/build";
    my $stdout = `cd ./compilers/dir/build_tmp/; mkdir -p build`;
    if (defined $compile{inplace} and $compile{inplace}) {
      $build_dir = "./compilers/dir/build_tmp/$compile{dir}";
    }
    $stdout = `export PREFIX=$prefix/compilers/dir; export TARGET=$target; cd $build_dir; ../$compile{dir}/configure $compile{configure} 2>&1`;
    if ($? != 0) {
      print "Failed. Output: $stdout\n";
      exit 1;
    }
    print "Compiling ";
    $stdout = `cd $build_dir; make $compile{make} 2>&1`;
    if ($? != 0) {
      print "Failed. Output: $stdout\n";
      exit 1;
    }
    print "Installing";
    $stdout = `cd $build_dir; make $compile{install} 2>&1`;
    if ($? != 0) {
      print "Failed. Output: $stdout\n";
      exit 1;
    }
    print "\n";
    `rm -rf ./compilers/dir/build_tmp/build`;
  }
}

print "Complete.\n";
`rm -rf ./compilers/build_tmp`;
exit 0;
