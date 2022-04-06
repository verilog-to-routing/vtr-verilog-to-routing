#!/usr/bin/python3

import sys
import re
import os
import argparse

openlist = []

def xprint(s, newline=False):
    for i in range(tabs):
        f.write("  ")
    f.write(str(s))

    if newline:
        f.write("\n")

def xbegin(s):
    global tabs

    xprint("<" + s)
    openlist.append(s)
    tabs = tabs + 1

def xclose():
    f.write(">\n")

def xcloseend():
    global tabs

    f.write("/>\n")
    openlist.pop()
    tabs = tabs - 1

def xend():
    global tabs

    s = openlist.pop()
    tabs = tabs - 1
    xprint("</" + s + ">\n")

def xcopy(fsrc):
    src = open(fsrc, "r")
    for line in src:
        xprint(line)
    src.close()
    f.write("\n")


def xprop(s, v):
    f.write(" " + s + '="' + str(v) + '"')

def switch_pos(pole, track):


def switchbox():


def clb():


def connectbox():