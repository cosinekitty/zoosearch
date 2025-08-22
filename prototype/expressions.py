#!/usr/bin/env python3

import sys
from typing import Generator

def PostfixExpressions(exactOperatorCount:int) -> Generator[str, None, None]:
    if exactOperatorCount == 0:
        for c in 'abcdxyz':
            yield c

if __name__ == '__main__':
    print(', '.join(PostfixExpressions(0)))
    sys.exit(0)