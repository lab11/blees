#!/bin/bash

python2 ../../gerbmerge-1.8/gerbmerge/gerbmerge.py --place-file=merged.placement.txt merge.cfg
cd rev_a/output
rm gerber.pdf squall-blees-board-panelized.drd
python2 ../../script/gerber2pdf.py

