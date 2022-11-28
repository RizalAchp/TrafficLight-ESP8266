#include <config.hpp>

#define RED_LED    D5
#define YELLOW_LED D6
#define GREEN_LED  D7

/// client mqtt object
AsyncMqttClient mqttClient;
/// ticker untuk callback reconnect
Ticker mqttReconnectTimer;
Ticker wifiReconnectTimer;
/// variable to store `millis()` value
unsigned long startTime = 0;
/// variable to compare time elapsed from `millis()`
unsigned long elapsedTime = 0;
/// state variable

StateType nextState    = StateType::STATE_GREEN;
StateType currentState = StateType::STATE_GREEN;
StateType lastState    = StateType::STATE_YELLOW;

/// variable to check if car is greater than 5
bool isCarGreaterThanFive = false;

/// callback event saat Wifi mendapatkan STA IP dari AccessPoint Wifi (connected)
/// =>  mencoba untuk connect pada broker MQTT server
inline static void OnWifiConnect(const WiFiEventStationModeGotIP &ev) {
    PRINTLN("CONNECTED TO WIFI!");
    PRINTF("ip: %s, mask: %s, gateway: %s\n", ev.ip.toString().c_str(), ev.mask.toString().c_str(),
           ev.gw.toString().c_str());
    mqttClient.connect();
}

/// callback event saat WiFi terputus (disconnected)
/// =>  mencoba untuk reconnect menggunakan timer callback
///     dan detach timer mqtt agar tidak reconnect pada broker saat wifi belum tersedia
inline static void OnWifiDisconnect(const WiFiEventStationModeDisconnected &ev) {
    PRINT_WIFI_DISCONNECT_REASON(ev);
    // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
    mqttReconnectTimer.detach();
    wifiReconnectTimer.once(2, []() { WiFi.begin(SECRET_SSID, SECRET_PASSWORD); });
}

/// callback event saat Mqtt client telah tersambung pada server broker MQTT
/// =>  subscribe pada topic yang telah di tentukan
inline static void OnMqttEventConnect(bool sessionPresent) {
    PRINTLN("Connected to MQTT.");
    PRINTF("Session present: %s\n", BOOLSTR(sessionPresent));
    auto _id1 = mqttClient.subscribe(TOPIC_CAPTURE, DEFAULT_QOS);
    PRINTF("Subscribing at topic %s QoS 0, packetId: %d", TOPIC_CAPTURE, _id1);
    (void)(_id1);  // ignore warning unused variable on release
    auto _id2 = mqttClient.subscribe(TOPIC_COUNT_OF_CAR, DEFAULT_QOS);
    PRINTF("Subscribing at topic %s QoS 0, packetId: %d", TOPIC_COUNT_OF_CAR, _id2);
    (void)(_id2);  // ignore warning unused variable on release
}

/// callback event saat Mqtt client terputus dari sambungan mqtt server broker
/// =>  subscribe pada topic yang telah di tentukan
inline static void OnMqttEventDisconnect(AsyncMqttClientDisconnectReason reason) {
    PRINT_MQTT_DISCONNECT_REASON(reason);  // if debug print reason why disconnected

    if (WiFi.isConnected()) mqttReconnectTimer.once(2, []() { mqttClient.connect(); });
}

/// callback event saat mqtt client mendapatkan message publish dari broker
/// => check apakah topic sesuai dengan yang dibutuhkan dan memproses payload nya
inline static void OnMqttEventMessage(char *topic, char *payload,
                                      AsyncMqttClientMessageProperties properties, size_t len,
                                      size_t index, size_t total) {
    /// if debug, print all the passed message parameter
    PRINTLN("OnMessage: \r\n");
    PRINTF("\ttopic   : %s\r\n", topic);
    PRINTF("\tpaylodd : %s\r\n", payload);
    PRINTF("\tqos     : %zu\r\n", properties.qos);
    PRINTF("\tretain  : %s\r\n", BOOLSTR(properties.qos));
    PRINTF("\tindex   : %zu\r\n", index);
    PRINTF("\ttotal   : %zu\r\n", total);

    /// compare each topics, if equal to our topic
    if (0 == strcmp(topic, TOPIC_CAPTURE)) {
        PRINTF("CaptureCallback: {payload: %s}", payload);
    }
    if (0 == strcmp(topic, TOPIC_COUNT_OF_CAR)) {
        PRINTF("CarCountCallback: {payload: %s}", payload);

        char *p;
        /// parse string payload into unsigned integer
        // variable to check if parsing or converting is not failed and other state
        bool isgreather = strtoul(payload, &p, 10) > 5;
        bool converted  = (*p == 0);
        bool isgreen    = (currentState == StateType::STATE_GREEN);
        /// and assign status if count_car greater than five and currentState is green
        /// if evaluate to false, assign the old value from isCarGreaterThanFive
        isCarGreaterThanFive = (isgreen && isgreather && converted) ? true : isCarGreaterThanFive;
    }
}

inline static void ledOnState(const uint8_t red, const uint8_t yellow, const uint8_t green) {
    digitalWrite(RED_LED, red);
    digitalWrite(YELLOW_LED, yellow);
    digitalWrite(GREEN_LED, green);
}

WiFiEventHandler wifiConnectHandler(WiFi.onStationModeGotIP(&OnWifiConnect));
WiFiEventHandler wifiDisconnectHandler(WiFi.onStationModeDisconnected(&OnWifiDisconnect));

void setup() {
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
        /// will message adalah saat terjadi kesalahan oleh client ini, client ini akan mengirimkan
        /// will message ini, payloadnya isinya adalah MQTT_CLIENT_ID dari client ini
        /// informasi lebih lengkap: https://www.emqx.com/en/blog/use-of-mqtt-will-message#overview
        .setWill("willtopic", DEFAULT_QOS, DEFAULT_RETAIN, MQTT_CLIENT_ID, sizeof(MQTT_CLIENT_ID))
        .setClientId(MQTT_CLIENT_ID)           // set client id
        .onConnect(&OnMqttEventConnect)        // callback event jika mqtt connected
        .onDisconnect(&OnMqttEventDisconnect)  // calbback event jika mqtt disconnected
        .onMessage(&OnMqttEventMessage)        // callback event jika mqtt menerima publish/message
        .setServer(MQTT_SERVER, MQTT_PORT)     // set server pada client (host broker and port)
        .setCredentials(MQTT_USERNAME, MQTT_PASSWORD);  // set username and pasword (default null)

    PRINTLN("Connect to Wifi");
    WiFi.begin(SECRET_SSID, SECRET_PASSWORD);  // connect to wifi
}

void loop() {
    ////////////////////////////////////////////////
    /// check apakah mqttClient terhubung pada broker
    if (mqttClient.connected()) {
        /////////////////////////////////////////////////////////////////////////
        /// check apakah state itu red dan elapsedTime 83% dari INTERVAL_RED
        /// dan akan publish message pada service mqtt dengan TOPIC dan MESSAGE capture
        if ((elapsedTime >= CAPTURE_TIME) && (currentState == StateType::STATE_RED)) {
            mqttClient.publish(TOPIC_CAPTURE, DEFAULT_QOS, DEFAULT_RETAIN, TOPIC_CAPTURE,
                               sizeof(TOPIC_CAPTURE));
        }
    }

    ///////////////////
    /// update state dan interval dengan millis
    startTime = millis();
    //////////////////////
    // Decode and execute the current state
    switch (currentState) {
        case StateType::STATE_GREEN:
            ledOnState(0, 0, 1);
            ////////////////////////////////////
            // check elapsedTime from millis tanpa menggunakan delay
            // agar tidak ngeblock loop
            if (elapsedTime >= INTERVAL_GREEN) {
                elapsedTime = 0;
                //////////////////////
                // set lastState untuk memberikan informasi pada  StateType::STATE_YELLOW
                // untuk menentukan nextState
                lastState = StateType::STATE_GREEN;
                ////////////////////////////////////////////////////
                // check apakah terdeteksi 5 mobil untuk menentukan state selanjutnya
                // menggunakan ternary operator
                nextState = (isCarGreaterThanFive == true) ? StateType::STATE_ISFIVE
                                                           : StateType::STATE_YELLOW;
            }
            break;
        case StateType::STATE_ISFIVE:  // state saat terdeteksi 5 mobil
            ledOnState(0, 0, 1);
            if (elapsedTime >= INTERVAL_ISFIVE) {
                elapsedTime = 0;
                nextState   = StateType::STATE_YELLOW;
            }
            break;
        case StateType::STATE_YELLOW:
            ledOnState(0, 1, 0);
            if (elapsedTime >= INTERVAL_YELLOW) {
                elapsedTime = 0;
                // check lastState untuk menentukan nextState pada lampu kuning
                nextState = (lastState == StateType::STATE_RED) ? StateType::STATE_GREEN
                                                                : StateType::STATE_RED;
            }
            break;
        case StateType::STATE_RED:
            ledOnState(1, 0, 0);
            if (elapsedTime >= INTERVAL_RED) {
                elapsedTime = 0;
                lastState   = StateType::STATE_RED;
                nextState   = StateType::STATE_YELLOW;
            }
            break;
        default: break;
    }

    currentState = nextState;
    elapsedTime += millis() - startTime;
}
