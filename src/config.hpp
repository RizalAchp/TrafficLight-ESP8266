#pragma once

#ifndef __PROJECT_AKHIR_CONFIG_HEADER__
#define __PROJECT_AKHIR_CONFIG_HEADER__

#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
/// #include <LiquidCrystal_I2C.h>

/// define for pin led / lamp
#define RED_LED D5
#define YELLOW_LED D6
#define GREEN_LED D7

/////////////////////////////////////////////////////////////////////////
///               STATE TYPE TRAFFIC LIGHT BERUPA ENUM                ///
/////////////////////////////////////////////////////////////////////////
enum StateType : uint8_t {
	GREEN = 0,
	YELLOW = 1,
	RED = 2,
	ISFIVE = 3,
	CAPTURE = 4,
};

#define STATE_OUTPUT_CHANGE(_RED, _YELLOW, _GREEN)                             \
	{                                                                      \
		digitalWrite(RED_LED, _RED);                                   \
		digitalWrite(YELLOW_LED, _YELLOW);                             \
		digitalWrite(GREEN_LED, _GREEN);                               \
	}

/// callback event saat mqtt client mendapatkan message publish dari broker
/// => check apakah topic sesuai dengan yang dibutuhkan dan memproses payload
/// nya
void OnMqttEventMessage(char *topic, char *payload,
			AsyncMqttClientMessageProperties properties, size_t len,
			size_t index, size_t total);
/// callback event saat Mqtt client terputus dari sambungan mqtt server broker
/// =>  subscribe pada topic yang telah di tentukan
void OnMqttEventDisconnect(AsyncMqttClientDisconnectReason reason);
/// callback event saat Mqtt client telah tersambung pada server broker MQTT
/// =>  subscribe pada topic yang telah di tentukan
void OnMqttEventConnect(bool sessionPresent);

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
constexpr const char *NAME_STATE_CONSTANT[] = {
    /*StateType::GREEN*/ "Green",
    /*StateType::YELLOW*/ "Yellow",
    /*StateType::RED*/ "Red",
    /*StateType::ISFIVE*/ "(CarCount > 5)",
    /*StateType::ISFIVE*/ "Capture"};

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

/// constant array to make an easier access by indexing with StateType
constexpr const uint8_t INTERVALS[] = {
    /*StateType::GREEN in second => */ (30),
    /*StateType::YELLOW in second => */ (2),
    /*StateType::RED => in second */ (30),
    /*StateType::ISFIVE in second => */ (10),
    /*StateType::CAPTURE in second => */ (25)};

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
