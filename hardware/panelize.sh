#!/bin/bash

python2 ../../gerbmerge-1.8/gerbmerge/gerbmerge.py --place-file=placement.txt merge.cfg
cd rev_a/output
rm gerber.pdf
mv squall-blees-board-panelized.drd /tmp/
python2 ../../script/gerber2pdf.py
mv /tmp/squall-blees-board-panelized.drd ./

