#!/usr/bin/env python3

import sys
from typing import Generator

Vars = 'abcdxyz'

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
    for expr in PostfixExpressions(opcount):
        print(expr)
    sys.exit(0)