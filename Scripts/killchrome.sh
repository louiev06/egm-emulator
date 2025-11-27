#!/bin/sh
while chromePids="$(pgrep '(^|/)(chrome|chromium|chromium-bin)($| )')" && [ -n "$chromePids" ]; do
    kill $chromePids
    echo "Waiting for chrome to exit"
    sleep 1
done
