#!/usr/bin/env python3

import sys
from typing import Generator

Vars = 'axyz'

def PostfixExpressions(opcount:int) -> Generator[str, None, None]:
    if opcount == 0:
        for u in Vars:
            yield u
    elif opcount > 0:
        for leftCount in range(opcount):
            rightCount = (opcount-1) - leftCount
            for u in PostfixExpressions(leftCount):
                for v in PostfixExpressions(rightCount):
                    if u != v:
                        yield u + v + '-'
                    if u < v:
                        yield u + v + '+'
                    if u <= v:
                        yield u + v + '*'

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('USAGE: expressions.py opcount')
        sys.exit(1)
    opcount = int(sys.argv[1])
    print('\n'.join(PostfixExpressions(opcount)))
    sys.exit(0)