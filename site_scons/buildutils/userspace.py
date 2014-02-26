
# Cleans a set of flags so we can build proper applications rather than
# freestanding binaries.
def fixFlags(env, flags):
    flags = flags.replace('-nostdinc', '')
    flags = flags.replace('-ffreestanding', '')
    flags = flags.replace('-nostdlib', '')
    flags = flags.replace('-fno-builtin', '')
    if env['ARCH_TARGET'] in ['X86', 'X64']:
        flags = flags.replace('-mno-mmx', '')
        flags = flags.replace('-mno-sse', '')
        flags = flags.replace('-fno-exceptions', '')
        flags = flags.replace('-fno-rtti', '')
        flags += ' -msse2 -mfpmath=sse '
    if env['ARCH_TARGET'] == 'X64':
        flags = flags.replace('-mcmodel=kernel', '-mcmodel=small')
        flags = flags.replace('-mno-red-zone', '')
        flags = flags.replace('-mno-mmx', '')
        flags = flags.replace('-mno-sse', '')
        flags += ' -msse2 -mfpmath=sse '
    if env['ARCH_TARGET'] == 'PPC':
        flags += ' -U__svr4__ '
    return flags
