# EGM Emulator GUI - API Specification

## Overview
The web GUI communicates with the C++ backend via REST API (HTTP) and WebSocket for real-time updates.

## Base URL
- REST API: `http://localhost:8080/api`
- WebSocket: `ws://localhost:8080/ws`

## REST API Endpoints

### GET /api/status
Get current machine status
```json
Response: {
  "credits": 125.50,
  "winAmount": 0.00,
  "denom": 0.01,
  "gameName": "Zeus Lightning",
  "isPlaying": false,
  "status": "Ready"
}
```

### GET /api/ip
Get device IP address
```json
Response: {
  "ip": "192.168.1.100"
}
```

### GET /api/exceptions
Get list of available SAS exception codes
```json
Response: {
  "exceptions": [
    { "code": 16, "name": "Slot Door Open" },
    { "code": 17, "name": "Drop Door Open" },
    ...
  ]
}
```

### POST /api/play
Trigger a game play
```json
Request: {
  "denom": 0.01
}

Response: {
  "credits": 124.50,
  "winAmount": 5.00,
  "success": true
}
```

### POST /api/cashout
Cash out current credits
```json
Request: {}

Response: {
  "amount": 125.50,
  "credits": 0.00,
  "success": true
}
```

### POST /api/denom
Change denomination
```json
Request: {
  "denom": 0.25
}

Response: {
  "denom": 0.25,
  "success": true
}
```

### POST /api/exception
Trigger a SAS exception
```json
Request: {
  "code": 16  // 0x10 - Slot Door Open
}

Response: {
  "code": 16,
  "success": true
}
```

### POST /api/billinsert
Insert bill (add credits)
```json
Request: {
  "amount": 100.00
}

Response: {
  "credits": 225.50,
  "amount": 100.00,
  "success": true
}
```

## WebSocket Messages

The WebSocket connection sends real-time updates to the GUI.

### Server -> Client Messages

#### Status Update
```json
{
  "type": "status",
  "credits": 125.50,
  "winAmount": 0.00,
  "denom": 0.01,
  "gameName": "Zeus Lightning",
  "isPlaying": false,
  "status": "Ready"
}
```

#### Exception Event
```json
{
  "type": "exception",
  "code": 16,
  "name": "Slot Door Open",
  "active": true
}
```

#### Credit Update
```json
{
  "type": "credit_update",
  "credits": 225.50,
  "delta": 100.00
}
```

#### Win Notification
```json
{
  "type": "win",
  "amount": 50.00,
  "credits": 175.50
}
```

## Implementation Notes

### C++ HTTP Server Options
1. **cpp-httplib** - Single header, simple to use
2. **mongoose** - Embedded web server, supports WebSocket
3. **Boost.Beast** - Part of Boost, full-featured
4. **libmicrohttpd** - GNU library, lightweight

### Recommended: mongoose
- Single header implementation
- Built-in WebSocket support
- Small footprint
- Works well on embedded systems

### Integration Points
The HTTP server should integrate with the existing `Machine` class:
- `Machine::getCredits()` - Get current credits
- `Machine::playGame()` - Trigger game play
- `Machine::cashout()` - Cash out credits
- `Machine::setDenomination()` - Change denom
- `Machine::addException()` - Trigger SAS exception
- `Machine::incrementMeter()` - Add credits from bill insert

### Threading Considerations
- HTTP server should run in separate thread
- Use existing recursive_mutex for thread-safe access to Machine
- WebSocket broadcasts should not block Machine operations

### Example Structure
```cpp
class HTTPServer {
public:
    HTTPServer(Machine* machine, int port = 8080);
    void start();
    void stop();
    void broadcastStatus();

private:
    Machine* machine_;
    // mongoose or cpp-httplib server instance
    void handleStatus(Request& req, Response& res);
    void handlePlay(Request& req, Response& res);
    void handleException(Request& req, Response& res);
    // ... other handlers
};
```

## GUI Features

### Dynamic Exception List
The GUI displays all available SAS exceptions in a modal dialog:
- Populated from `/api/exceptions` endpoint
- Falls back to common exceptions if API fails
- Displays exception code (hex) and name
- Click to trigger

### Flexible Bill Insert
Support for any bill denomination:
- Common denominations: $1, $5, $10, $20, $50, $100
- Custom amount input field
- Converts to credits based on current denomination

### Keyboard Shortcuts
- **Space**: Play
- **C**: Cashout
- **Arrow Up/Down**: Change denomination
- **E**: Open exception modal
- **B**: Open bill insert modal
- **Escape**: Close modals

## Deployment

### Makefile Integration
Add to Makefile sentinel target:
```makefile
# Copy GUI media files
mkdir -p ${SENTINEL_HOME}/media
cp -r media/* ${SENTINEL_HOME}/media/
```

### Chrome Launch
Add to sentinel.sh or create separate service:
```bash
# Kill existing chrome
/opt/ncompass/bin/killchrome.sh

# Launch chrome in kiosk mode
chrome --kiosk --no-first-run --disable-session-crashed-bubble \
  --disable-infobars --window-size=640,240 \
  --window-position=0,0 http://localhost:8080/index.html &
```

## Testing

### Local Testing (Development)
1. Run C++ backend with HTTP server on port 8080
2. Open `media/index.html` in browser
3. GUI will attempt to connect to localhost:8080

### Zeus OS Testing
1. Build with GUI files included in sentinel.img
2. Deploy to Zeus OS
3. Start Sentinel (HTTP server starts automatically)
4. Chrome launches in kiosk mode, loads GUI from localhost
