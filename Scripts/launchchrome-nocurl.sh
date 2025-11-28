#!/bin/sh
# Launch Chrome in kiosk mode for EGM Emulator GUI
# Alternative version that doesn't require curl

SENTINEL_HOME=/opt/ncompass
CHROME_URL="http://localhost:8080/index.html"
MAX_WAIT=30

echo "Waiting for HTTP server to be available..." > /var/log/chrome-launch.log 2>&1

# Wait for server to be ready by checking if port 8080 is listening
counter=0
until netstat -ln | grep -q ":8080 " || [ $counter -eq $MAX_WAIT ]; do
    echo "Waiting for HTTP server (attempt $counter/$MAX_WAIT)..." >> /var/log/chrome-launch.log 2>&1
    sleep 1
    counter=$((counter + 1))
done

if [ $counter -eq $MAX_WAIT ]; then
    echo "ERROR: HTTP server did not become available after $MAX_WAIT seconds" >> /var/log/chrome-launch.log 2>&1
    exit 1
fi

echo "HTTP server is ready, launching Chrome..." >> /var/log/chrome-launch.log 2>&1

# Kill any existing Chrome instances
${SENTINEL_HOME}/bin/killchrome.sh

# Set up Chrome environment
export XDG_RUNTIME_DIR=/run/user/root
export DISPLAY=:0

# Chrome user data directory (for cache, settings, etc.)
CHROME_USER_DIR=/tmp/chrome-egm

# Clean up old Chrome data on each boot
rm -rf "$CHROME_USER_DIR"
mkdir -p "$CHROME_USER_DIR"

# Find the chrome binary
CHROME_BIN=""
if [ -x /usr/bin/chromium-browser ]; then
    CHROME_BIN=/usr/bin/chromium-browser
elif [ -x /usr/bin/chromium ]; then
    CHROME_BIN=/usr/bin/chromium
elif [ -x /usr/bin/chrome ]; then
    CHROME_BIN=/usr/bin/chrome
else
    echo "ERROR: Chrome/Chromium not found!" >> /var/log/chrome-launch.log 2>&1
    exit 1
fi

echo "Using Chrome binary: $CHROME_BIN" >> /var/log/chrome-launch.log 2>&1

# Launch Chrome in kiosk mode
$CHROME_BIN \
    --kiosk \
    --no-first-run \
    --disable-session-crashed-bubble \
    --disable-infobars \
    --disable-translate \
    --disable-features=TranslateUI \
    --window-size=640,240 \
    --window-position=0,0 \
    --user-data-dir="$CHROME_USER_DIR" \
    --disk-cache-dir=/tmp/chrome-cache \
    "$CHROME_URL" >> /var/log/chrome.log 2>&1 &

CHROME_PID=$!
echo "Chrome launched with PID $CHROME_PID" >> /var/log/chrome-launch.log 2>&1

# Save PID for later management
echo $CHROME_PID > /var/run/chrome-egm.pid

echo "Chrome launch complete. Check /var/log/chrome.log for output" >> /var/log/chrome-launch.log 2>&1
