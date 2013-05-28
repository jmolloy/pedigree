#!/usr/bin/env python

# This script is designed to configure a pup config file with the paths from
# this checkout of Pedigree.

import os
import sys

scriptdir = os.path.dirname(os.path.realpath(__file__))

pupConfigDefault = "%s/scripts/pup/pup.conf.default" % (scriptdir,)
pupConfig = "%s/scripts/pup/pup.conf" % (scriptdir,)

target_arch = 'i686'
if sys.argc > 1:
    target_arch = sys.argv[1]

# Open the original file
with open(pupConfigDefault, "r") as default:
    with open(pupConfig, "w") as new:
        s = default.read()
        s = s.replace("$installRoot", "%s/images/local" % (scriptdir,))
        s = s.replace("$localDb", "%s/images/local/support/pup/db" % (scriptdir,))
        s += '\n\n[settings]\narch=%s' % (target_arch,)
        new.write(s)

print "Configuration file '%s' updated." % (pupConfig)

