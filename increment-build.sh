#!/bin/bash
# Auto-increment build number

VERSION_FILE="include/megamic/version.h"

if [ -f "$VERSION_FILE" ]; then
    # Extract current build number
    CURRENT_BUILD=$(grep "define BUILD_NUMBER" "$VERSION_FILE" | awk '{print $3}')
    NEW_BUILD=$((CURRENT_BUILD + 1))

    # Update the build number
    sed -i "s/#define BUILD_NUMBER.*/#define BUILD_NUMBER $NEW_BUILD/" "$VERSION_FILE"

    echo "Build number incremented: $CURRENT_BUILD -> $NEW_BUILD"
else
    echo "Error: $VERSION_FILE not found"
    exit 1
fi
