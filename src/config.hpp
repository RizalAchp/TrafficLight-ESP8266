#pragma once

#ifndef __PROJECT_AKHIR_CONFIG_HEADER__
#define __PROJECT_AKHIR_CONFIG_HEADER__

#include <Arduino.h>

#if defined(ESP32)
#include <Ticker.h>
#include <WiFi.h>

#define RED_LED GPIO_NUM_4
#define YELLOW_LED GPIO_NUM_16
#define GREEN_LED GPIO_NUM_17

#define TRAFFIC_LIGHT_STATE(_RED, _YELLOW, _GREEN)                             \
	{                                                                      \
		digitalWrite(RED_LED, _RED);                                   \
		digitalWrite(YELLOW_LED, _YELLOW);                             \
		digitalWrite(GREEN_LED, _GREEN);                               \
	}

#define SUBSCRIBE_MQTT(TOPIC, QOS)                                             \
	{                                                                      \
		uint16_t id = mqttClient.subscribe(TOPIC, QOS);                \
		PRINTF("Sub topic '%s', QoS 0, id: %d\r\n", TOPIC, id);        \
		IGNORE(id);                                                    \
	}
#endif
#include <AsyncMqttClient.h>
#define IGNORE(...)
/// #include <LiquidCrystal_I2C.h>

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

constexpr const uint8_t INTERVALS[] = {30, 2, 30, 10, 25};

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
void PRINT_MQTT_DISCONNECT_REASON(const WiFiEventInfo_t &eventinfo);
constexpr const char *NAME_STATE_CONSTANT[] = {
    /*StateType::GREEN*/ "Green",
    /*StateType::YELLOW*/ "Yellow",
    /*StateType::RED*/ "Red",
    /*StateType::ISFIVE*/ "FiveCar",
    /*StateType::ISFIVE*/ "Capture"};
#else
#define SERIAL_BEGIN(...) IGNORE(__VA_ARGS__)
#define PRINT(...) IGNORE(__VA_ARGS__)
#define PRINTLN(...) IGNORE(__VA_ARGS__)
#define PRINTF(...) IGNORE(__VA_ARGS__)
#define BOOLSTR(_BOOL) IGNORE(_BOOL)
#define PRINT_MQTT_DISCONNECT_REASON(_REASON) IGNORE(_REASON)
#define PRINT_WIFI_DISCONNECT_REASON(_REASON) IGNORE(_REASON)
#endif

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

#define MAX_PAYLOAD_BUFFER_SIZE 32

/// @brief OnMqttEventConnect callback function
/// @param sessionPresent
void OnMqttEventConnect(bool sessionPresent);

/// @brief OnMqttEventDisconnect callback  function
/// @param reason
void OnMqttEventDisconnect(AsyncMqttClientDisconnectReason reason);

/// @brief OnMqttEventMessage callback function
/// @param topic
/// @param payload
/// @param prop
/// @param len
/// @param index
/// @param total
void OnMqttEventMessage(const char *topic, const char *payload,
			const AsyncMqttClientMessageProperties prop,
			const size_t len, const size_t index,
			const size_t total);
#endif
