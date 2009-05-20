#!/usr/bin/python
# coding: utf-8
"""

Copyright (c) 2009 James Molloy, Jörg Pfähler, Matthew Iselin

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

----------------------------------------------------

This script contains a set of functions which can be used in other
Python scripts or from the command line.

It should be noted that all relative paths are relative to the
_install ISO_ directory, not the current working directory. For
Pedigree, this is in images/install/disk/install-files.

---------- To add a file to the installer ----------
./scripts/maintain_installer.py add <relative path to file> \
                                    <file to install as> <compulsory>

For instance, the line for bash would be:
./scripts/maintain_installer.py add /applications/bash /applications/bash yes

Pass "-" as the destination to specify "Same as source" (to avoide duplication
as seen above).

The line for an empty file would be:
./scripts/maintain_installer.py add - /my_empty.txt yes

NOTE: No sanity checking is performed - ensure that the file is not already in
the list, or else it will be installed twice by the installer.

---------- To remove a file from the installer ----------
./scripts/maintain_installer.py remove <relative path to file>

So to remove bash from the installer:
./scripts/maintain_installer.py remove /applications/bash

Removing an empty file does not require any special changes, as the script
will automatically find empty files and handle their unique syntax properly.

You may specify a regular expression to remove multiple files.

---------- To list files in the installer ----------
./scripts/maintain_installer.py list <optional path>

So to list the applications directory:
./scripts/maintain_installer.py list /applications/

The list operation will recursively list files as they would appear on the
destination disk when installed. The path may be a regular expression, such as:

./scripts/maintain_installer.py list "/[acu]+.*?"

Which will display:
/applications/bash
/applications/ls
......
/config/greeting
......
/config/lynx/lynx.cfg
......
/users/james/.bashrc

----------------------------------------------------

Note: This script could probably be cleaned up a fair bit :)

"""

import os, sys, hashlib, re, time

# TODO: make the script able to run from anywhere?
baseDir = "./images/install/disk/install-files"
fileList = baseDir + "/filelist.txt"

def Read(fn):
    f = open(fn, "r")
    ret = f.read()
    f.close()
    return ret
    
def Write(fn, s):
    f = open(fn, "w")
    f.write(s)
    f.close()

def Sanitize(fList):
    lines = fList.split("\n")
    ret = ""
    for line in lines:
        line = line.strip()
        if(len(line) == 0):
            continue
        if(line[0] == '#'):
            continue
        ret += line + "\n"
    
    # Clean out excessive whitespace
    ret = re.sub("[ ]+", " ", ret)
    
    # Strip the trailing newline & return
    ret = ret.strip()
    return ret

def Username():
    if sys.platform == "win32":
        cmd="USERNAME"
    else:
        cmd="USER"
    return os.getenv(cmd)

def Md5OfFile(fn):
    global baseDir
    fn = baseDir + fn
    
    return hashlib.md5(Read(fn))

def addFile(fList, srcDir, destDir, compulsory = "yes"):
    # Append the entry (so simple <3)
    now = time.strftime("%I:%M:%S %d/%m/%y GMT", time.gmtime())
    newEntry = ""
    if(fList[-1] != '\n'):
        newEntry = "\n"
    newEntry += "#@@@ Added " + now + " by " + Username() + ".\n"
    
    if(srcDir[0] != '-'):
        if(srcDir[0] != '/'):
            srcDir = "/" + srcDir
    else:
        srcDir = ""
            
    if(destDir[0] == '-'):
        destDir = "*"
    elif(destDir[0] != '/'):
        destDir = "/" + destDir
    
    try:
        fileMd5 = "none"
        if(srcDir != ""):
            fileMd5 = Md5OfFile(srcDir).hexdigest()
        newEntry += srcDir + " " + destDir + " " + fileMd5 + " " + compulsory + "\n"
    except IOError:
        print "Couldn't access the file '" + baseDir + srcDir + "' for MD5 calculation!"
        return fList
    
    ret = fList + newEntry
    if(ret == fList):
        print "No file was added - check your filename and try again?"
    else:
        print "File added."
    return ret

def removeFile(fList, path):

    # Clean out excessive whitespace
    fList = re.sub("[ ]+", " ", fList)
    
    # Parse each line of the list
    dirList = []
    for line in fList.split("\n"):
    
        line = line.rstrip()
        if(len(line) == 0 or line[0] == '#'):
        
            prevLine = line
        
            # Handle removing files added by this script
            if(len(line) >= 2 and line[1] == '@'):
                continue
            dirList += [line]
            continue
        
        s = line.split(" ")
        if(len(s) == 0):
            continue
        
        # Now, we're looking at destination file here
        if(s[1] == '*'):
            s[1] = s[0]
        d = s[1]
        
        # Add it if the expression does not match
        m = re.match(path, d)
        if(m == None):
            dirList += [line]
    
    # Return the cleaned list
    ret = '\n'.join(dirList)
    if(ret == fList):
        print "No file was removed - check your filename and try again?"
    else:
        print "File removed."
    return ret

def listFiles(fList, dir = "/"):
    # Parse each line of the list
    dirList = []
    for line in fList.split("\n"):
        s = line.split(" ")
        if(len(s) == 0):
            continue
        d = ""
        if(len(s[0]) > 0):
            d = s[0]
        else:
            d = s[1]
        
        # Match against the passed directory
        m = re.match(dir, d)
        if(m <> None):
            dirList += [d]
    
    # And now match against the directory
    return '\n'.join(dirList)

def main(argc, argv):
    try:
        fList = Read(fileList)
        fCleanList = Sanitize(fList)
    except IOError:
        print "Couldn't find the file list - I'm looking in:"
        print fileList
        print "Make sure it exists and I can access it."
        return 0
    except:
        print "Unknown error accessing the file list!"
        return 0
    
    if(argc < 2):
        print "Not enough arguments passed!"
        return 0
    
    if(argv[1] == "list"):
        dir = "/"
        if(argc >= 3):
            dir = argv[2]
        listFiles(fCleanList, dir)
    elif(argv[1] == "add"):
        try:
            fList = addFile(fList, argv[2], argv[3], argv[4])
        except IndexError:
            print "Not enough arguments passed for the add command!"
            return 0;
        except:
            print "Unhandled error occurred while adding file"
            return 0
        
        Write(fileList, fList)
        
    elif(argv[1] == "remove"):
        try:
            fList = removeFile(fList, argv[2])
        except IndexError:
            print "Not enough arguments passed for the remove command!"
            raise
            return 0;
        except:
            print "Unhandled error occurred while removing file"
            raise
            return 0
        
        Write(fileList, fList)

if(len(sys.argv) <= 1):
    print "No parameters passed!"
else:
    main(len(sys.argv), sys.argv)

