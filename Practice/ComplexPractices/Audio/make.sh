#!/bin/bash

set -e
gcc $@ -framework CoreAudio -framework AudioToolbox -framework CoreFoundation -o Audio
