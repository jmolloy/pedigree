####################################
# SCons build system for Pedigree
## Tyler Kennedy (AKA Linuxhq AKA TkTech)
## <3 XLQ
####################################

import os.path
Import(['env'])

# To include a new subdirectory just add to the list.
subdirs = [
    'src'
]

SConscript([os.path.join(i, 'SConscript') for i in subdirs],exports = ['env'])