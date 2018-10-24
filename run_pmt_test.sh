#!/bin/bash

#usage: filename should be passed WITHOUT CHANNEL NUMBER AND EXTENSION

CHANNEL_NUMBER=$1
FILENAME=$2

./main $CHANNEL_NUMBER $FILENAME

var="(root -l -q -x 'plot_QDC_histo.cpp(\"$FILENAME\",$CHANNEL_NUMBER)')"
eval $var

var2="(root -l -q -x 'fit_QDC_histo.cpp(\"$FILENAME\",$CHANNEL_NUMBER)')"
eval $var2
