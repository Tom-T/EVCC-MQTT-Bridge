#ifndef settings_h
#define settings_h

//Inverter settings
#define DeviceRxPin D7
#define DeviceTxPin D8

//Wifi Settings
#define WIFI_SSID "wifiname"
#define WIFI_PASSWORD "wifipassword"

//MQTT Broker
#define MQTT_HOST IPAddress(192, 168, 100, 150)
#define MQTT_PORT 1883
#define MQTT_USER "mqttusername"
#define MQTT_PASSWORD "mqttpassword"
#define MQTT_TOPIC "mppsolar"

#endif