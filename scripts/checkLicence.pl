 #!/usr/bin/perl

 #
 # Copyright (c) 2008 James Molloy, JÃ¶rg PfÃ¤hler, Matthew Iselin
 #
 # Permission to use, copy, modify, and distribute this software for any
 # purpose with or without fee is hereby granted, provided that the above
 # copyright notice and this permission notice appear in all copies.
 #
 # THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 # WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 # MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 # ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 # WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 # ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 # OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 #

RAH

print "Checking C++ files...\n";

my $files = `find ./ -name CMakeLists.txt -or -name *.pl`;
foreach $filename (split /\n/, $files) {
  open my $fd, "<", $filename;
  
  # check for the correct licence.
  local $/ = undef;
  my $file = <$fd>;
  unless ($file =~ m|$make_licence|m) {
    print "$filename does not have the correct licence. Fixing...\n";
    $file =~ s,#(.|\n)*?Copyright(.|\n)*?#\n+,$make_licence,;
     close $fd;
     open my $fd2, ">", $filename;
     binmode $fd2, ':encoding(UTF-8)';
     print $fd2 "$file";
     close $fd2;
  }
}
