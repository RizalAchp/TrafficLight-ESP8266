#include <config.hpp>

#define MAX_PAYLOAD_BUFFER_SIZE 32
/// client mqtt object
AsyncMqttClient mqttClient;
/// ticker untuk callback reconnect
Ticker mqttReconnectTimer;
Ticker wifiReconnectTimer;
Ticker StateTimer;

/// variable to compare time elapsed in second
/// (karena tidak akan melebihi 256 second, jadi saya menggunakan uint8_t)
uint8_t elapsedTime = 0;
/// state variable
StateType currentState = StateType::GREEN;
StateType lastState = StateType::YELLOW;

/// variable to check if car is greater than 5
uint8_t count_of_car = 0;
bool isCaptured = false;

/// callback event saat Wifi mendapatkan STA IP dari AccessPoint Wifi
/// (connected)
/// =>  mencoba untuk connect pada broker MQTT server
WiFiEventHandler wifiConnectHandler(
    WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &ev) {
	    PRINTLN(F("CONNECTED TO WIFI!"));
	    PRINTF("IP 		: %s\r\n", ev.ip.toString().c_str());
	    PRINTF("MASK	: %s\r\n", ev.mask.toString().c_str());
	    PRINTF("GATEAWAY: %s\r\n", ev.gw.toString().c_str());
	    mqttClient.connect();
    }));

/// callback event saat WiFi terputus (disconnected)
/// =>  mencoba untuk reconnect menggunakan timer callback
///     dan detach timer mqtt agar tidak reconnect pada broker saat wifi belum
///     tersedia
WiFiEventHandler wifiDisconnectHandler(WiFi.onStationModeDisconnected(
    [](const WiFiEventStationModeDisconnected &ev) {
	    PRINT_WIFI_DISCONNECT_REASON(ev);
	    // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
	    mqttReconnectTimer.detach();
	    // pass lamda to wifiReconnectTimer to call WiFi.begin(..)
	    wifiReconnectTimer.once(
		2, []() { WiFi.begin(SECRET_SSID, SECRET_PASSWORD); });
    }));

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

	// connect to wifi
	WiFi.begin(SECRET_SSID, SECRET_PASSWORD);

	/// precicion timer to print state each seconds
	StateTimer.attach_ms(1000, []() {
		// Decode and execute the current state
		switch (currentState) {
		case StateType::GREEN:
			STATE_OUTPUT_CHANGE(0, 0, 1);
			if (elapsedTime >= INTERVALS[StateType::GREEN]) {
				elapsedTime = 0;
				lastState = StateType::GREEN;

				currentState = (count_of_car > 5)
						   ? StateType::ISFIVE
						   : StateType::YELLOW;

				/// hanya memastikan untuk mereset value saat
				/// capture terkirim
				count_of_car = 0;
				isCaptured = false;
			}
			break;

		case StateType::ISFIVE: /// state saat terdeteksi 5 mobil
			STATE_OUTPUT_CHANGE(0, 0, 1);
			if (elapsedTime >= INTERVALS[StateType::ISFIVE]) {
				elapsedTime = 0;
				currentState = StateType::YELLOW;
			}
			break;

		case StateType::YELLOW:
			STATE_OUTPUT_CHANGE(0, 1, 0);
			if (elapsedTime >= INTERVALS[StateType::YELLOW]) {
				elapsedTime = 0;
				currentState = (lastState == StateType::RED)
						   ? StateType::GREEN
						   : StateType::RED;
			}
			break;

		case StateType::RED:
			STATE_OUTPUT_CHANGE(1, 0, 0);
			if ((elapsedTime == INTERVALS[StateType::CAPTURE])) {
				mqttClient.publish(
				    TOPIC_CAPTURE, DEFAULT_QOS, DEFAULT_RETAIN,
				    TOPIC_CAPTURE, sizeof(TOPIC_CAPTURE));
			}

			if (elapsedTime >= INTERVALS[StateType::RED]) {
				elapsedTime = 0;
				lastState = StateType::RED;
				currentState = StateType::YELLOW;
			}
			break;

		default:
			break;
		}
		elapsedTime++;

#if RELEASE == 0
		PRINT(NAME_STATE_CONSTANT[currentState]);
		PRINTF(" State: %d second remaining",
		       (INTERVALS[currentState] - elapsedTime));
		PRINTLN();
#endif
	});
}

/// loop gk kepake, karena sebelum e saat di test di esp32,  pake loop, banyak
/// bugnya, yang tiba tiba hard reset sendiri, maupun freeze
/// jadi untuk state nya, pake timer ticker (didalam setup())
void loop() {}

void OnMqttEventConnect(bool sessionPresent)
{
	PRINTLN("Connected to MQTT.");
	PRINTF("Session present: %s\r\n", BOOLSTR(sessionPresent));
	auto _id1 = mqttClient.subscribe(TOPIC_CAPTURE, DEFAULT_QOS);
	PRINTF("subs topic '%s', QoS 0, id: %d\r\n", TOPIC_CAPTURE, _id1);
	(void)(_id1); // ignore warning unused variable on release
	auto _id2 = mqttClient.subscribe(TOPIC_COUNT_OF_CAR, DEFAULT_QOS);
	PRINTF("subs topic '%s', QoS 0, id: %d\r\n", TOPIC_COUNT_OF_CAR, _id2);
	(void)(_id2); // ignore warning unused variable on release
}

void OnMqttEventDisconnect(AsyncMqttClientDisconnectReason reason)
{
	// if debug print reason why disconnected
	PRINT_MQTT_DISCONNECT_REASON(reason);
	if (WiFi.isConnected())
		mqttReconnectTimer.once(2, []() { mqttClient.connect(); });
}

void OnMqttEventMessage(char *topic, char *payload,
			AsyncMqttClientMessageProperties properties, size_t len,
			size_t index, size_t total)
{
	/// early return if payload is null
	if (payload == nullptr)
		return;

	static char tempbuf[MAX_PAYLOAD_BUFFER_SIZE] = {0};
	strncpy(tempbuf, payload, len);
	/// compare each topics
	if (0 == strcmp(topic, TOPIC_COUNT_OF_CAR)) {
		/// parse string payload into unsigned integer
		// check if ppayload greater than 5
		count_of_car = atoi(tempbuf);
	}
	if (0 == strcmp(topic, TOPIC_CAPTURE)) {
		/// do something for capture
		isCaptured = (0 == strcmp(tempbuf, TOPIC_CAPTURE));
	}

	/// if debug, print all the passed message parameter
	PRINTLN("OnMessage:");
	PRINTF("\ttopic   : %s\r\n", topic);
	PRINTF("\tpayload : %s\r\n", tempbuf);
	PRINTF("\tqos     : %u\r\n", properties.qos);
	PRINTF("\tretain  : %s\r\n", BOOLSTR(properties.qos));
	PRINTF("\tlen     : %zu\r\n", len);
	PRINTF("\tindex   : %zu\r\n", index);
	PRINTF("\ttotal   : %zu\r\n", total);
}