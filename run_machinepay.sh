#!/bin/sh
if [ ! -e /machinepay ]; then
    echo "Error: /machinepay directory does not exist inside container " >&2
    echo "You need to run machinepay container by mapping external volume into /machinepay"  >&2
    exit 1
fi

exec chpst -C /machinepay /usr/bin/machinepay
