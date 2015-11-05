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

from __future__ import print_function

import collections
import re
import subprocess
import sys
import tempfile


def err(s):
    print(s, file=sys.stderr)


def main():
    if len(sys.argv) < 2:
        err('Usage: graphmemlog.py <file path>')
        return 1

    with open(sys.argv[1], 'r') as f:
        memlog = f.readlines()

    # Extract the data points we care about.
    memlog = [x for x in memlog if not x.startswith('\t')]

    series_titles = {}
    series_start = {}
    series_delta = {}

    # Build plot data file.
    # 1 <series1> <series2> <series3>
    plot_content = ''
    for i, line in enumerate(memlog):
        entries = line.split('\t')
        plot_content += '%d ' % i
        for j, entry in enumerate(entries):
            fields = entry.split(':')

            series = fields[0].strip().lower()
            value = fields[1].strip().lower()

            # Remove 'K' indicator (not necessary - same units).
            value = value.strip('k')

            # Add to the row.
            series_titles[j] = series
            plot_content += '%s ' % value

            if series not in series_start:
                series_start[series] = value
            series_delta[series] = int(value) - int(series_start.get(series, 0))

        plot_content += '\n'

    # Create temp file for the plot content ready to pass to gnuplot
    plot_data = tempfile.NamedTemporaryFile()
    plot_data.write(plot_content)
    plot_data.flush()

    # 'plot' command (with the various series names).
    plot = 'plot '
    for index, title in series_titles.items():
        plot += '"%s" using 1:%d title "%s" with points pointtype 0, ' % (plot_data.name, index + 2, title.title())

    # Build gnuplot script.
    gnuplot_script = tempfile.NamedTemporaryFile()
    gnuplot_script.write('''#!/usr/bin/gnuplot
reset
set autoscale
set xtic auto
set ytic auto
set title "Pedigree Memory Usage"
set xlabel "Seconds"
set ylabel "Kilobytes"
set terminal pngcairo size 1440,900 enhanced font 'Verdana, 10'
set output "memlog.png"
%s
''' % plot)
    gnuplot_script.flush()

    # Run gnuplot.
    subprocess.check_call(['/usr/bin/gnuplot', gnuplot_script.name])

    # Show deltas on terminal.
    for series, delta in series_delta.items():
        print('Delta for %s: %sK' % (series, delta))

    # Show heap growth as % of page growth.
    heap_growth = series_delta.get('heap')
    page_growth = series_delta.get('pages')
    print('Heap comprises %.2f%% of the pages delta.' % (float(heap_growth) / float(page_growth) * 100.0,))

    # Time-based growth.
    heap_growth_bytes = heap_growth * 1024
    print('Heap grew by %.2f bytes/sec.' % (float(heap_growth_bytes) / float(i)))


if __name__ == '__main__':
    exit(main())
