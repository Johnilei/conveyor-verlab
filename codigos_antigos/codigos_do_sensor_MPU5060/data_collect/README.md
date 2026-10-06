## Components

### main.cpp

This is the code on the ESP32. It creates its own Wi-Fi network, continuously reads data from the sensor and sends everything it has collected when the magnetic switch is activated.

Obs.: If you want to modify this code using PlatformIO, add the line "monitor_speed = 115200" to your `platformio.ini` file. This ensures the serial monitor will display the output correctly.

### receptor_udp.py

This is the script that receives the collected data. It "pings" the ESP32 to let it know it's ready to start, waits for the data to arrive and then saves it into a `.csv` file in the `logs` folder.