#!/bin/bash
echo "THIS SCRIPT WILL NOW BUILD THE ATLAS_CLIENT SECURITY COMPONENTS"

# Create Build directory
echo "*********** Step 1. Creating build directories ************"
mkdir build build/data_plane
echo "******************* Step 1 finished! ********************"

# Buildint project
echo "******* Step 2. Building ATLAS_GATEWAY components *******"
cd build/
cmake ..
make
echo "******************* Step 2 finished! ********************"

echo "  ALL ATLAS_CLIENT COMPONENTS HAVE BEEN BUILT"
