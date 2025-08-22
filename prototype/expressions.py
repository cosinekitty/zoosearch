#!/usr/bin/env python3

import sys
from typing import Generator

Vars = 'abcdxyz'
NumVars = len(Vars)

def PostfixExpressions(exactOperatorCount:int) -> Generator[str, None, None]:
    if exactOperatorCount == 0:
        for u in 'abcdxyz':
            yield u
        return

    if exactOperatorCount == 1:
        for u in 'abcdxyz':
            for v in 'abcdxyz':
                if u < v:
                    yield u + v + '-'
                    yield v + u + '-'
                    yield u + v + '+'
                    yield u + v + '*'
        return

if __name__ == '__main__':
    print(', '.join(PostfixExpressions(0)))
    print(', '.join(PostfixExpressions(1)))
    sys.exit(0)