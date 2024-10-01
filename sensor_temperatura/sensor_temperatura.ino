#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "MAX30105.h"  // Biblioteca do sensor MAX30102

// Configurações - variáveis editáveis
const char* default_SSID = "SEU_WIFI"; // Nome da rede Wi-Fi
const char* default_PASSWORD = "SUA_SENHA_WIFI"; // Senha da rede Wi-Fi
const char* default_BROKER_MQTT = "15.228.199.49"; // IP do Broker MQTT
const int default_BROKER_PORT = 1883; // Porta do Broker MQTT
const char* default_TOPICO_PUBLISH_HEART = "/sensor/max30102/heart"; // Tópico MQTT de envio de batimentos cardíacos
const char* default_TOPICO_PUBLISH_SPO2 = "/sensor/max30102/spo2";  // Tópico MQTT de envio de SpO2
const char* default_ID_MQTT = "esp32_max30102"; // ID MQTT

// Instância do sensor MAX30105 (compatível com MAX30102)
MAX30105 particleSensor;

// Instâncias do cliente Wi-Fi e MQTT
WiFiClient espClient;
PubSubClient client(espClient);
char EstadoSaida = '0';

void initSerial() {
    Serial.begin(115200);
}

void initWiFi() {
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(default_SSID);
    Serial.println("Aguarde");
    WiFi.begin(default_SSID, default_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("Conectado com sucesso!");
    Serial.print("IP obtido: ");
    Serial.println(WiFi.localIP());
}

void initMQTT() {
    client.setServer(default_BROKER_MQTT, default_BROKER_PORT);
    client.setCallback(mqtt_callback);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) {
        char c = (char)payload[i];
        msg += c;
    }
    Serial.print("Mensagem recebida no tópico: ");
    Serial.println(msg);
}

void setup() {
    initSerial();
    initWiFi();
    initMQTT();

    // Inicializa o sensor MAX30102
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("Sensor MAX30102 não encontrado!");
        while (1);
    }

    particleSensor.setup();
    particleSensor.setPulseAmplitudeRed(0x0A);  // Configura LED vermelho para SpO2
    particleSensor.setPulseAmplitudeGreen(0);   // Desativa LED verde
    delay(5000);
}

void loop() {
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop();

    // Leitura dos dados do sensor
    long irValue = particleSensor.getIR();
    long redValue = particleSensor.getRed();

    if (irValue > 50000) {
        // Simulação de leitura de batimentos cardíacos e SpO2
        int heartRate = random(60, 100);  // Use algoritmos reais se necessário
        int spo2 = random(95, 100);       // Use algoritmos reais se necessário

        String heartRateStr = String(heartRate);
        String spo2Str = String(spo2);

        // Publica os dados via MQTT
        Serial.println("Publicando dados...");
        client.publish(default_TOPICO_PUBLISH_HEART, heartRateStr.c_str());
        client.publish(default_TOPICO_PUBLISH_SPO2, spo2Str.c_str());

        Serial.print("Batimentos: ");
        Serial.println(heartRateStr);
        Serial.print("SpO2: ");
        Serial.println(spo2Str);
    }

    delay(2000);  // Intervalo de 2 segundos entre leituras
}

void reconnectMQTT() {
    while (!client.connected()) {
        Serial.print("Tentando se conectar ao Broker MQTT...");
        if (client.connect(default_ID_MQTT)) {
            Serial.println("Conectado ao Broker MQTT!");
        } else {
            Serial.print("Falha na conexão. Código: ");
            Serial.print(client.state());
            Serial.println(" Tentando novamente em 5 segundos");
            delay(5000);
        }
    }
}
