# Temperature sensing device
Small battery-powered ESP32 device attached to a wall. Its main purpose is broadcasting measured temperature to a main unit. 
Battery is a standard Li-Ion 3.7V 18650. The device with 3000 mAh 18650 battery should be able to broadcast temperature for 60 days. 

For broadcasting is used Bluetooth Low Energy protocol. The device wakes up every 2 minutes and starts broadcasting for 1 second.

DS18B20 1-Wire temperature sensor is used for measuring temperature. The sensor is configured to measure temperature with resolution 0.125°C.

## BLE advertising
Bluetooth Low Energy defines Generic Access Profile (GAP) protocol in which a device can advertise their services with basic information about the device.

Temperature sensing device uses advertising to broadcast measured temperature with additional properties in scan response data:

1. Physical address
2. Device name (Temperature sensor)
3. Measured temperature

The main unit scans for BLE devices over and over and this way collects broadcasted messages.
