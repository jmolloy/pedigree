#!/usr/bin/env python

# This script is designed to configure a pup config file with the paths from
# this checkout of Pedigree.

import os

scriptdir = os.path.dirname(os.path.realpath(__file__))

pupConfigDefault = "%s/scripts/pup/pup.conf.default" % (scriptdir,)
pupConfig = "%s/scripts/pup/pup.conf" % (scriptdir,)

# Open the original file
with open(pupConfigDefault, "r") as default:
    with open(pupConfig, "w") as new:
        s = default.read()
        s = s.replace("$installRoot", "%s/images/local" % (scriptdir,))
        s = s.replace("$localDb", "%s/images/local/support/pup/db" % (scriptdir,))
        new.write(s)

print "Configuration file '%s' updated." % (pupConfig)

