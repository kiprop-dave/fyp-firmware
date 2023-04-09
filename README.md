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

- Arduino.h
- ArduinoJson.h
- DHT.h
- PubSubClient.h
- WiFi.h

## Configuration

Before running the code, the following parameters must be configured in the code:

1. DHT sensor pins and type

The DHTPIN1 and DHTPIN2 constants should be set to the pins that the DHT sensors are connected to.
The DHTTYPE constant should be set to the type of DHT sensor being used.

2. WiFi Details

The ssid and password constants should be set to the SSID and password of the WiFi network that the ESP32 will connect to.

3. MQTT server details

The mqtt_server, mqtt_user, and mqtt_password constants should be set to the address of the MQTT server, the username for the server, and the password for the server, respectively.
The mqtt_port constant should be set to the port that the MQTT server is listening on.

4. GPIO pins

The sirenPin constant should be set to the pin that the siren LED is connected to. The offButtonPin constant should be set to the
pin that the button to turn off the siren is connected to. The avianIdealPin, avianWarning, and avianCritical constants should be
set to the pins that the LEDs indicating the status of the bird enclosure are connected to. The reptileIdealPin, reptileWarning, and
reptileCritical constants should be set to the pins that the LEDs indicating the status of the reptile enclosure are connected to.

### Usage

Once the code has been configured, it can be uploaded to the ESP32 and run. The ESP32 will connect to the WiFi network and the MQTT server,
read the temperature and humidity from the DHT sensors, and send the data to the server. The status of the enclosures will be indicated by the LEDs,
and the siren LED can be turned on and off using the button.
