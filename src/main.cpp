#include <Arduino.h>
#include <config.hpp>

/// client mqtt object
AsyncMqttClient mqttClient;

Ticker wifiReconnectTimer;
Ticker mqttReconnectTimer;
Ticker StateTimer;

/// variable time elapsed in second
uint8_t elapsedTime = 0;
/// state variable
StateType currentState = GREEN;
StateType lastState = GREEN;
/// variable to check if car is greater than 5
bool isCarGreaterThanFive = false;

void setup()
{
	SERIAL_BEGIN(115200);
	/// LED begin ( set pinMode )
	pinMode(RED_LED, OUTPUT);
	pinMode(YELLOW_LED, OUTPUT);
	pinMode(GREEN_LED, OUTPUT);

	WiFi.onEvent(
	    [](const WiFiEvent_t event, const WiFiEventInfo_t eventinfo) {
		    PRINTF("[WiFi-event] event: %d\r\n", event);
		    switch (event) {
		    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
			    PRINTF("WiFi connected... IP address: %s\r\n",
				   WiFi.localIP().toString().c_str());
			    mqttClient.connect();
			    break;
		    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
			    PRINTLN("WiFi lost connection");

			    mqttReconnectTimer.detach();
			    wifiReconnectTimer.once(2, []() {
				    WiFi.begin(SECRET_SSID, SECRET_PASSWORD);
			    });
			    break;
		    default:
			    break;
		    }
	    });
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
	    /// client ini akan mengirimkan will message ini, payloadnya
	    /// isinya adalah MQTT_CLIENT_ID dari client ini informasi lebih
	    /// lengkap:
	    /// https://www.emqx.com/en/blog/use-of-mqtt-will-message#overview
	    .setWill("willtopic", DEFAULT_QOS, DEFAULT_RETAIN, MQTT_CLIENT_ID,
		     sizeof(MQTT_CLIENT_ID))
	    // set server pada client (host broker and port)
	    .setServer(MQTT_SERVER, MQTT_PORT)
	    // set username and pasword (default null)
	    .setCredentials(MQTT_USERNAME, MQTT_PASSWORD);

	// connect to wifi
	WiFi.begin(SECRET_SSID, SECRET_PASSWORD);

	/// menggunakan ticker timer per detik untuk menentukan state lampu
	/// untuk mendapatkan elapsedtime yang tepat
	///
	/// sebelum nya sudah pake loop, dan menggunakan millis(). namun
	/// terdapat banyak bug, dimana esp nya freeze atau bahkan esp nya hard
	/// reset sendiri
	StateTimer.attach_ms(1000, []() {
#if RELEASE == 0
		PRINT(NAME_STATE_CONSTANT[currentState]);
		PRINTF(" State: %u s", (INTERVALS[currentState] - elapsedTime));
		PRINTLN();
#endif
		switch (currentState) {
		case GREEN:
			TRAFFIC_LIGHT_STATE(0, 0, 1);
			if (elapsedTime >= INTERVALS[GREEN]) {
				elapsedTime = 0;
				lastState = GREEN;
				currentState = (isCarGreaterThanFive == true)
						   ? ISFIVE
						   : YELLOW;
				isCarGreaterThanFive = false;
			}
			break;

		case ISFIVE: /// state saat terdeteksi 5 mobil
			TRAFFIC_LIGHT_STATE(0, 0, 1);
			if (elapsedTime >= INTERVALS[ISFIVE]) {
				elapsedTime = 0;
				currentState = YELLOW;
			}
			break;

		case YELLOW:
			TRAFFIC_LIGHT_STATE(0, 1, 0);
			if (elapsedTime >= INTERVALS[YELLOW]) {
				elapsedTime = 0;
				currentState =
				    (lastState == GREEN) ? RED : GREEN;
			}
			break;

		case RED:
			TRAFFIC_LIGHT_STATE(1, 0, 0);
			if (elapsedTime == INTERVALS[CAPTURE]) {
				mqttClient.publish(
				    TOPIC_CAPTURE, DEFAULT_QOS, DEFAULT_RETAIN,
				    TOPIC_CAPTURE, sizeof(TOPIC_CAPTURE));
			}
			if (elapsedTime >= INTERVALS[RED]) {
				elapsedTime = 0;
				lastState = RED;
				currentState = YELLOW;
			}
			break;

		default:
			break;
		}
		elapsedTime++;
	});
}

/// sudah coba menggunakan loop untuk menjalankan intruksi, tetapi banyak BUG
/// nya.. namun jika anda hanya menjalankan intruksi yang ringan, tidak apa
/// didalam loop()
void loop()
{ /* QOSONQ */
}

/// callback event saat Mqtt client telah tersambung pada server broker MQTT
/// =>  subscribe pada topic yang telah di tentukan
void OnMqttEventConnect(bool sessionPresent)
{
	IGNORE(sessionPresent);
	PRINTLN(F("Connected to MQTT..."));

	SUBSCRIBE_MQTT(TOPIC_CAPTURE, DEFAULT_QOS);
	SUBSCRIBE_MQTT(TOPIC_COUNT_OF_CAR, DEFAULT_QOS);
}

/// callback event saat Mqtt client terputus dari sambungan mqtt server broker
/// =>  subscribe pada topic yang telah di tentukan
void OnMqttEventDisconnect(AsyncMqttClientDisconnectReason reason)
{
	// if debug print reason why disconnected
	PRINT_MQTT_DISCONNECT_REASON(reason);
	if (WiFi.isConnected())
		mqttReconnectTimer.once(2, []() { mqttClient.connect(); });
}

/// callback event saat mqtt client mendapatkan message publish dari broker
/// => check apakah topic sesuai dengan yang dibutuhkan dan memproses payload
/// nya
void OnMqttEventMessage(const char *topic, const char *payload,
			const AsyncMqttClientMessageProperties prop,
			const size_t len, const size_t index,
			const size_t total)
{
	/// early return if payload is null
	if (payload == nullptr)
		return;

	char payloadBuffer[MAX_PAYLOAD_BUFFER_SIZE] = {0};
	strncpy(payloadBuffer, payload, len);
	/// compare each topics
	if (0 == strcmp(topic, TOPIC_COUNT_OF_CAR)) {
		/// parse string payload into unsigned integer
		// check if ppayload greater than 5
		auto isgreather = (atoi(payloadBuffer) > 5);
		// check if currentState is only on state green
		auto isgreen = (currentState == GREEN);
		/// and check all the condition
		isCarGreaterThanFive = (isgreen && isgreather);
	}
	if (0 == strcmp(topic, TOPIC_CAPTURE)) {
		/// do something for capture
	}

	/// if debug, print all the passed message parameter
	PRINTLN(F("OnMessage:"));
	PRINTF("\ttopic   : %s\r\n", topic);
	PRINTF("\tpaylodd : %s\r\n", payloadBuffer);
	PRINTF("\tqos     : %u\r\n", prop.qos);
	PRINTF("\tretain  : %s\r\n", BOOLSTR(prop.qos));
	PRINTF("\tlen     : %zu\r\n", len);
	PRINTF("\tindex   : %zu\r\n", index);
	PRINTF("\ttotal   : %zu\r\n", total);
}