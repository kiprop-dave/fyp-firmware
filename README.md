## Introduction

This code is for an ESP32-based IoT project that monitors temperature and humidity in two different enclosures,
one for birds and one for reptiles. The code uses two DHT sensors to obtain temperature and humidity readings
and sends the data to an MQTT server over WiFi. The code also controls LEDs to indicate the status of the enclosures
and simulates a siren with another LED. There is also a push button that can stop the siren. The project uses platformio as
a toolchain. Visit the platformio website [here](https://platformio.org/) to get started.

This is part of a bigger project for my electrical enginnering final year project titled "real-time web based monitor/alert system for a small wildlife preserve hatchery".
The otherbuilding blocks of the project are a client web application and a server. Their codebases are found here:

- [client web application](https://github.com/kiprop-dave/fyp-webClient)
- [server](https://github.com/kiprop-dave/fyp-server)

## Dependencies

The following libraries are required to run this code:

- Arduino
- ArduinoJson
- Adafruit DHT sensor library
- Adafruit unified sensor library
- PubSubClient
- WiFi
- ESPAsyncWebserver
- SPIFFS

## Configuration

Before running the code, the following parameters must be configured in the code:

1. DHT sensor pins and type

The DHTPIN1 and DHTPIN2 constants should be set to the pins that the DHT sensors are connected to.
The DHTTYPE constant should be set to the type of DHT sensor being used.

2. MQTT server details

The mqtt_server, mqtt_user, and mqtt_password constants should be set to the address of the MQTT server, the username for the server, and the password for the server, respectively.
The mqtt_port constant should be set to the port that the MQTT server is listening on.

3. GPIO pins

The sirenPin constant should be set to the pin that the siren LED is connected to. The avianIdealPin, avianWarning, and avianCritical constants should be
set to the pins that the LEDs indicating the status of the avian enclosure are connected to. The reptileIdealPin, reptileWarning, and
reptileCritical constants should be set to the pins that the LEDs indicating the status of the reptile enclosure are connected to. The setLimitsPin should
be set to the pins that is connected to a button used to reset the limits. The reset_wifi_pin should be set to the pin connected to a button used to reset the wifi credentials.

### Usage

Once the code has been configured, upload the files in the data folder to the esp32. These are simple web pages that are used to fill in the user's configuration i.e the wifi
and the limits for the different enclosures. The code can then be uploaded to the esp32 and run. In order to initially set the wifi configuration, the esp32 will act as a wifi
access point. After connecting to the wifi access point, visit the ip address printed on the serial monitor where the wifi ssid and password can be filled in. If the connection is
successful, it will connect to the internet and the mqtt broker. In order to set the limits, visit the ipaddress/config/limits on a device connected to the same network and set the
respective limits. Once the limits have been set, the esp32 can then start taking readings and publishing them to the broker.
