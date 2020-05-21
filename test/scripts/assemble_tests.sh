#!/bin/sh
echo hwtests/*.asm hwtests/*/*.asm hwtests/*/*/*.asm hwtests/*/*/*/*.asm | xargs python qdgbas.py
