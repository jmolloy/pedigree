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


import sys

left, right = sys.argv[1:]

with open(left) as f:
    left = f.readlines()

with open(right) as f:
    right = f.readlines()

left_locks = {}
right_locks = {}

def strip(l):
    return l.strip('[,]').strip()

def extract(l):
    y = l[0].split(' ')

    return {
        'lock': strip(y[2]),
        'cpu': strip(y[4]),
        'ret': strip(y[-2]),
        'order': l[1],
        'index': strip(y[-1]),
    }


N = 0
def do_locks(l, a):
    global N
    for x in l:
        x = x.strip()
        if not x:
            continue
        if not x.startswith('Spinlock'):
            continue
        if not 'acquire' in x and not 'release' in x:
            continue

        y = x.split(' ')
        lck = y[2]
        lck = strip(lck)

        acq = 'acquire' in x
        if acq:
            a[lck] = (x, N)
        else:
            try:
                del a[lck]
            except KeyError:
                # print lck, x
                pass

        N += 1


do_locks(left, left_locks)
do_locks(right, right_locks)

same = set(left_locks.keys()) & set(right_locks.keys())

sames = []
for x in same:
    sames.append(left_locks[x])
    sames.append(right_locks[x])

sames.sort(key=lambda x: int(extract(x)['index']))

def noice(d):
    return '{' + ', '.join('%s=%s' % i for i in d.items()) + '}'

for l in sames:
    x = extract(l)
    print 'CPU%s Attempts:' % x['cpu']
    print x
    print

# print left_locks
# print right_locks
