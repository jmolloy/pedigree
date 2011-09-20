#!/usr/bin/env python
'''
    PUP: Pedigree UPdater

    Copyright (c) 2011 Matthew Iselin

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

    pup-common.py: common operations used by all pup scripts
'''

import ConfigParser, os

def getConfig(args):
    # Check for a config file
    # TODO: proper option parsing.
    configFile = "/support/pup/pup.conf"
    if len(args) > 0:
        configFile = args[0]
    
    # TODO: error handling

    # Try and read the config file
    if os.path.exists(configFile):
        cp = ConfigParser.ConfigParser()
        cp.read(configFile)

        localPath = cp.get("paths", "localdb")
        installRoot = cp.get("paths", "installroot")

        remotePath = []
        for server in cp.items("remotes"):
            remotePath += [server[1]]
    else:
        # Sane defaults!
        localPath="./local_repo"
        installRoot="./install_root"
        remotePath=["http://test.local/pup"]
    
    if localPath[-1] == "/":
        localPath = localPath[0:-1]
    if installRoot[-1] == "/":
        installRoot = installRoot[0:-1]
    if remotePath[-1] == "/":
        remotePath = remotePath[0:-1]
    
    return (remotePath, localPath, installRoot)

