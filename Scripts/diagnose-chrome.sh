#!/bin/sh
# Diagnostic script to troubleshoot Chrome display issues

echo "=== Chrome Diagnostic Report ==="
echo ""

echo "1. Checking if Chrome process is running..."
ps aux | grep chrome | grep -v grep
echo ""

echo "2. Checking Chrome launch log..."
if [ -f /var/log/chrome-launch.log ]; then
    cat /var/log/chrome-launch.log
else
    echo "No chrome-launch.log found"
fi
echo ""

echo "3. Checking Chrome output log..."
if [ -f /var/log/chrome.log ]; then
    tail -50 /var/log/chrome.log
else
    echo "No chrome.log found"
fi
echo ""

echo "4. Checking available Chrome binaries..."
for binary in /usr/bin/chromium-browser /usr/bin/chromium /usr/bin/chrome; do
    if [ -x "$binary" ]; then
        echo "Found: $binary"
        $binary --version 2>&1 || echo "  (cannot get version)"
    fi
done
echo ""

echo "5. Checking display environment..."
echo "DISPLAY=$DISPLAY"
echo "XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR"
ps aux | grep -i weston | grep -v grep
echo ""

echo "6. Checking HTTP server..."
curl -I http://localhost:8080/index.html 2>&1 | head -10
echo ""

echo "7. Checking media files..."
ls -la /opt/ncompass/media/ 2>&1 | head -20
echo ""

echo "8. Checking for Chrome core dumps..."
ls -lh /tmp/core* 2>&1 | head -10
echo ""

echo "=== End Diagnostic Report ==="
