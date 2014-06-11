#!/usr/bin/env python2.7
'''
Copyright (c) 2008-2014, Pedigree Developers

Please see the CONTRIB file in the root of the source tree for a full
list of contributors.

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
'''

import os, sys, stat, shutil, urllib, sqlite3, tarfile

import pup_common

def main(arglist):

    remotePath, localPath, installRoot, desiredArch = pup_common.getConfig(arglist[1:])

    if not os.path.exists(localPath):
        os.makedirs(localPath)
    if not os.path.exists(installRoot):
        os.makedirs(installRoot)
    
    s = sqlite3.connect(localPath + "/packages.pupdb")
    e = s.execute("select * from packages where name=? and arch=? order by ver desc limit 1", (arglist[0], desiredArch))
    data = e.fetchone()
    if data is None:
        s.close()
        
        print "The package '%s' [%s] is not available. You may need to run `pup sync' to update the list of available packages." % (arglist[0], desiredArch)
        exit(1)
    s.close()
    
    # Package name
    packageName = "%s-%s-%s" % (data[1], data[2], desiredArch)
    localFile = "%s/%s.pup" % (localPath, packageName)
    
    print "Preparing to install %s" % (packageName)
    
    if not os.path.exists(localFile):

        print "    -> Downloading..."

        for server in remotePath:
            remoteUrl = "%s/%s.pup" % (server, packageName)
        
            print "      + trying %s" % (remoteUrl),
            try:
                o = urllib.FancyURLopener()
                o.retrieve(remoteUrl, localFile)
                print "(OK)"
                break
            except:
                print "(failed)"
                continue
        
        if not os.path.exists(localFile):
            print "Error: couldn't download the package from any server. Check your internet connection and try again."
            exit(1)
    
    print "    -> Installing..."
    
    # TODO: track installed packages in a local database
    
    t = tarfile.open(localFile)

    # Perform extraction, but ignore owner/group information
    uid = os.getuid()
    gid = os.getgid()
    for f in t.getmembers():
        name = os.path.join(installRoot, f.name)
        linkname = os.path.join(installRoot, f.linkname)
        mode = f.mode

        # Make sure the caller can write to the file always.
        mode |= stat.S_IWRITE

        # We need to create the file now.
        if f.isdir():
            if not os.path.exists(name):
                os.makedirs(name)
        elif f.isfile() or f.isreg():
            buf = t.extractfile(f)
            with open(name, 'w') as g:
                shutil.copyfileobj(buf, g)

            os.chmod(name, mode)
        elif f.issym():
            if os.path.lexists(name):
                os.unlink(name)
            try:
                os.symlink(linkname, name)
            except OSError:
                print "Extracting %s failed, target %s does not exist." % (f.name, f.linkname)
        elif f.islnk():
            if os.path.lexists(name):
                os.unlink(name)
            try:
                os.link(linkname, name)
            except OSError:
                print "Extracting %s failed, target %s does not exist." % (f.name, f.linkname)
        else:
            print "(%s is not a sane file type)" % (f.name,)

        #print name
    #t.extractall(installRoot)
    
    print "Package %s [%s] is now installed." % (packageName, desiredArch)

if __name__ == '__main__':
    main(sys.argv[1:])
