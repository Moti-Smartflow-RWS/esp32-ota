
#!/bin/bash

# Path to your Arduino project
PROJECT_PATH="/Users/motischnapp/Library/CloudStorage/GoogleDrive-snir@smartflow-rws.com/האחסון שלי/Old/Arduino-S/Smartflow_Box_WiFi_ESP32/ESP32Code"

# Move to the project directory
cd "$PROJECT_PATH" || { echo "Project path not found"; exit 1; }

# Add all changes
git add .

# Commit with a timestamp message
git commit -m "Auto update: $(date '+%Y-%m-%d %H:%M:%S')"

# Push to GitHub
git push

# Show a macOS notification using AppleScript
osascript -e 'display notification "Project pushed to GitHub successfully!" with title "GitHub Sync"'
