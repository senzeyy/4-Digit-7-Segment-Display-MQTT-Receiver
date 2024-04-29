#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <config.h>
#include <ArduinoJson.h>

WiFiClient espClient;
PubSubClient client(espClient);

int blink = 0;
int objectTemp = 0; // Globale Variable für objectTemp

//PWM configuration
const int PWM_CHANNEL = 15; //0-15 PWM-Kanäle auf ESP
const int PIN_LED = 13; //GPIO-PIN für Ausgangssignal an LED
const int PWM_FREQUENCY = 1000; //Frequenz des PWM Signals
const int PWM_RESOLUTION = 6; //Auflösung des PWM-Signals in Bits

//4-Digit-7-Segment-Display
// Pinbelegung für die Segmente des Displays
const int segmentPins[] = {17, 18, 22, 21, 19, 5, 23, 15};

// Pinbelegung für die Ziffern des Displays (Punkte)
const int digitPins[] = {2, 0, 4, 16};

// Zuordnung der Segmente zu den Ziffern (0-9), um z.B. die Zahl null zu erhalten, setzt man jedes Segment außer G und DP auf HIGH bzw. 1
const byte digitSegments[10] = {
  B1111110,  // 0
  B0110000,  // 1
  B1101101,  // 2
  B1111001,  // 3
  B0110011,  // 4
  B1011011,  // 5
  B1011111,  // 6
  B1110000,  // 7
  B1111111,  // 8
  B1111011,  // 9
};

//Methoden die in void setup verwendet werden, müssen vor void setup deklariert werden, deshalb void setup und void loop am Ende des Codes

//Initialisieren der Pins für das Display
void init_display_pins(){
  for (int i = 0; i < 8; i++) {
    pinMode(segmentPins[i], OUTPUT);
  }
  for (int i = 0; i < 4; i++) {
    pinMode(digitPins[i], OUTPUT);
  }
}

//test Methode,um zu testen ob richtig Verkabelt wurde/PINs richtig deklariert wurden. Segmente werden von A-G nach der Reihenfolge abgefahren
void testSegments() {
  // Schleife über jedes Segment
  for (int segment = 0; segment < 7; segment++) {
    // Alle Segmente ausschalten
    for (int i = 0; i < 7; i++) {
      digitalWrite(segmentPins[i], LOW);
      }

    // Aktiviere das aktuelle Segment
    digitalWrite(segmentPins[segment], HIGH);
    Serial.print("Segment: ");
    Serial.println(segment);
    Serial.print("SegmentPins: ");
    Serial.println(segmentPins[segment]);
    delay(500); // Wartezeit, um das Segment zu sehen
  }

  // Alle Segmente ausschalten
  for (int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], LOW);
  }
}



//prüft ob jedes Zeichen im String eine Zahl ist
bool isInteger(const char* str) {
  // Überprüfe jedes Zeichen im String
  for (int i = 0; str[i] != '\0'; i++) {
    // Wenn das aktuelle Zeichen keine Ziffer ist, gib false zurück
    if (str[i] < '0' || str[i] > '9') {
      return false;
    }
  }
  // Wenn alle Zeichen Ziffern sind, gib true zurück
  return true;
}


void setDigit(int number, int digit) {//Schreibt eine Zahl an die gewünschte Ziffer
  // Deaktiviere alle Ziffern
  for (int i = 0; i <= 3; i++) {
    digitalWrite(digitPins[i], HIGH);
  }

  // Aktiviere die gewünschte Ziffer
  digitalWrite(digitPins[digit], LOW);

  // Aktiviere die Segmente entsprechend der übergebenen Zahl
  byte digitPattern = digitSegments[number];
  for (int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], bitRead(digitPattern, 6 - i)); // Umgekehrte Reihenfolge der Segmente, weil sonst wird von hinten nach vorne gelesen
  }
  
}

int getNumberOfDigits(int number) {
  if (number == 0) {
    return 1; // 0 hat eine Ziffer
  }

  int count = 0;
  while (number != 0) {
    number /= 10;
    ++count;
  }
  return count;
}

void displayNumber(int number) {
  if (number < 0 || number > 9999) {
    // Das Display unterstützt nur 4-stellige Zahlen
    return;
  }

 int numDigits = getNumberOfDigits(number); // Anzahl der Ziffern in der Zahl
int divisor = pow(10, numDigits - 1); // Divisor für die linkeste Ziffer

for (int i = 0; i < numDigits; i++) {
  int digit = number / divisor; // Extrahiere die linkeste Ziffer
  setDigit(digit, i); // Setze die Ziffer an der entsprechenden Position
  number %= divisor; // Entferne die linkeste Ziffer aus der Zahl
  divisor /= 10; // Verringere den Divisor für die nächste Ziffer
  delay(1); // Kurze Verzögerung zwischen den Ziffern
}
}

//Verbindung mit WiFi aufbauen
void connectWifi() {
  WiFi.mode(WIFI_STA); //WiFi Station Mode, Alternativ WIFI_AP, dann baut ESP eigenen AP(AccesPoint) auf 
  WiFi.begin(ssid, password);
  delay(1000);
  while (WiFi.status() != WL_CONNECTED) { //während Verbindung nicht aufgebaut ist
    Serial.print(".");
    delay(1000);
  }
    Serial.print("\nWifi connected, IP of this ESP32: ");
    Serial.print(WiFi.localIP());
    Serial.print(", MAC-Adresse: ");
    Serial.println(WiFi.macAddress());
}

//MQTT
//callback Routine für MQTT-Broker
void callbackRoutine(char* topic, byte* message, unsigned int length){
 Serial.print("Message received on Topic: ");
  Serial.print(topic); // Ausgabe des empfangenen Topics
  Serial.print("  Message: ");
  
  // Parse JSON
  StaticJsonDocument<128> doc; // Adjust the size according to your JSON message size
  DeserializationError error = deserializeJson(doc, message, length);
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Check if the JSON object contains the required fields
  if (!doc.containsKey("IRSensor")) {
    Serial.println("Missing IRSensor field in JSON object");
    return;
  }

  JsonObject sensorData = doc["IRSensor"];
  if (!sensorData.containsKey("Objekt-Temperatur") || !sensorData.containsKey("Umgebungstemperatur")) {
    Serial.println("Missing temperature fields in JSON object");
    return;
  }

  // Extract temperature values
  objectTemp = sensorData["Objekt-Temperatur"];
  int ambientTemp = sensorData["Umgebungstemperatur"];

  Serial.println(objectTemp);
  // Check if objectTemp is greater than 200
  if (objectTemp > 200) {
    blink = 1;
  } else {
    blink = 0;
  }
}

//Mit MQTT-Broker verbinden
void connectMqtt() {
  Serial.println("Connecting to MQTT...");
  client.setServer(MQTT_BROKER_IP, MQTT_PORT); //MQTT-Broker und PORT über den kommuniziert wird
  
  while (!client.connected()) {
    if (!client.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.print("MQTT connection failed:");
      Serial.print(client.state());
      Serial.println("Retrying...");
      delay(MQTT_RETRY_WAIT);
    }
  }

  Serial.println("MQTT connected");
  Serial.println();

  Serial.print("\nIPv4 of MQTT-Broker: ");
  Serial.println(MQTT_BROKER_IP);

  client.setCallback(callbackRoutine); //callbackRoutine definieren, d.h. wenn Broker die Message erhalten hat und eine Antwort zurückgibt, dann wird diese Methode aufgerufen
  client.subscribe(baseTopic); //hier wird ein Topic im MQTT-Broker subscribet, das asugewählte Topic wird dann in derCallbackRoutiner übergeben
  
}

void blinkLED(){
   //Schleife um LED-Helligkeit im 1ms-Takt zu erhöhen
  for(int i=0;i<pow(2, PWM_RESOLUTION)-1;i++){
    ledcWrite(PWM_CHANNEL,i); //pwm
    displayNumber(objectTemp); //hier auch eingebaut, damit Display ohne Flackern angezeigt werden kann, während LED blinkt
    delay(1);
  }
//Schleife um LED-Helligkeit im 1ms-Takt zu reduzieren
  for(int i=pow(2, PWM_RESOLUTION)-1;i>=0;i--){
    ledcWrite(PWM_CHANNEL,i);
    displayNumber(objectTemp);
    delay(1);
  }
}

void setup()
{
  pinMode(PIN_LED, OUTPUT); //GPIO-PIN als Ausgang/Output setzen
  ledcSetup(PWM_CHANNEL,PWM_FREQUENCY,PWM_RESOLUTION); //PWM-Kanal/Signal konfigurieren
  ledcAttachPin(PIN_LED,PWM_CHANNEL); //PIN für LED-Signal an PWM-Kanal zuweisen
  Serial.begin(9600);
  init_display_pins();
  delay(2000);  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  connectWifi();
  connectMqtt();
  Serial.println("Initialisiert");
}

void loop()
{
  client.loop();
  //client.publish(baseTopic, messageBuffer);
  if(blink==0){
    displayNumber(objectTemp);
  }
  if(blink==1){
    blinkLED();
  }
}


