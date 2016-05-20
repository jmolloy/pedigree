#!/usr/bin/perl

use strict;
use warnings;

die ("No target given!") unless scalar @ARGV > 0;

my $target = $ARGV[0];

my $gcc_version = "4.8.2";
my $binutils_version = "2.24";
my $nasm_version = "2.11.02";

my $gcc_configure_special = " --disable-werror ";
my $binutils_configure_special = " --disable-werror ";

my $gcc_libcpp_make = "";
my $gcc_libcpp_install = "";

# Handle special arguments. These are given to change the behaviour of the script, or to
# work around issues with specific operating systems.
for(my $i = 2; $i < @ARGV; $i++)
{
    if($ARGV[$i] eq "osx-compat")
    {
        $gcc_configure_special .= " --with-gmp=/opt/local --with-libiconv-prefix=/opt/local ";
    }
    elsif($ARGV[$i] eq "libcpp")
    {
        $gcc_libcpp_make = "all-target-libstdc++-v3";
        $gcc_libcpp_install = "install-target-libstdc++-v3";
    }
}

my @download = ( {'url' => "ftp://ftp.gnu.org/gnu/gcc/gcc-$gcc_version/gcc-$gcc_version.tar.bz2",
                  'name' => 'GCC',
                  'filename' => "gcc-$gcc_version.tar.bz2",
                  'extract' => "tar -xjf gcc-$gcc_version.tar.bz2",
                  'arch' => 'all'},
                 {'url' => "ftp://ftp.gnu.org/gnu/binutils/binutils-$binutils_version.tar.bz2",
                  'name' => 'Binutils',
                  'filename' => "binutils-$binutils_version.tar.bz2",
                  'extract' => "tar -xjf binutils-$binutils_version.tar.bz2",
                  'arch' => 'all'},
                 {'url' => "http://www.nasm.us/pub/nasm/releasebuilds/$nasm_version/nasm-$nasm_version.tar.bz2",
                  'name' => 'Nasm',
                  'filename' => "nasm-$nasm_version.tar.bz2",
                  'extract' => "tar -xjf nasm-$nasm_version.tar.bz2",
                  'arch' => 'i686-pedigree x86_64-pedigree amd64-pedigree i686-elf amd64-elf'} );

my @command = ( {'cwd' => "gcc-$gcc_version",
                 'name' => "fixing autoconf version dependency",
                 'cmd' => "autoconf -V | grep autoconf | tr ' ' '\n' | tail -1 | xargs printf -- '-i.bak \"s/2.64/\%s/g\" ./config/override.m4' | xargs sed",
                 'arch' => 'all'},
                {'cwd' => "gcc-$gcc_version",
                 'name' => "libstdc++ crossconfig",
                 'cmd' => "autoreconf --force",
                 'arch' => 'all'} );

my @patch = ( {'cwd' => "gcc-$gcc_version",
               'name' => "Gcc pedigree target patch",
               'flags' => '-p1',
               'input' => 'pedigree-gcc.patch',
               'arch' => 'all'},
              {'cwd' => "binutils-$binutils_version",
               'name' => "Binutils pedigree target patch",
               'flags' => '-p1',
               'input' => 'pedigree-binutils.patch',
               'arch' => 'all'} );

my @compile = ( {'dir' => "nasm-$nasm_version",
                 'inplace' => 1, # Nasm should be built inside the source tree.
                 'name' => "Nasm",
                 'configure' => "--prefix=\$PREFIX",
                 'make' => "",
                 'install' => 'install',
                 'arch' => 'i686-pedigree x86_64-pedigree amd64-pedigree i686-elf amd64-elf',
                 'test' => './bin/nasm' },
                {'dir' => "binutils-$binutils_version",
                 'name' => "Binutils",
                 'configure' => "--target=\$TARGET $binutils_configure_special --prefix=\$PREFIX --disable-nls",
                 'make' => "all",
                 'install' => "install",
                 'arch' => 'all',
                 'test' => './bin/!TARGET-objdump'},
                {'dir' => "gcc-$gcc_version",
                 'name' => "Gcc",
                 'configure' => "--target=\$TARGET $gcc_configure_special --prefix=\$PREFIX --disable-nls --enable-languages=c,c++ --without-headers --without-newlib",
                 'make' => "all-gcc all-target-libgcc",
                 'install' => "install-gcc install-target-libgcc",
                 'arch' => 'i686-pedigree amd64-pedigree x86_64-pedigree arm-pedigree i686-elf amd64-elf arm-elf ppc-elf powerpc-elf',
                 'test' => './bin/!TARGET-gcc',
                 'clean' => $gcc_libcpp_make eq ""},
                {'dir' => "gcc-$gcc_version",
                 'ok' => $gcc_libcpp_make ne "",
                 'name' => "libstdc++",
                 'configure' => "--target=\$TARGET $gcc_configure_special --prefix=\$PREFIX --disable-nls --enable-languages=c++ --with-newlib --disable-libstdcxx-pch --enable-shared",
                 'make' => "$gcc_libcpp_make",
                 'install' => "$gcc_libcpp_install",
                 'arch' => 'i686-pedigree amd64-pedigree x86_64-pedigree arm-pedigree i686-elf amd64-elf arm-elf ppc-elf powerpc-elf',
                 'test' => './!TARGET/lib/libstdc++.a'},
                {'dir' => "gcc-$gcc_version",
                 'name' => "Gcc (mips)",
                 'configure' => "--target=\$TARGET $gcc_configure_special --prefix=\$PREFIX --disable-nls --enable-languages=c,c++ --without-headers --without-newlib --with-llsc=yes",
                 'make' => "all-gcc all-target-libgcc",
                 'install' => "install-gcc install-target-libgcc",
                 'arch' => 'mips64el-elf',
                 'test' => './bin/!TARGET-gcc'} );

###################################################################################
# Script start.

$ENV{CC} = "";
$ENV{CXX} = "";
$ENV{AS} = "";
$ENV{CPP} = "";
$ENV{CFLAGS} = "";
$ENV{CXXFLAGS} = "";
$ENV{LDFLAGS} = "";
$ENV{ASFLAGS} = "";

my $dir = $ARGV[1];

my $prefix = `pwd`;
chomp $prefix;

die "Please use target '[arch]-pedigree'." unless $target =~ /(i686|x86_64|arm|amd64|mips64el|powerpc)/; # '*-pedigree';

# Firstly, find out where to store the compilers.
unless (-l "./compilers/dir") {
  print "This appears to be the first time you've compiled this checkout. Where should I look for / store my compilers?\n";
  print "<not interactive, using $dir>\n";
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

  next if (defined $compile{ok} and !$compile{ok});

  if ($compile{arch} =~ m/($target)|(all)/i) {
    # Already installed?
    my $str = "./compilers/dir/$compile{test}";
    $str =~ s/!TARGET/$target/;
    $all_installed = 0 unless (-f $str);
  }
}

goto SYMLINKS if $all_installed;

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
      my $stdout = `cd ./compilers/dir/dl_cache; wget $download{url} 2>&1`;
      if ($? != 0) {
        print "Failed (download).\n";
        print $stdout;
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

# Run everything we need to.
print "Running: ";
foreach (@command) {
  my %command = %$_;

  if ($command{arch} =~ m/($target)|(all)/i) {
    print "$command{name} ";
    my $stdout = `cd ./compilers/dir/build_tmp/$command{cwd}; $command{cmd} 2>&1`;
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

  next if (defined $compile{ok} and !$compile{ok});

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
    $stdout = `cd $build_dir; make $compile{make} 2>&1 & pid=\$!; while kill -0 \$pid >/dev/null 2>&1; do printf "." 1>&2; sleep 10; done`;
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

    # Only clean out the build directory if required (helps with libcpp build).
    if ((!defined $compile{clean}) || (defined $compile{clean} and $compile{clean})) {
      `rm -rf ./compilers/dir/build_tmp/build`;
    }
  }
}

SYMLINKS:

print "Complete; linking crt*.o...\n";

`ln -s $prefix/build/kernel/crt0.o ./compilers/dir/lib/gcc/$target/$gcc_version/crt0.o`;
`ln -s $prefix/build/kernel/crti.o ./compilers/dir/lib/gcc/$target/$gcc_version/crti.o`;
`ln -s $prefix/build/kernel/crtn.o ./compilers/dir/lib/gcc/$target/$gcc_version/crtn.o`;
`ln -s $prefix/src/subsys/posix/include ./compilers/dir/$target/include`;
print "Done.\n";

`rm -rf ./compilers/dir/build_tmp`;
exit 0;
