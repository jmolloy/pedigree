#!/bin/bash

cd $1

# build whatever target the caller wanted, and make VS-style errors/warnings
make $2 2>&1 | bash ../scripts/visualStudioMakePipe.sh
