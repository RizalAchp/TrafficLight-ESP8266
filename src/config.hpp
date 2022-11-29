#pragma once

#ifndef __PROJECT_AKHIR_CONFIG_HEADER__
#define __PROJECT_AKHIR_CONFIG_HEADER__

#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
/// #include <LiquidCrystal_I2C.h>

/////////////////////////////////////////////////////////////////////////
///               STATE TYPE TRAFFIC LIGHT BERUPA ENUM                ///
/////////////////////////////////////////////////////////////////////////
enum StateType {
	GREEN,
	YELLOW,
	RED,
	ISFIVE,
	CAPTURE,
};

//////////////////////////////////////////////////////////////////////////
/// NDEBUG DEFINE BLOCK, UNTUK NONAKTIFKAN SERIAL JIKA RELEASE MODE    ///
/// DENGAN CARA MENAMBAHKAN DEFINE SEBELUM HEADER INI DI INCLUDE ATAU  ///
///               PASSING ARGUMENT DEFINE PADA COMPILER                ///
//////////////////////////////////////////////////////////////////////////
#if !RELEASE // jika RELEASE != 1, Serial akan berfungsi
#define SERIAL_BEGIN(...) Serial.begin(__VA_ARGS__)
#define PRINT(...) Serial.print(__VA_ARGS__)
#define PRINTLN(...) Serial.println(__VA_ARGS__)
#define PRINTF(...) Serial.printf(__VA_ARGS__)
#define BOOLSTR(_BOOL) ((_BOOL) ? "true" : "false")
void PRINT_MQTT_DISCONNECT_REASON(AsyncMqttClientDisconnectReason reason);
void PRINT_WIFI_DISCONNECT_REASON(const WiFiEventStationModeDisconnected &ev);
void PRINT_STATE(StateType type, const unsigned long elapsed);

#else
#define SERIAL_BEGIN(...)
#define PRINT(...)
#define PRINTLN(...)
#define PRINTF(...)
#define BOOLSTR(_BOOL)
#define PRINT_MQTT_DISCONNECT_REASON(_REASON) (void)(_REASON)
#define PRINT_WIFI_DISCONNECT_REASON(_REASON) (void)(_REASON)
#define PRINT_STATE(TYPE, ELAPSED)
#endif

//////////////////////////////////////////////////////////////////////
/// CONFIGURASI CONSTANT VARIABLE, UBAH SEBELUM COMPILE DAN UPLOAD ///
//////////////////////////////////////////////////////////////////////

/// INTERVAL DALAM MILISECOND ( detik * 1000 )
constexpr auto SECOND_MS(const unsigned long SEC) { return 1000UL * SEC; }

/// constant array to make an easier access by indexing with StateType
constexpr const unsigned long INTERVALS[] = {
    /*StateType::GREEN => */ SECOND_MS(30),
    /*StateType::YELLOW => */ SECOND_MS(2),
    /*StateType::RED => */ SECOND_MS(30),
    /*StateType::ISFIVE => */ SECOND_MS(10),
    /*StateType::CAPTURE => */ SECOND_MS(25)};

#ifndef _SSID_WIFI

#define SECRET_SSID "realme C3"
#else
#define SECRET_SSID _SSID_WIFI

#endif

#ifndef _PSWD_WIFI

#define SECRET_PASSWORD "11223344"
#else
#define SECRET_PASSWORD _PSWD_WIFI

#endif

#ifndef _MQTT_SERVER

#define MQTT_SERVER "test.mosquitto.org" // DEFAULT SERVER
#else
#define MQTT_SERVER _MQTT_SERVER

#endif

#ifndef _MQTT_PORT

#define MQTT_PORT 1883
#else
#define MQTT_PORT _MQTT_PORT

#endif

#ifndef _MQTT_CLIENT_ID

#define MQTT_CLIENT_ID "ESP32WEMOS-TugasAkhir"
#else
#define MQTT_CLIENT_ID _MQTT_CLIENT_ID

#endif

#ifndef _TOPIC_CAPTURE

#define TOPIC_CAPTURE "capture"
#else
#define TOPIC_CAPTURE _TOPIC_CAPTURE
#endif

#ifndef _TOPIC_COUNT_OF_CAR
#define TOPIC_COUNT_OF_CAR "count_of_car"
#else
#define TOPIC_COUNT_OF_CAR _TOPIC_COUNT_OF_CAR
#endif

#ifndef _MQTT_QOS
#define DEFAULT_QOS 0
#else
#define DEFAULT_QOS _MQTT_QOS
#endif

#ifndef _MQTT_RETAIN
#define DEFAULT_RETAIN true
#else
#define DEFAULT_RETAIN _MQTT_RETAIN
#endif

#ifndef _MQTT_USERNAME
#define MQTT_USERNAME nullptr
#else
#define MQTT_USERNAME _MQTT_USERNAME
#endif

#ifndef _MQTT_PASSWORD
#define MQTT_PASSWORD nullptr
#else
#define MQTT_PASSWORD _MQTT_PASSWORD
#endif

#endif
