#!/bin/sh
set -e

# 1. Check data dir (with sleep to prevent spin loop)
if [ ! -d /machinepay ]; then
    echo "Error: /machinepay directory missing." >&2
    sleep 10 # Prevent runit tight loop
    exit 1
fi

FLAG_FILE="/machinepay/first_run_complete"

if [ ! -f "$FLAG_FILE" ]; then
    echo "First run detected..."

    # 2. Correct path to setup script
    # We execute this as machinepay so created files have correct permissions
    chpst -u machinepay /usr/local/bin/first_run.sh

    touch "$FLAG_FILE"
    # Fix ownership of flag file so machinepay can see it later
    chown machinepay:machinepay "$FLAG_FILE"
fi

# 3. Run binary as machinepay
# -u: user
# -C: working directory
exec chpst -u machinepay -C /machinepay /usr/local/bin/machinepay