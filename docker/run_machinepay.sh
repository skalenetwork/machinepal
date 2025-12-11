#!/bin/sh

set -e

if [ ! -e /machinepay ]; then
    echo "Error: /machinepay directory does not exist inside container " >&2
    echo "You need to run machinepay container by mapping external volume into /machinepay"  >&2
    exit 1
fi


# Define the path for your flag file
FLAG_FILE="/var/lib/machinepay/first_run_complete"

if [ ! -f "$FLAG_FILE" ]; then
    # --- FIRST RUN ---
    echo "This is the first run. Executing one-time setup..."

    # Run your setup script
    /first-run-setup.sh

    # Create the flag file to mark setup as complete
    # Ensure the directory exists first
    mkdir -p $(dirname "$FLAG_FILE")
    touch "$FLAG_FILE"

    echo "Setup complete."
else
    # --- SUBSEQUENT RUNS ---
    echo "Not the first run, skipping setup."
fi


exec chpst -C /machinepay /usr/bin/machinepay
