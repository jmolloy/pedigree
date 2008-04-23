#!/usr/bin/perl
#######################################################################
# checkBuildSystem.pl - a script to check for and possibly install
#                       gcc/binutils in the build/ directory.

my $COMPILER_VERSION = "4.2.2";
my $BINUTILS_VERSION = "2.18";
my $FTP_DIR = "ftp://ftp.gnu.org/gnu/gcc/gcc-$COMPILER_VERSION";
my $BINUTILS_FTP_DIR = "ftp://ftp.gnu.org/gnu/binutils";
my $NASM_VERSION = "2.02";
my $NASM_DOWNLOAD = "http://downloads.sourceforge.net/nasm/nasm-$NASM_VERSION.tar.gz";
my $delete_cache = 0;

# Is the compiler with the given version string (e.g. i686-elf) installed?
sub is_installed {
  my ($arch) = @_;

  return -d "./compilers/$arch";
}

sub is_nasm_installed {
  my ($arch) = @_;
  
  if ($arch =~ m/(i686|amd64)-elf/i) {
    return -f "./compilers/bin/nasm";
  } else {
    return 1;
  }
}

# Do we have a compiler download cache?
sub is_cached {
  return 1 if (-d "./compilers/dl_cache" and
               -f "./compilers/dl_cache/gcc-core-$COMPILER_VERSION.tar.bz2" and
               -f "./compilers/dl_cache/gcc-g++-$COMPILER_VERSION.tar.bz2" and
               -f "./compilers/dl_cache/binutils-$BINUTILS_VERSION.tar.bz2");
  return 0;
}

# Download the compiler.
sub download {
  my ($arch) = @_;
  
  `mkdir ./compilers/dl_cache` unless (-d "./compilers/dl_cache");

  my $already_cached = 0;

  if (-f "./compilers/dl_cache/gcc-core-$COMPILER_VERSION.tar.bz2") {
    print "\e[32mGcc core files already downloaded, using cached version.\e[0m\n";
    $already_cached = 1;
  } else {
    print "\e[32mDownloading gcc core files, version $COMPILER_VERSION...\e[0m\n";  
    `cd ./compilers/dl_cache; wget $FTP_DIR/gcc-core-$COMPILER_VERSION.tar.bz2`;
    if ($? != 0) {
      print "\e[31mError: gcc core files failed to download!\e[0m\n";
      return 1;
    }
  }

  if (-f "./compilers/dl_cache/gcc-g++-$COMPILER_VERSION.tar.bz2") { 
    print "\e[32mG++ files already downloaded, using cached version.\e[0m\n";
    $already_cached = 1;
  } else {
    print "\e[32mDownloading g++, version $COMPILER_VERSION...\e[0m\n";
    `cd ./compilers/dl_cache; wget $FTP_DIR/gcc-g++-$COMPILER_VERSION.tar.bz2`;
    if ($? != 0) {
      print "\e[31mError: g++ files failed to download!\e[0m\n";
      return 1;
    }
  }

  if (-f "./compilers/dl_cache/binutils-$BINUTILS_VERSION.tar.bz2") { 
    print "\e[32mBinutils files already downloaded, using cached version.\e[0m\n";
    $already_cached = 1;
  } else {
    print "\e[32mDownloading binutils, version $BINUTILS_VERSION...\e[0m\n";
    `cd ./compilers/dl_cache; wget $BINUTILS_FTP_DIR/binutils-$BINUTILS_VERSION.tar.bz2`;
    if ($? != 0) {
      print "\e[31mError: binutils files failed to download!\e[0m\n";
      return 1;
    }
  }

  if ($already_cached == 0) {
    # Ask the user if he wants to keep the cached files.
    print "\e[32mThe main compiler tarballs can be kept on disk, in case a new compiler build is needed. This will mean that new compiler builds will be faster, but will consume roughly 50MB of disk space.\nCache compiler files? [yes]\e[0m: ";
    $delete_cache = 1 if (<STDIN> =~ m/n/i);
  }
  return 0;
}

sub download_nasm {
  `mkdir ./compilers/dl_cache` unless (-d "./compilers/dl_cache");

  if (-f "./compilers/dl_cache/nasm-$NASM_VERSION.tar.gz") {
    print "\e[32mNASM files already downloaded, using cached version.\e[0m\n";
  } else {
    print "\e[32mDownloading NASM, version $NASM_VERSION...\e[0m\n";
    `cd ./compilers/dl_cache; wget $NASM_DOWNLOAD`;
    if ($? != 0) {
      print "\e[31mError: NASM files failed to download!\e[0m\n";
      return 1;
    }
  }
  return 0;
}

# Extract the compiler - assumed download() called and succeeded.
sub extract {
  `mkdir ./compilers/tmp_build` unless (-d "./compilers/tmp_build");

  `cp ./compilers/dl_cache/* ./compilers/tmp_build`;
  return 1 if $? != 0;

  print "\e[32mExtracting downloaded files [1/3]...\e[0m\n";
  `cd ./compilers/tmp_build/; tar -xf gcc-core-$COMPILER_VERSION.tar.bz2`;
  return 1 if $? != 0;

  print "\e[32mExtracting downloaded files [2/3]...\e[0m\n";
  `cd ./compilers/tmp_build/; tar -xf gcc-g++-$COMPILER_VERSION.tar.bz2`;
  return 1 if $? != 0;

  print "\e[32mExtracting downloaded files [3/3]...\e[0m\n";
  `cd ./compilers/tmp_build/; tar -xf binutils-$BINUTILS_VERSION.tar.bz2`;
  return 1 if $? != 0;

  `rm ./compilers/tmp_build/*.tar*`;
  return 1 if $? != 0;

  `rm -r ./compilers/dl_cache` if $delete_cache == 1;

  return 0;
}

# Extract the assembler - assumed download_nasm() called and succeeded.
sub extract_nasm {
  `mkdir ./compilers/tmp_build` unless (-d "./compilers/tmp_build");

  `cp ./compilers/dl_cache/nasm-$NASM_VERSION.tar.gz ./compilers/tmp_build`;
  return 1 if $? != 0;

  print "\e[32mExtracting downloaded files...\e[0m\n";
  `cd ./compilers/tmp_build/; tar -xf nasm-$NASM_VERSION.tar.gz`;
  return 1 if $? != 0;

  `rm ./compilers/tmp_build/*.tar*`;
  return 1 if $? != 0;

  `rm -r ./compilers/dl_cache` if $delete_cache == 1;

  return 0;
}


# Patch the compiler for amd64
sub patch_amd64 {
  print "\e[32mPatching gcc (amd64 patch)...\e[0m\n";
  `patch ./compilers/tmp_build/gcc-$COMPILER_VERSION/gcc/config.gcc <./compilers/gcc_amd64.patch`;
  return 1 if $? != 0;
	
  return 0;
}

# Patch the compiler to output more DWARF CFI information.
sub patch_dwarf {
    print "\e[32mPatching gcc (dwarf patch)...\e[0m\n";
    `patch -p1 ./compilers/tmp_build/gcc-$COMPILER_VERSION/ <./compilers/gcc_dwarf.patch`;
    return 1 if $? != 0;
    return 0;
}

# Configure, make and make install the compiler.
sub install {
  my ($arch) = @_;
  
  `mkdir ./compilers/$arch` unless -d "./compilers/$arch";

  `mkdir -p ./compilers/tmp_build/build_binutils`;
  `mkdir -p ./compilers/tmp_build/build_gcc`;

  print "\e[32mBinutils: \e[0m\e[32;1mPatching...\e[0m ";
  my ($pwd) = `pwd` =~ m/^[ \n]*(.*?)[ \n]*$/;
  `cd ./compilers/tmp_build/binutils-$BINUTILS_VERSION; wget http://www.jamesmolloy.co.uk/binutils-2.18-makeinfo.patch`;
  `cd ./compilers/tmp_build/binutils-$BINUTILS_VERSION; patch <binutils-2.18-makeinfo.patch`;

  print "\e[32;1mConfiguring...\e[0m ";
  
  `export PREFIX=$pwd/compilers; export TARGET=$arch; cd ./compilers/tmp_build/build_binutils/; ../binutils-$BINUTILS_VERSION/configure --target=\$TARGET --prefix=\$PREFIX --disable-nls >/tmp/binutils-configure.out 2>/tmp/binutils-configure.err`;
  if ($? != 0) {print "\e[31mFAIL (Log file at /tmp/binutils-configure.{out|err})\e[0m"; return 1;}
  print "\n\e[32;1mCompiling...\e[0m ";
  `export PREFIX=$pwd/compilers; export TARGET=$arch; cd ./compilers/tmp_build/build_binutils/; make all >/tmp/binutils-make.out 2>/tmp/binutils-make.err`;
  if ($? != 0) {print "\e[31mFAIL (Log file at /tmp/binutils-make.{out|err})\e[0m"; return 1;}
  print "\n\e[32;1mInstalling...\e[0m ";
  `export PREFIX=$pwd/compilers; export TARGET=$arch; cd ./compilers/tmp_build/build_binutils/; make install >/tmp/binutils-make-install.out 2>/tmp/binutils-make-install.err`;
  if ($? != 0) {print "\e[31mFAIL (Log file at /tmp/binutils-make-install.{out|err})\e[0m\n"; return 1;}
  print "\n";

  print "\e[32mGCC: \e[0m\e[32;1mConfiguring...\e[0m ";
  `export PREFIX=$pwd/compilers; export TARGET=$arch; cd ./compilers/tmp_build/build_gcc/; ../gcc-$COMPILER_VERSION/configure --target=\$TARGET --prefix=\$PREFIX --disable-nls --enable-languages=c,c++ --without-headers --without-newlib >/tmp/gcc-configure.out 2>/tmp/gcc-configure.err`;
  if ($? != 0) {print "\e[31mFAIL (Log file at /tmp/gcc-configure.{out|err})\e[0m"; return 1;}
  print "\n\e[32;1mCompiling...\e[0m ";
  `export PREFIX=$pwd/compilers; export TARGET=$arch; cd ./compilers/tmp_build/build_gcc/; make all-gcc >/tmp/gcc-make.out 2>/tmp/gcc-make.err`;
  if ($? != 0) {print "\e[31mFAIL (Log file at /tmp/gcc-make.{out|err})\e[0m"; return 1;}
  print "\n\e[32;1mInstalling...\e[0m ";
  `export PREFIX=$pwd/compilers; export TARGET=$arch; cd ./compilers/tmp_build/build_gcc/; make install-gcc >/tmp/gcc-make-install.out 2>/tmp/gcc-make-install.err`;
  if ($? != 0) {print "\e[31mFAIL (Log file at /tmp/gcc-make-install.{out|err})\e[0m\n"; return 1;}
  print "\n";

  print "\e[32mCleaning up...\e[0m\n";
  `rm -rf ./compilers/tmp_build`;

  return 0;
} 

sub install_nasm {

  my ($pwd) = `pwd` =~ m/^[ \n]*(.*?)[ \n]*$/;
  
  print "\e[32mNASM: \e[0m\e[32;1mConfiguring...\e[0m ";
  `export PREFIX=$pwd/compilers; cd ./compilers/tmp_build/nasm-$NASM_VERSION; ./configure --prefix=\$PREFIX >/tmp/nasm-configure.out 2>/tmp/nasm-configure.err`;
  if ($? != 0) {print "\e[31mFAIL (Log file at /tmp/nasm-configure.{out|err})\e[0m"; return 1;}
  
  print "\n\e[32;1mCompiling...\e[0m ";
  `export PREFIX=$pwd/computers; cd ./compilers/tmp_build/nasm-$NASM_VERSION; make >/tmp/nasm-make.out 2>/tmp/nasm-make.err`;
  if ($? != 0) {print "\e[31mFAIL (Log file at /tmp/nasm-make.{out|err})\e[0m"; return 1;}
  print "\n\e[32;1mInstalling...\e[0m ";
  `export PREFIX=$pwd/compilers; cd ./compilers/tmp_build/nasm-$NASM_VERSION; make install >/tmp/nasm-make-install.out 2>/tmp/nasm-make-install.err`;
  if ($? != 0) {print "\e[31mFAIL (Log file at /tmp/nasm-make-install.{out|err})\e[0m\n"; return 1;}
  print "\n";

  print "\e[32mCleaning up...\e[0m\n";
  `rm -rf ./compilers/tmp_build`;
  
  return 0;
}

#####################################################################################################
# Script start.

die("No target given!") unless scalar @ARGV > 0;

my $target = $ARGV[0];
my $compiler_already_installed = 0;
my $assembler_already_installed = 0;
my $install_compiler = 0;
my $install_assembler = 0;

if (is_installed($target)) {
  print "\e[32mTarget $target has a suitable compiler already installed.\e[0m\n";
  $compiler_already_installed = 1;
}

unless ($compiler_already_installed) {
  print "\e[32mTarget $target does not have a suitable compiler. One must be installed before the make process can continue. Install now? [yes]\e[0m: ";
  $install_compiler = 1;
  $install_compiler = 0 if <STDIN> =~ m/n/i;
}

if (is_nasm_installed($target)) {
  print "\e[32mTarget $target has a suitable assembler already installed, or does not need one.\e[0m\n";
  $assembler_already_installed = 1;
}

if (($assembler_already_installed == 0) and ($install_compiler == 0)) {
  print "\e[32mTarget $target does not have a suitable assembler (NASM). One must be installed before the make process can continue. Install now? [yes]\e[0m: ";
  $install_assembler = 1;
  $install_assembler = 0 if <STDIN> =~ m/n/i;
} elsif ($assembler_already_installed == 0) {
  $install_assembler = 1;
}

$ENV{CC} = "";
$ENV{CXX} = "";
$ENV{AS} = "";
$ENV{CPP} = "";
$ENV{CFLAGS} = "";
$ENV{CXXFLAGS} = "";
$ENV{LDFLAGS} = "";
$ENV{ASFLAGS} = "";

if ($install_compiler) {
  if (download($target) != 0) {
    print "\e[31mFATAL ERROR: Script cannot continue.\e[0m\n";
    exit 1;
  }
  
  if (extract($target) != 0) {
    print "\e[31mFATAL ERROR: Script cannot continue.\e[0m\n";
    exit 1;
  }
  
  if ($target eq "amd64-elf")
  {
    if (patch_amd64() != 0)
    {
      print "\e[31mFATAL ERROR: Script cannot continue.\e[0m\n";
      exit 1;
    }
  }

  if (patch_dwarf() != 0)
  {
      print "\e[31mFATAL ERROR: Script cannot continue.\e[0m\n";
      exit 1;
  }
  
  if (install($target) != 0) {
    print "\e[31mFATAL ERROR: Script cannot continue.\e[0m\n";
    exit 1;
  }
}

if ($install_assembler) {
  if (download_nasm($target) != 0) {
    print "\e[31mFATAL ERROR: Script cannot continue.\e[0m\n";
    exit 1;
  }
  
  if (extract_nasm($target) != 0) {
    print "\e[31mFATAL ERROR: Script cannot continue.\e[0m\n";
    exit 1;
  }
  
  if (install_nasm($target) != 0) {
    print "\e[31mFATAL ERROR: Script cannot continue.\e[0m\n";
    exit 1;
  }
}

exit 0;

