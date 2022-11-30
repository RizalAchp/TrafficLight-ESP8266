#include <config.hpp>

/// define for pin led / lamp
#define RED_LED D5
#define YELLOW_LED D6
#define GREEN_LED D7

/// client mqtt object
AsyncMqttClient mqttClient;
/// ticker untuk callback reconnect
Ticker mqttReconnectTimer;
Ticker wifiReconnectTimer;

#if !RELEASE
Ticker printerStateTimer;
#endif

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

/// callback event saat Wifi mendapatkan STA IP dari AccessPoint Wifi
/// (connected)
/// =>  mencoba untuk connect pada broker MQTT server
WiFiEventHandler wifiConnectHandler(WiFi.onStationModeGotIP([](auto ev) {
	PRINTLN("CONNECTED TO WIFI!");
	PRINTF("ip: %s, mask: %s, gateway: %s\n", ev.ip.toString().c_str(),
	       ev.mask.toString().c_str(), ev.gw.toString().c_str());
	mqttClient.connect();
}));

/// callback event saat WiFi terputus (disconnected)
/// =>  mencoba untuk reconnect menggunakan timer callback
///     dan detach timer mqtt agar tidak reconnect pada broker saat wifi belum
///     tersedia
WiFiEventHandler
    wifiDisconnectHandler(WiFi.onStationModeDisconnected([](auto ev) {
	    PRINT_WIFI_DISCONNECT_REASON(ev);
	    // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
	    mqttReconnectTimer.detach();
	    // pass lamda to wifiReconnectTimer to call WiFi.begin(..)
	    wifiReconnectTimer.once(
		2, []() { WiFi.begin(SECRET_SSID, SECRET_PASSWORD); });
    }));

/// callback event saat Mqtt client telah tersambung pada server broker MQTT
/// =>  subscribe pada topic yang telah di tentukan
inline static void OnMqttEventConnect(bool sessionPresent)
{
	PRINTLN("Connected to MQTT.");
	PRINTF("Session present: %s\n", BOOLSTR(sessionPresent));
	auto _id1 = mqttClient.subscribe(TOPIC_CAPTURE, DEFAULT_QOS);
	PRINTF("Subscribing to topic '%s' with QoS 0, packetId: %d",
	       TOPIC_CAPTURE, _id1);
	(void)(_id1); // ignore warning unused variable on release
	auto _id2 = mqttClient.subscribe(TOPIC_COUNT_OF_CAR, DEFAULT_QOS);
	PRINTF("Subscribing to topic '%s' with QoS 0, packetId: %d",
	       TOPIC_COUNT_OF_CAR, _id2);
	(void)(_id2); // ignore warning unused variable on release
}

/// callback event saat Mqtt client terputus dari sambungan mqtt server broker
/// =>  subscribe pada topic yang telah di tentukan
inline static void OnMqttEventDisconnect(AsyncMqttClientDisconnectReason reason)
{
	// if debug print reason why disconnected
	PRINT_MQTT_DISCONNECT_REASON(reason);
	if (WiFi.isConnected())
		mqttReconnectTimer.once(2, []() { mqttClient.connect(); });
}

/// callback event saat mqtt client mendapatkan message publish dari broker
/// => check apakah topic sesuai dengan yang dibutuhkan dan memproses payload
/// nya
inline static void
OnMqttEventMessage(char *topic, char *payload,
		   AsyncMqttClientMessageProperties properties, size_t len,
		   size_t index, size_t total)
{
	/// compare each topics
	if (0 ==
	    strncmp(topic, TOPIC_COUNT_OF_CAR, sizeof(TOPIC_COUNT_OF_CAR))) {
		char *p;
		/// parse string payload into unsigned integer
		// check if ppayload greater than 5
		auto isgreather = (strtol(payload, &p, 10) > 5);
		// check if payload is parsed successfully
		auto converted = (*p == 0);
		// check if currentState is only on state green
		auto isgreen = (currentState == StateType::GREEN);

		/// and check all the condition
		isCarGreaterThanFive = (isgreen && isgreather && converted);
	}
	if (0 == strncmp(topic, TOPIC_CAPTURE, sizeof(TOPIC_CAPTURE))) {
		/// do something for capture
	}

	/// if debug, print all the passed message parameter
	PRINTLN("OnMessage:");
	PRINTF("\ttopic   : %s\r\n", topic);
	PRINTF("\tpaylodd : %s\r\n", payload);
	PRINTF("\tqos     : %u\r\n", properties.qos);
	PRINTF("\tretain  : %s\r\n", BOOLSTR(properties.qos));
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
	if (!mqttClient.connected() &&
	    !(elapsedTime >= INTERVALS[StateType::CAPTURE]))
		return;
	mqttClient.publish(TOPIC_CAPTURE, DEFAULT_QOS, DEFAULT_RETAIN,
			   TOPIC_CAPTURE, sizeof(TOPIC_CAPTURE));
}

void setup()
{
	SERIAL_BEGIN(115200);
	///////////////
	/// LED begin ( set pinMode )
	pinMode(RED_LED, OUTPUT);
	pinMode(YELLOW_LED, OUTPUT);
	pinMode(GREEN_LED, OUTPUT);
	///////////////
	/// start service ( mencoba connecting ke wifi and broker mqtt )
	PRINTLN("Settingup client...");
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

	PRINTLN("Connect to Wifi");
	// connect to wifi
	WiFi.begin(SECRET_SSID, SECRET_PASSWORD);

#if !RELEASE
	/// precicion timer to print state each seconds
	printerStateTimer.attach(
	    1.f, []() { PRINT_STATE(currentState, elapsedTime); });
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

            isCarGreaterThanFive = false;
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
