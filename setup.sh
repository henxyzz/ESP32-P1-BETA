
#!/bin/bash

# Install prerequisites
echo "Installing prerequisites..."
apt-get update
apt-get install -y git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# Clone ESP-IDF
echo "Cloning ESP-IDF..."
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git

# Install ESP-IDF
cd ~/esp/esp-idf
./install.sh esp32

# Source the ESP-IDF configuration
echo "export IDF_PATH=~/esp/esp-idf" >> ~/.bashrc
echo ". ~/esp/esp-idf/export.sh" >> ~/.bashrc

echo "ESP-IDF setup complete. Please restart the shell with 'bash' and try building again."
