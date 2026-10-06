# Conveyor Belt Inertial Data Collector

This project is designed for inertial data collection from a conveyor belt, using an ESP32 equipped with a GY-512 (MPU-6050) sensor and a reed switch.

## Repository Structure

The project code is organized into the following main directories:

* **motor:** Contains the code for the Arduino responsible for controlling the conveyor belt's motor.

* **data_collect:** Contains the scripts that execute the data collection, from sensor reading to data logging.

## How to Run

1.  Power up the ESP32.
2.  On your computer, connect to the "ESTEIRA" Wi-Fi network (password: "12345678").
3.  In the `data_collect`* folder, run the script in a new terminal:

    ```sh
    python receptor_udp.py
    ```
4. Turn on the conveyor belt motor.

The ESP32 will then start collecting data. The log will be saved automatically when the reed switch passes the magnet on the belt.

*Obs.: You can also try running it from a new, empty folder containing only the receptor_udp.py file. No need to clone the entire repository.

## PlatformIO Compatibility

Both `main.cpp` files in this repository were developed using **PlatformIO** (https://platformio.org/) within VS Code.

To upload or modify these files properly, it is recommended to open the project using the PlatformIO extension in VS Code.

To do so, follow these steps:

* Install PlatformIO extension.

* Select the **PlatformIO icon** on the left sidebar in VS Code:

![Ícone do PlatformIO](images/platformio_icon.png)

* Navigate to *Quick Access > PIO Home > Open* and select `+ New Project`.

* Fill in the required information:
    * **Board:** Espressif ESP32 Dev Module
    * **Framework:** Arduino

* Inside the `src` folder, replace the content of the existing `main.cpp` file with the desired code, either from the **motor** or **data_collect** directory.