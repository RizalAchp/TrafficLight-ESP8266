#include <Arduino.h>
#include <config.hpp>

/// variable to store `millis()` value
unsigned long startTime = 0;
/// variable to compare time elapsed from `millis()`
unsigned long elapsedTime = 0;
/// state variable
StateType nextState = StateType::GREEN;
StateType currentState = StateType::GREEN;
StateType lastState = StateType::YELLOW;
/// variable to check if car is greater than 5
bool isCarGreaterThanFive = false;
/// client mqtt object
AsyncMqttClient mqttClient;
RECONNECT_DEF(wifiReconnectTimer, mqttReconnectTimer);

/// callback event saat Wifi mendapatkan STA IP dari AccessPoint Wifi
/// (connected)
/// =>  mencoba untuk connect pada broker MQTT server
HANDLE_EVENT(wifiConnectHandler, wifiDisconnectHandler);

#if !RELEASE
Ticker printerStateTimer;
#endif

inline static void subscribe(const char *topic, uint8_t qos)
{
	auto id = mqttClient.subscribe(topic, qos);
	PRINTF("Subscribe to topic '%s', QoS 0, id: %ud", topic, id);
	IGNORE(id);
}
/// callback event saat Mqtt client telah tersambung pada server broker MQTT
/// =>  subscribe pada topic yang telah di tentukan
inline static void OnMqttEventConnect(bool sessionPresent)
{
	PRINTLN(F("Connected to MQTT."));
	PRINTF("Session present: %s\n", BOOLSTR(sessionPresent));
	subscribe(TOPIC_CAPTURE, DEFAULT_QOS);
	subscribe(TOPIC_COUNT_OF_CAR, DEFAULT_QOS);
}

/// callback event saat Mqtt client terputus dari sambungan mqtt server broker
/// =>  subscribe pada topic yang telah di tentukan
inline static void OnMqttEventDisconnect(AsyncMqttClientDisconnectReason reason)
{
	// if debug print reason why disconnected
	PRINT_MQTT_DISCONNECT_REASON(reason);
	if (WiFi.isConnected())
		START_TIMER(mqttReconnectTimer, []() { mqttClient.connect(); });
}

/// callback event saat mqtt client mendapatkan message publish dari broker
/// => check apakah topic sesuai dengan yang dibutuhkan dan memproses payload
/// nya
inline static void OnMqttEventMessage(char *topic, char *payload,
				      AsyncMqttClientMessageProperties prop,
				      size_t len, size_t index, size_t total)
{
	/// early return if payload is null
	if (payload == nullptr)
		return;

	static char tempbuf[32] = {0};
	memcpy(tempbuf, payload, len);
	tempbuf[len] = '\0';
	/// compare each topics
	if (0 == strcmp(topic, TOPIC_COUNT_OF_CAR)) {
		/// parse string payload into unsigned integer
		// check if ppayload greater than 5
		auto isgreather = (atoi(tempbuf) > 5);
		// check if currentState is only on state green
		auto isgreen = (currentState == StateType::GREEN);
		/// and check all the condition
		isCarGreaterThanFive = (isgreen && isgreather);
	}
	if (0 == strcmp(topic, TOPIC_CAPTURE)) {
		/// do something for capture
	}

	/// if debug, print all the passed message parameter
	PRINTLN(F("OnMessage:"));
	PRINTF("\ttopic   : %s\r\n", topic);
	PRINTF("\tpaylodd : %s\r\n", tempbuf);
	PRINTF("\tqos     : %u\r\n", prop.qos);
	PRINTF("\tretain  : %s\r\n", BOOLSTR(prop.qos));
	PRINTF("\tlen     : %zu\r\n", len);
	PRINTF("\tindex   : %zu\r\n", index);
	PRINTF("\ttotal   : %zu\r\n", total);
}

inline static void ledOnState(const uint8_t red, const uint8_t yellow,
			      const uint8_t green)
{
	digitalWrite(RED_LED, red);
	digitalWrite(YELLOW_LED, yellow);
	digitalWrite(GREEN_LED, green);
}

inline static void publish_message_capture()
{
	if (mqttClient.connected() &&
	    (elapsedTime >= INTERVALS[StateType::CAPTURE])) {
		mqttClient.publish(TOPIC_CAPTURE, DEFAULT_QOS, DEFAULT_RETAIN,
				   TOPIC_CAPTURE, sizeof(TOPIC_CAPTURE));
	}
}

void setup()
{
	SERIAL_BEGIN(115200);
	/// LED begin ( set pinMode )
	pinMode(RED_LED, OUTPUT);
	pinMode(YELLOW_LED, OUTPUT);
	pinMode(GREEN_LED, OUTPUT);

	CREATE_RECONNECT_CALLBACK(
	    wifiReconnectTimer, "wifiTimer",
	    [](Handler_t t_) { WiFi.begin(SECRET_SSID, SECRET_PASSWORD); });
	CREATE_RECONNECT_CALLBACK(mqttReconnectTimer, "mqttTimer",
				  [](Handler_t _t) { mqttClient.connect(); });
	/// start service ( mencoba connecting ke wifi and broker mqtt )
	mqttClient
	    // set client id
	    .setClientId(MQTT_CLIENT_ID)
	    // callback event jika mqtt connected
	    .onConnect(&OnMqttEventConnect)
	    // calbback event jika mqtt disconnected
	    .onDisconnect(&OnMqttEventDisconnect)
	    // callback event jika mqtt menerima
	    .onMessage(&OnMqttEventMessage)
	    /// will message adalah saat terjadi kesalahan oleh client ini,
	    /// client ini akan mengirimkan will message ini, payloadnya isinya
	    /// adalah MQTT_CLIENT_ID dari client ini informasi lebih lengkap:
	    /// https://www.emqx.com/en/blog/use-of-mqtt-will-message#overview
	    .setWill("willtopic", DEFAULT_QOS, DEFAULT_RETAIN, MQTT_CLIENT_ID,
		     sizeof(MQTT_CLIENT_ID))
	    // set server pada client (host broker and port)
	    .setServer(MQTT_SERVER, MQTT_PORT)
	    // set username and pasword (default null)
	    .setCredentials(MQTT_USERNAME, MQTT_PASSWORD);

	PRINTLN(F("Connect to Wifi"));
	// connect to wifi
	WiFi.begin(SECRET_SSID, SECRET_PASSWORD);

#if !RELEASE
	/// precicion timer to print state each seconds
	printerStateTimer.attach_ms(
	    1000, []() { PRINT_STATE(currentState, elapsedTime); });
#endif
}

void loop()
{
	/// update state dan interval dengan millis
	startTime = millis();
	// Decode and execute the current state
	switch (currentState) {
	case StateType::GREEN:
		ledOnState(0, 0, 1);

		if (elapsedTime >= INTERVALS[StateType::GREEN]) {
			elapsedTime = 0;
			lastState = StateType::GREEN;
			nextState = (isCarGreaterThanFive == true)
					? StateType::ISFIVE
					: StateType::YELLOW;

			if (mqttClient.connected()) {
				mqttClient.publish(TOPIC_COUNT_OF_CAR,
						   DEFAULT_QOS, DEFAULT_RETAIN,
						   "0", sizeof("0"));
				isCarGreaterThanFive = false;
			}
		}
		break;

	case StateType::ISFIVE: /// state saat terdeteksi 5 mobil
		ledOnState(0, 0, 1);
		if (elapsedTime >= INTERVALS[StateType::ISFIVE]) {
			elapsedTime = 0;
			nextState = StateType::YELLOW;
		}
		break;

	case StateType::YELLOW:
		ledOnState(0, 1, 0);
		if (elapsedTime >= INTERVALS[StateType::YELLOW]) {
			elapsedTime = 0;
			nextState = (lastState == StateType::RED)
					? StateType::GREEN
					: StateType::RED;
		}
		break;

	case StateType::RED:
		ledOnState(1, 0, 0);
		publish_message_capture();

		if (elapsedTime >= INTERVALS[StateType::RED]) {
			elapsedTime = 0;
			lastState = StateType::RED;
			nextState = StateType::YELLOW;
		}
		break;

	default:
		break;
	}
	currentState = nextState;
	elapsedTime += millis() - startTime;
}
