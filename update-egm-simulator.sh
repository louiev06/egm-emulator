#!/usr/bin/env bash
set -xEeuo pipefail

# Determine config file path (works from both WSL and Windows paths)
CONFIG_FILE="/mnt/c/.config/egmsimulatorip.txt"

# Initialize variables
MASTER_IP=""
SLAVE_IP=""

# Check if IP config file exists, otherwise prompt for it
if [ -f "$CONFIG_FILE" ]; then
    # Read config file line by line
    while IFS= read -r line; do
        # Remove carriage returns
        line=$(echo "$line" | tr -d '\r')

        # Skip empty lines and comments
        if [ -z "$line" ] || [[ "$line" == \#* ]]; then
            continue
        fi

        # Split on = sign
        if [[ "$line" == *"="* ]]; then
            key="${line%%=*}"
            value="${line#*=}"

            # Trim whitespace
            key=$(echo "$key" | xargs)
            value=$(echo "$value" | xargs)

            case "$key" in
                ncompass_master)
                    MASTER_IP="$value"
                    ;;
                egm_slave)
                    SLAVE_IP="$value"
                    ;;
            esac
        fi
    done < "$CONFIG_FILE"

    echo "Config loaded - Master: $MASTER_IP, Slave: $SLAVE_IP"
else
    echo "Config file not found: $CONFIG_FILE"
    read -p "Enter EGM slave IP address: " SLAVE_IP
    read -p "Enter nCompass master IP address: " MASTER_IP
fi

# Validate we have both IPs
if [ -z "$SLAVE_IP" ]; then
    echo "ERROR: No egm_slave IP configured!"
    exit 1
fi

if [ -z "$MASTER_IP" ]; then
    echo "ERROR: No ncompass_master IP configured!"
    exit 1
fi

user='root'
ssh='sshpass -p @ristocrat ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null'

# Read WSL config to determine build location
WSL_CONFIG="/mnt/c/.config/wsl.ini"
WSL_BUILD_PATH=""

if [ -f "$WSL_CONFIG" ]; then
    # Parse config file
    WSL_BASE=$(grep "^WSL_BASE=" "$WSL_CONFIG" | cut -d'=' -f2 | tr -d '\r')
    DEST_BASE=$(grep "^DEST_BASE=" "$WSL_CONFIG" | cut -d'=' -f2 | tr -d '\r')
    EGM_SOURCE=$(grep "^EGM_SOURCE_BASE=" "$WSL_CONFIG" | cut -d'=' -f2 | tr -d '\r')

    if [ -n "$WSL_BASE" ] && [ -n "$EGM_SOURCE" ]; then
        # Calculate relative path by removing EGM_SOURCE_BASE from hardcoded project path
        # EGM_SOURCE is "C:\_code", project is "C:\_code\egm-emulator"
        # Result should be "\egm-emulator" then convert to "/egm-emulator"

        # Simply hardcode the project name since we know the structure
        PROJECT_NAME="egm-emulator"

        # Build WSL path: WSL_BASE + /egm-emulator
        WSL_BUILD_PATH="${WSL_BASE}/${PROJECT_NAME}"

        echo "WSL Config found: WSL_BASE=$WSL_BASE"
        echo "Build path: $WSL_BUILD_PATH/build/dist/sentinel.img"
    fi
fi

# Determine build output location
# Build array of possible paths dynamically
POSSIBLE_PATHS=()

# Add WSL config location if available
if [ -n "$WSL_BUILD_PATH" ]; then
    POSSIBLE_PATHS+=("${WSL_BUILD_PATH}/build/dist/sentinel.img")
fi

# Add fallback locations
POSSIBLE_PATHS+=(
    "build/dist/sentinel.img"
    "/mnt/c/_code/egm-emulator/build/dist/sentinel.img"
)

SENTINEL_IMG=""
for path in "${POSSIBLE_PATHS[@]}"; do
    if [ -f "$path" ]; then
        SENTINEL_IMG="$path"
        echo "Found build: $SENTINEL_IMG"
        break
    fi
done

if [ -z "$SENTINEL_IMG" ]; then
    echo "ERROR: sentinel.img not found in any expected location!"
    for path in "${POSSIBLE_PATHS[@]}"; do
        echo "  Checked: $path"
    done
    exit 1
fi

# Deploy to EGM slave
echo ""
echo "Deploying to EGM slave: $SLAVE_IP"

# Send sentinel.img to /sdboot/ on the EGM slave
rsync \
    --chown=root:root \
    --inplace \
    --modify-window=1 \
    --progress \
    --rsh="$ssh" \
    --rsync-path='rsync' \
    "$SENTINEL_IMG" \
    "$user@$SLAVE_IP:/sdboot/sentinel.img"

echo "Deployment complete!"

# Reboot both machines
echo ""
echo "Rebooting nCompass master ($MASTER_IP) to detect updated EGM..."
sshpass -p @ristocrat ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ServerAliveInterval=1 -o ServerAliveCountMax=1 "$user@$MASTER_IP" 'nohup /sbin/reboot -f >/dev/null 2>&1 &' || true
echo "Master reboot command sent"

sleep 2

echo ""
echo "Rebooting EGM slave ($SLAVE_IP)..."
sshpass -p @ristocrat ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ServerAliveInterval=1 -o ServerAliveCountMax=1 "$user@$SLAVE_IP" 'nohup /sbin/reboot -f >/dev/null 2>&1 &' || true
echo "Slave reboot command sent"

echo ""
echo "Done! Master and EGM simulator rebooting..."
