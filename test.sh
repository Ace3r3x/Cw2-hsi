#!/bin/bash

# Test script for MasterMind game

# Compile the game
make

# Check if compilation was successful
if [ $? -ne 0 ]; then
  echo "Compilation failed."
  exit 1
fi

# Run the game (requires sudo for GPIO access)
echo "Running the game..."
sudo ./mastermind

# Add more tests as needed