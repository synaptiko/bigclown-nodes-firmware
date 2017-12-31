#!/usr/bin/env bash
VERSION=`cat VERSION`
rm -rf out obj *.bin
make -j4 release APP_DIR="apps/climate-monitor" OUT=bcf-climate-monitor-kit-$VERSION VERSION=$VERSION
make -j4 release APP_DIR="apps/climate-monitor" EXTRA_CFLAGS="-D'USE_BATTERY_FORMAT_STANDARD' -D'USE_HUMIDITY_TAG' -D'HUMIDITY_TAG_CORRECTION_OFFSET'=12.0f" OUT=bcf-climate-monitor-tags-$VERSION VERSION=$VERSION
make -j4 release APP_DIR="apps/motion-detector" OUT=bcf-motion-detector-$VERSION VERSION=$VERSION
