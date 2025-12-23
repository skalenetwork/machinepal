#!/bin/bash

set -e

# Variable for easy name changes
IMAGE_NAME="machinepal_image"

echo "Rebuilding $IMAGE_NAME..."

# Try to remove the image, but don't exit if it doesn't exist
docker rmi -f "$IMAGE_NAME" 2>/dev/null || true

# Build the image
docker build -t "$IMAGE_NAME" -f Dockerfile .``

echo "Docker image $IMAGE_NAME built successfully."

# test image

mkdir machinepal
cd machinepal && docker run -it -v $(pwd):/machinepal -e PUID=$(id -u) -e PGID=$(id -g) machinepal_image init
ls -l machinepal
rm -rf machinepal
echo "Docker image $IMAGE_NAME tested successfully."