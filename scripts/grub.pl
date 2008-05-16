#!/usr/bin/perl

my $GRUB_VERSION = "0.97";

# Do we have a compiler download cache?
sub is_cached {
  return 1 if (-d "./compilers/dir/dl_cache" and
               -f "./compilers/dir/dl_cache/grub-$GRUB_VERSION.tar.gz");
  return 0;
}

# Download the compiler.
sub download {
  my $already_cached = 0;

  if (-f "./compilers/dir/dl_cache/grub-$GRUB_VERSION.tar.gz") {
    print "\e[32mgrub files already downloaded, using cached version.\e[0m\n";
    $already_cached = 1;
  } else {
    print "\e[32mDownloading grub files, version $GRUB_VERSION...\e[0m\n";  
    `cd ./compilers/dir/dl_cache; wget ftp://alpha.gnu.org/gnu/grub/grub-$GRUB_VERSION.tar.gz`;
    if ($? != 0) {
      print "\e[31mError: grub files failed to download!\e[0m\n";
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

# Extract the compiler - assumed download() called and succeeded.
sub extract {
  `mkdir ./compilers/dir/tmp_build` unless (-d "./compilers/dir/tmp_build");

  `cp ./compilers/dir/dl_cache/* ./compilers/dir/tmp_build`;
  return 1 if $? != 0;

  print "\e[32mExtracting downloaded files...\e[0m\n";
  `cd ./compilers/dir/tmp_build/; tar -xzf grub-$GRUB_VERSION.tar.gz`;
  return 1 if $? != 0;

  `rm ./compilers/dir/tmp_build/*.tar*`;
  return 1 if $? != 0;

  `rm -r ./compilers/dir/dl_cache` if $delete_cache == 1;

  return 0;
}

#Patch grub to load elf64 files
sub patch_grub
{
  print "\e[32mPatching grub...\e[0m\n";
  `cp compilers/grub_amd64.patch compilers/dir/tmp_build/grub-$GRUB_VERSION && cd compilers/dir/tmp_build/grub-$GRUB_VERSION && pwd > ../../../test.txt && patch -p1 <grub_amd64.patch`;
  return 1 if $? != 0;

  return 0;
}

# Configure, make and make install the compiler.
sub install {
  `mkdir -p ./compilers/dir/tmp_build/build_grub`;

  my ($pwd) = `pwd` =~ m/^[ \n]*(.*?)[ \n]*$/;

  print "\e[32mgrub: \e[0m\e[32;1mConfiguring...\e[0m ";
 `export PREFIX=$pwd/compilers/dir; cd ./compilers/dir/tmp_build/build_grub/; ../grub-$GRUB_VERSION/configure --prefix=\$PREFIX --disable-ffs --disable-ufs2 --disable-minix --disable-reiserfs --disable-vstafs --disable-jfs --disable-xfs >/tmp/grub-configure.out 2>/tmp/grub-configure.err`;
  if ($? != 0) {print "\e[31mFAIL (Log file at /tmp/gcc-configure.{out|err})\e[0m"; return 1;}
  print "\n\e[32;1mCompiling...\e[0m ";
  `export PREFIX=$pwd/compilers/dir; cd ./compilers/dir/tmp_build/build_grub/; make >/tmp/grub-make.out 2>/tmp/grub-make.err`;
  if ($? != 0) {print "\e[31mFAIL (Log file at /tmp/grub-make.{out|err})\e[0m"; return 1;}
  print "\n\e[32;1mInstalling...\e[0m ";
  `export PREFIX=$pwd/compilers/dir; cd ./compilers/dir/tmp_build/build_grub/; make install >/tmp/grub-make-install.out 2>/tmp/grub-make-install.err`;
  if ($? != 0) {print "\e[31mFAIL (Log file at /tmp/grub-make-install.{out|err})\e[0m\n"; return 1;}
  print "\n";

  print "\e[32mCleaning up...\e[0m\n";
  `rm -rf ./compilers/dir/tmp_build`;

  return 0;
} 

#####################################################################################################
# Script start.
if (download() != 0) {
  print "\e[31mFATAL ERROR: Script cannot continue.\e[0m\n";
  exit 1;
}

if (extract() != 0) {
  print "\e[31mFATAL ERROR: Script cannot continue.\e[0m\n";
  exit 1;
}

if (patch_grub() != 0)
{
  print "\e[31mFATAL ERROR: Script cannot continue.\e[0m\n";
  exit 1;
}

if (install() != 0) {
  print "\e[31mFATAL ERROR: Script cannot continue.\e[0m\n";
  exit 1;
}

exit 0;
