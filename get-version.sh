#!/bin/bash
# Extract version from version.h and output as VERSION_STRING.BUILD_NUMBER

VERSION_STRING=$(grep "VERSION_STRING" include/version.h | sed 's/.*"\(.*\)".*/\1/')
BUILD_NUMBER=$(grep "BUILD_NUMBER" include/version.h | awk '{print $3}')

echo "${VERSION_STRING}.${BUILD_NUMBER}"
