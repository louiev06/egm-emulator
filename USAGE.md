# EGM Emulator - Usage Guide

## Building the EGM Emulator

### 1. Build on Windows

Open Command Prompt and navigate to the project directory:

```cmd
cd C:\_code\egm-emulator\egm-emulator-cpp
```

Run the build:

```cmd
wsl_build.bat --docker-target yocto --build-option sentinel
```

**Expected Output:**
```
Checking if WSL Ubuntu-20.04 is running...
Docker daemon is already running.
Syncing files to WSL...
Building EGMEmulator...
Creating sentinel.img...
sentinel.img created successfully!
Size: 1.2M

Build complete! sentinel.img is ready.

NOTE: This sentinel.img replaces the standard Sentinel application.
Copy it to the root of your SD card to use the EGM Emulator.
```

### 2. Verify Build Output

Check that `sentinel.img` was created:

```cmd
dir sentinel.img
```

You should see a file approximately 1-2MB in size.

## Deploying to Zeus OS Device

### 1. Copy to SD Card

Copy the `sentinel.img` file to the root of your Zeus OS SD card:

**Windows:**
```cmd
copy sentinel.img E:\
```
(Replace `E:\` with your SD card drive letter)

**Or manually:**
- Plug SD card into your computer
- Copy `sentinel.img` to the root of the SD card
- Eject SD card safely

### 2. Boot Zeus OS Device

1. Insert the SD card into your Zeus OS device (Device B - EGM Emulator)
2. Power on the device
3. Zeus OS will automatically:
   - Unpack sentinel.img to `/opt/ncompass/bin/Sentinel`
   - Execute the Sentinel application (which is our EGMEmulator)

### 3. Verify Application is Running

**Option 1: Check via serial console**
```bash
ps aux | grep Sentinel
```

**Option 2: Check log file**
```bash
tail -f /var/log/sentinel.log
```

**Expected log output:**
```
EGM Emulator - SAS Slave Device
Version 1.0.0
===============================
Platform: Zeus OS
Adding games...
  Game 1: Double Diamond ($0.01 denom)
  Game 2: Triple Stars ($0.25 denom)
  Game 3: Bonus Wheel ($1 denom)

Current game: Double Diamond

Adding progressive levels...
  Level 1 (Mini):  $100
  Level 2 (Minor): $500
  Level 3 (Major): $2500
  Level 4 (Grand): $10000

Adding $100 in credits...
Current credits: 10000 ($100)

Initializing SAS communication (Slave Mode)...
SAS Port started - Address: 1
Listening for SAS polls from master device...

Machine started and ready!
Machine playable: Yes

===============================
EGM Emulator running...
Press Ctrl+C to stop
===============================
```

## Connecting Devices

### Hardware Setup

You have 2 Zeus devices:

**Device A (Player Tracking - Master)**
- Sends SAS polls using SASDaemon
- Uses existing nCompass Sentinel application
- Serial port: `/dev/ttymxc4`

**Device B (EGM Emulator - Slave)**
- Responds to SAS polls using SASCommPort
- Uses our new EGMEmulator (as Sentinel)
- Serial port: `/dev/ttymxc4`

### Serial Cable Connection (Crossover)

Connect the 3-wire serial cables with crossover wiring:

```
Device A          Device B
--------          --------
Pin 1 (White/RX) ↔ Pin 2 (Red/TX)
Pin 2 (Red/TX)   ↔ Pin 1 (White/RX)
Pin 3 (Black/GND)↔ Pin 3 (Black/GND)
```

### Testing Communication

After connecting both devices:

**On Device B (EGM Emulator):**
```bash
tail -f /var/log/sentinel.log
```

You should see SAS statistics updating every 10 seconds:
```
--- SAS Statistics ---
Messages Received: 245
Messages Sent:     245
General Polls:     200
Long Polls:        45
CRC Errors:        0
Framing Errors:    0

--- Machine Status ---
Credits:           $100.00
Games Played:      0
Games Won:         0
Coin In:           $0.00
Coin Out:          $0.00
---------------------
```

**On Device A (Player Tracking):**

Check that it can poll Device B successfully. The logs will depend on your Player Tracking application.

## Runtime Behavior

### What the EGM Emulator Does

1. **Initializes** a gaming machine with:
   - 3 games (Double Diamond $0.01, Triple Stars $0.25, Bonus Wheel $1.00)
   - 4 progressive levels (Mini/Minor/Major/Grand)
   - $100 starting credits

2. **Listens** for SAS polls on `/dev/ttymxc4`:
   - Responds to general polls (0x81)
   - Handles long poll meter queries
   - Reports progressive levels
   - Processes TITO, AFT commands

3. **Reports** status every 10 seconds:
   - SAS communication statistics
   - Current credits
   - Meters (games played, coin in/out)

### SAS Commands Supported

The emulator responds to 32 SAS long poll commands:

**Meter Commands:**
- 0x11: Total Coin In
- 0x12: Total Coin Out
- 0x13: Total Drop
- 0x14: Total Jackpot
- 0x15: Games Played
- 0x16: Games Won
- 0x17: Games Lost
- 0x1F: Current Credits

**Progressive Commands:**
- 0x53: Send Progressive Levels
- 0x86: Set Progressive Group Level
- 0x87: Send Current Hopper Level
- 0x94: Send Legacy Bonus Award

**TITO Commands:**
- 0x70: Send Ticket Validation Data
- 0x71: Redeem Ticket
- 0x72: Request Last Validation Number
- And more...

**AFT Commands:**
- 0x72: AFT Transfer Request
- 0x73: AFT Transfer Complete
- 0x74: AFT Register
- And more...

## Troubleshooting

### Build Issues

**Problem**: Docker not running
```
Error: Failed to start Docker daemon.
```

**Solution**:
```cmd
wsl --distribution Ubuntu-20.04 --exec bash -c "sudo service docker start"
```

---

**Problem**: Permission denied on scripts
```
bash: ./build-with-docker.sh: Permission denied
```

**Solution**:
```bash
chmod +x build-with-docker.sh .docker-entrypoint.sh get-firmware-version.sh
```

---

**Problem**: sentinel.img not created
```
Warning: sentinel.img not found after build.
```

**Solution**: Check build logs for compilation errors. Run with verbose output:
```bash
MAKEFLAGS=-d wsl_build.bat --docker-target yocto --build-option sentinel
```

### Runtime Issues

**Problem**: Application not starting

**Check**:
1. Verify sentinel.img is on SD card root
2. Check file permissions: `ls -l /opt/ncompass/bin/Sentinel`
3. Check logs: `tail -f /var/log/sentinel.log`

---

**Problem**: No serial communication

**Check**:
1. Serial port exists: `ls -l /dev/ttymxc4`
2. Cable is connected with proper crossover wiring
3. Both devices are powered on
4. Check for SAS statistics in logs (should increment)

---

**Problem**: Application crashes

**Check**:
1. View logs: `tail -100 /var/log/sentinel.log`
2. Check for segmentation faults
3. Verify all dependencies are present: `ldd /opt/ncompass/bin/Sentinel`

Expected dependencies:
- `libpthread.so.0`
- `libs7lite.so`
- `libstdc++.so.6`
- `libgcc_s.so.1`
- `libc.so.6`

## Next Steps

Once the EGM Emulator is running and communicating:

1. ✅ Verify SAS communication is working
2. ✅ Confirm meters are being queried
3. 🔄 **Implement GUI (Phase 7)**
   - Add web interface for game simulation
   - Enable bill insertion simulation
   - Add play/cashout buttons
   - Display real-time credits and meters

4. 🔄 **Test full gameplay**
   - Simulate bill insertions
   - Play games via GUI
   - Cash out tickets
   - Verify TITO/AFT functionality

## Support

For build or runtime issues:
1. Check logs in `/var/log/sentinel.log`
2. Review build output for errors
3. Verify Zeus OS version compatibility
4. Check Docker image version matches nCompass firmware

## Summary

**Quick Reference:**

| Action | Command |
|--------|---------|
| Build | `wsl_build.bat --docker-target yocto --build-option sentinel` |
| Deploy | Copy `sentinel.img` to SD card root |
| Check Logs | `tail -f /var/log/sentinel.log` |
| Check Running | `ps aux \| grep Sentinel` |
| Stop | `pkill Sentinel` |
| Restart | Reboot Zeus OS device |

**File Locations on Zeus OS:**
- Executable: `/opt/ncompass/bin/Sentinel`
- Logs: `/var/log/sentinel.log`
- Serial Port: `/dev/ttymxc4`
- Config: None required (hardcoded defaults)
