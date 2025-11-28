#!/bin/sh
# Launch Chrome in kiosk mode for EGM Emulator GUI
# Waits for HTTP server to be available before launching

SENTINEL_HOME=/opt/ncompass
CHROME_URL="http://localhost:8080/index.html"
MAX_WAIT=30

echo "Waiting for HTTP server to be available..."

# Wait for server to be ready
counter=0
until curl -s -f "$CHROME_URL" > /dev/null 2>&1 || [ $counter -eq $MAX_WAIT ]; do
    echo "Waiting for HTTP server (attempt $counter/$MAX_WAIT)..."
    sleep 1
    counter=$((counter + 1))
done

if [ $counter -eq $MAX_WAIT ]; then
    echo "ERROR: HTTP server did not become available after $MAX_WAIT seconds"
    exit 1
fi

echo "HTTP server is ready, launching Chrome..."

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

# Find the Chrome binary
CHROME_BIN=""
if [ -x /usr/bin/google-chrome ]; then
    CHROME_BIN=/usr/bin/google-chrome
elif [ -x /usr/bin/chromium-browser ]; then
    CHROME_BIN=/usr/bin/chromium-browser
elif [ -x /usr/bin/chromium ]; then
    CHROME_BIN=/usr/bin/chromium
elif [ -x /usr/bin/chrome ]; then
    CHROME_BIN=/usr/bin/chrome
else
    echo "ERROR: Chrome/Chromium not found!" > /var/log/chrome.log 2>&1
    exit 1
fi

echo "Using Chrome binary: $CHROME_BIN"

# Launch Chrome in kiosk mode
# --kiosk: Fullscreen mode
# --no-first-run: Skip first run dialogs
# --disable-session-crashed-bubble: Don't show crash dialogs
# --disable-infobars: No info bars
# --window-size=640,240: Match display size
# --window-position=0,0: Top-left corner
# --user-data-dir: Persistent storage location
# --disable-translate: No translation prompts
# --disable-features=TranslateUI: Disable translate UI
# --disk-cache-dir=/tmp/chrome-cache: Use tmpfs for cache

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
    "$CHROME_URL" > /var/log/chrome.log 2>&1 &

CHROME_PID=$!
echo "Chrome launched with PID $CHROME_PID"

# Save PID for later management
echo $CHROME_PID > /var/run/chrome-egm.pid
