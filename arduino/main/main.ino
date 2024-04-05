// user-facing transaction state
// -1: failed
// 0: idle
// 1: initialized
// 2: started
// 3: created
// 4: approved

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <string.h>


#define LED_BUILTIN 2
#define BUTTON_BUILTIN 0

// WiFi
const char *ssid = "Konskuy";        // Enter your WiFi name
const char *password = "konskuyyy";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "broker.hivemq.com";
const int mqtt_port = 1883;
const char *mqtt_username = "averrous";
const char *mqtt_password = "test";
char s[50];

uint32_t chipId = 0;

bool isRegistering = false;

int userId = -1;

class ESPState {
private:
  int highDuration;
  int lowDuration;
  int transactionState;
  int minimumDuration;
  int refId;

public:
  ESPState() {
    this->highDuration = 0;
    this->lowDuration = 0;
    this->transactionState = 0;
    this->minimumDuration = 0;
    this->refId = -1;
  }
  ESPState(int highDuration, int lowDuration) {
    this->highDuration = highDuration;
    this->lowDuration = lowDuration;
    this->transactionState = 0;
    this->minimumDuration = 3000;
    this->refId = -1;
  }

  ESPState(int highDuration, int lowDuration, int transactionState) {
    this->highDuration = highDuration;
    this->lowDuration = lowDuration;
    this->transactionState = transactionState;
    this->minimumDuration = 3000;
    this->refId = -1;
  }

  ESPState(int highDuration, int lowDuration, int transactionState, int minimumDuration) {
    this->highDuration = highDuration;
    this->lowDuration = lowDuration;
    this->transactionState = transactionState;
    this->minimumDuration = minimumDuration;
    this->refId = -1;
  }

  ESPState(int highDuration, int lowDuration, int transactionState, int minimumDuration, int refId) {
    this->highDuration = highDuration;
    this->lowDuration = lowDuration;
    this->transactionState = transactionState;
    this->minimumDuration = minimumDuration;
    this->refId = refId;
  }


  int getHighDuration() {
    return highDuration;
  }

  void setRefId(int refId) {
    this->refId = refId;
  }

  int getRefId() {
    return this->refId;
  }

  int getLowDuration() {
    return lowDuration;
  }

  int getMinimumDuration() {
    return this->minimumDuration;
  }

  int getState() {
    return transactionState;
  }
};

class ESPStateManager {
private:
  ESPState state;
  ESPState defferedState;
  int lastStateUpdate;
  bool isQueued;
public:
  ESPStateManager() {
    this->state = ESPState(0, 0, 0, 0, -1);
    this->defferedState = ESPState(0, 0, 0, 0, -1);
    this->lastStateUpdate = millis();
    this->isQueued = false;
  }

  int getState() {
    return this->state.getState();
  }

  int getRefId() {
    return this->state.getRefId();
  }

  void nextState(int stateCode) {
    if (stateCode == 0) {
      this->defferedState = ESPState(0, 0, 0, 0, -1);
    } else if (stateCode == 1) {
      this->defferedState = ESPState(1000, 1000, 1, 0, this->state.getRefId());
    } else if (stateCode == 2) {
      this->defferedState = ESPState(1000, 1000, 2, 0, this->state.getRefId());
    } else if (stateCode == -1) {
      this->defferedState = ESPState(100, 100, -1, 5000, this->state.getRefId());
    } else if (stateCode == 3) {
      this->defferedState = ESPState(500, 0, 3, 0, this->state.getRefId());
    } else if (stateCode == 4) {
      this->defferedState = ESPState(500, 0, 4, 5000, this->state.getRefId());
    }

    this->isQueued = true;

    if (this->state.getMinimumDuration() == 0) {
      this->state = this->defferedState;
      this->isQueued = false;
      this->lastStateUpdate = millis();

      Serial.print("Current state: ");
      Serial.println(this->getState());
    }
  }

  void nextState(int stateCode, int refId) {
    if (stateCode == 0) {
      this->defferedState = ESPState(0, 0, 0, 0, -1);
    } else if (stateCode == 1) {
      this->defferedState = ESPState(1000, 1000, 1, 0, this->state.getRefId());
    } else if (stateCode == 2) {
      this->defferedState = ESPState(1000, 1000, 2, 0, this->state.getRefId());
    } else if (stateCode == -1) {
      this->defferedState = ESPState(200, 100, -1, 3000, this->state.getRefId());
    } else if (stateCode == 3) {
      this->defferedState = ESPState(500, 0, 3, 0, this->state.getRefId());
    } else if (stateCode == 4) {
      this->defferedState = ESPState(500, 0, 4, 3000, this->state.getRefId());
    }

    this->isQueued = true;
    this->defferedState.setRefId(refId);

    if (this->state.getMinimumDuration() == 0) {
      this->state = this->defferedState;
      this->isQueued = false;
      this->lastStateUpdate = millis();

      Serial.print("Current state: ");
      Serial.println(this->getState());
    }
  }

  int getHighDuration() {
    return this->state.getHighDuration();
  }

  int getLowDuration() {
    return this->state.getLowDuration();
  }

  void loop() {
    if (this->getState() == 1 && millis() - this->lastStateUpdate > 5000) {
      this->nextState(-1);
      this->nextState(0);
    }
    if (!this->isQueued) {
      return;
    }

    if (millis() - this->lastStateUpdate > this->state.getMinimumDuration()) {
      this->state = this->defferedState;
      this->isQueued = false;
      this->lastStateUpdate = millis();
      Serial.print("Current state: ");
      Serial.println(this->getState());
    }


  }
};

ESPStateManager stateManager;

void buttonPressedHandler() {
  if (stateManager.getState() == 0) {
    startTransaction();
    stateManager.nextState(1);
  }
}


WiFiClient espClient;
PubSubClient client(espClient);


void connectWifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the Wi-Fi network");
}

void connectMQTT() {
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Public EMQX MQTT broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

int buttonState;            // the current reading from the input pin
int lastButtonState = LOW;  // the previous reading from the input pin

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 100;    // the debounce time; increase if the output flickers

void initializeButton() {
  pinMode(BUTTON_BUILTIN, INPUT);
}

void debounce(std::function<void()> handler) {
  int reading = digitalRead(BUTTON_BUILTIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        handler();
      }
    }
  }
  lastButtonState = reading;
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  digitalWrite(LED_BUILTIN, LOW);
  connectWifi();
  connectMQTT();
  initializeButton();
  pinMode(LED_BUILTIN, OUTPUT);
  sprintf(s, "register/status/%i", chipId);
  client.subscribe(s);
}

void callback(char *topic, byte *payload, unsigned int length) {
  JsonDocument message;
  JsonDocument approvalPayload;

  deserializeJson(message, payload);
  sprintf(s, "payment/%i/status", userId);
  if (strcmp(topic, s) == 0) {
    const char *transactionStatus = message["transaction_status"];
    Serial.println(transactionStatus);
    if (strcmp(transactionStatus, "onprogress") == 0 && stateManager.getState() == 1) {
      Serial.println("Transaction is created");
      stateManager.nextState(2);
    } else if (strcmp(transactionStatus, "finished") == 0 && stateManager.getState() == 2) {
      Serial.println("Transaction is finished");
      int refId = message["transaction_id"];
      stateManager.nextState(3, refId);
      approvalPayload["transaction_id"] = stateManager.getRefId();
      approvalPayload["approval"] = true;
      char payloadString[1000];
      serializeJsonPretty(approvalPayload, payloadString);
      sprintf(s, "payment/%i/approval", userId);
      client.publish(s, payloadString);
      stateManager.nextState(4);
      stateManager.nextState(0);
    } else if (strcmp(transactionStatus, "failed") == 0) {
      Serial.println("Transaction is failed");
      stateManager.nextState(-1);
      stateManager.nextState(0);
    }
  }

  sprintf(s, "register/status/%i", chipId);
  if (strcmp(topic, s) == 0) {
    userId = message["user_id"];
    isRegistering = false;
    Serial.print("Registered, with user id: ");
    Serial.println(userId);
    sprintf(s, "payment/%i/status", userId);
    client.subscribe(s);
  }
}

void startTransaction() {
  JsonDocument payload;
  payload["amount"] = 20000;

  char payloadString[1000];
  serializeJsonPretty(payload, payloadString);
  sprintf(s, "payment/%i", userId);
  client.publish(s, payloadString);
}


void registerUser() {
  isRegistering = true;
  JsonDocument payload;
  payload["device_id"] = chipId;

  Serial.println("Registering user...");

  char payloadString[1000];
  serializeJsonPretty(payload, payloadString);
  client.publish("register", payloadString);
}


void led() {
  if (stateManager.getHighDuration() == 0 && stateManager.getLowDuration() == 0) {
    return;
  }
  digitalWrite(LED_BUILTIN, HIGH);
  delay(stateManager.getHighDuration());
  digitalWrite(LED_BUILTIN, LOW);
  delay(stateManager.getLowDuration());
}

void loop() {
  stateManager.loop();
  client.loop();
  if (userId == -1 && !isRegistering) {
    registerUser();
    return;
  }
  debounce(buttonPressedHandler);
  led();
}