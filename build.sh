#!/bin/bash

export RTIMEHOME=/Applications/rti_connext_dds_micro-2.4.12
export RTIMEARCH=x64Darwin17clang9.0

$RTIMEHOME/resource/scripts/rtime-make --config Release --build --name x64Darwin17clang9.0 --target Darwin --source-dir . -G "Unix Makefiles" --delete