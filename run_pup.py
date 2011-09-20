#!/usr/bin/env python

# Runs pup with the correct configuration file for this checkout of Pedigree.

import os, sys

scriptdir = os.path.dirname(os.path.realpath(__file__))

pup = "%s/scripts/pup/pup" % (scriptdir,)
pupConfig = "%s/scripts/pup/pup.conf" % (scriptdir,)

# Run pup.
args = " ".join(sys.argv[1:])
args += " %s" % (pupConfig,)
os.system("%s %s" % (pup, args))

