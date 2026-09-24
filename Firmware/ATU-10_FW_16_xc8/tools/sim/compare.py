#!/usr/bin/env python3
"""Compare two simulator runs (tools/sim/sim output), e.g. original vs new.

Usage: compare.py ORIG.tsv NEW.tsv [--table]
"""

import sys


def load(path):
    rows = []
    for line in open(path):
        f, load, reached, best, steps = line.split('\t')
        rows.append((f, load, float(reached), float(best), int(steps)))
    return rows


def summary(name, rows):
    n = len(rows)
    reached = [r[2] for r in rows]
    steps = [r[4] for r in rows]
    ok12 = sum(s <= 1.2 for s in reached) / n * 100
    ok15 = sum(s <= 1.5 for s in reached) / n * 100
    miss = sum(r[2] > max(1.2, r[3] * 1.1) for r in rows)
    capped = [min(s, 10) for s in reached]
    print(f'{name:6} mean SWR {sum(capped) / n:5.2f} (capped at 10)  '
          f'<=1.2: {ok12:5.1f}%  <=1.5: {ok15:5.1f}%  '
          f'missed optimum: {miss:3}  mean steps {sum(steps) / n:6.1f}  max steps {max(steps)}')


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    a, b = load(argv[1]), load(argv[2])
    if '--table' in argv:
        print(f'{"MHz":>6} {"load":>12} {"best":>6} | {"orig":>6} {"steps":>5} | {"new":>6} {"steps":>5}')
        for x, y in zip(a, b):
            mark = '  better' if y[2] < x[2] - 0.02 else '  WORSE' if y[2] > x[2] + 0.02 else ''
            print(f'{x[0]:>6} {x[1]:>12} {x[3]:6.2f} | {x[2]:6.2f} {x[4]:5} | {y[2]:6.2f} {y[4]:5}{mark}')
    summary('orig', a)
    summary('new', b)
    worse = sum(y[2] > x[2] + 0.02 for x, y in zip(a, b))
    better = sum(y[2] < x[2] - 0.02 for x, y in zip(a, b))
    print(f'new vs orig: {better} better, {worse} worse, {len(a) - better - worse} equal')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
