#!/usr/bin/env python
'''
    PUP: Pedigree UPdater

    Copyright (c) 2010 Matthew Iselin

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

    pup-install.py: install a package
'''

import os, sys, urllib, sqlite3, tarfile, ConfigParser

import pup_common

def main(arglist):

    remotePath, localPath, installRoot, desiredArch = pup_common.getConfig(arglist[1:])

    if not os.path.exists(localPath):
        os.makedirs(localPath)
    if not os.path.exists(installRoot):
        os.makedirs(installRoot)
    
    s = sqlite3.connect(localPath + "/packages.pupdb")
    e = s.execute("select * from packages where name=? and arch=? order by ver desc limit 1", ([arglist[0]]))
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
    t.extractall(installRoot)
    
    print "Package %s [%s] is now installed." % (packageName, desiredArch)

if __name__ == '__main__':
    main(sys.argv[1:])
