# ESPRIDGE 🧑‍🚀
ESP32 based Home Automation Bridge that is easy to upload and simple to use.

# What is it ?

A very simple implementation of the bridge to work with Echo devices (Alexa).
I've been running HA Bridge on Raspbery PI for years, using lots of ESP devices that i linked with many routines in my home.
Things like relays for watering, garage doors opening, Satel alarm integration with their ETHM1, and others.

After several PI crashes caused by the SD card, faulty upgrades and never-ending-command line actions i decided to get rid of this setup
and migrate to something simple, low power, and always available - zero downtimes. 
That's when i wrote my own implementation for the ultra low power device which is ESP32.

Feel free to play with the code, modify and update it as you wish, just remember to give some shout outs somewhere 🤝

# How does it work 🥣

ESPRIDGE will respond to SSDP queries that will happen when you ask `Alexa discover devices`.

- After the devices are discovered, you can say : `Alex turn on your_device` or `Alexa turn off your_device`.

- When you ask to `turn on` the device, ESPRIDGE will call a URL you have provided for `ON` action for your light.

- Similarily when you ask `turn off`, ESPRIDGE will call a URL for `OFF` action for your light.

When creating lights, you can provide these two urls in the form:

![image](https://github.com/invpe/ESPRIDGE/assets/106522950/570308ae-5327-4bd3-9e57-db3fc5708cd6)

# How can i use it ? ❔

Simply put your WiFi credentials in the code:

```
#define WIFI_A ""
#define WIFI_P ""
```

Compile, Upload and you're ready to go 🍪


- Access the panel via a simple url `http://espbridge/`
- By default `OTA` is turned `ON`, you can change that with `#ifdef OTA_SUPPORT`


# Screenshots 🖼️

`Adding new/editing lights`

![image](https://github.com/invpe/ESPRIDGE/assets/106522950/cc08c2e8-ae0c-4561-83ff-f91ea4167338)

  
`Background Discovery process`

![image](https://github.com/invpe/ESPRIDGE/assets/106522950/f4156757-5b1f-4070-89f5-2ea37fb2dfb9)

`Alexa visibility and control`

![image](https://github.com/invpe/ESPRIDGE/assets/106522950/1617ffeb-8431-4a95-800d-c00593f84d5d)

![image](https://github.com/invpe/ESPRIDGE/assets/106522950/67a132e2-8400-48fd-aa37-301894bf9704)
