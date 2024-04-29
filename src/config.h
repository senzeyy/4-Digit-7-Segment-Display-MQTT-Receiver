//Wifi and MQTT configuration
const char* ssid = "iPhone von Kaan";
const char* password = "achim123";

const char* baseTopic = "/3006007_KE/IR-Sensor";

const int MQTT_PORT = 1883;
const char* MQTT_BROKER_IP = "172.20.10.13";
const char* MQTT_CLIENTID = "KE_MQTT-RECEIVER";
const char* MQTT_USERNAME = "";
const char* MQTT_PASSWORD = "";
const int MQTT_RETRY_WAIT = 5000;

const int gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
const char* ntpServer = "ptbtime1.ptb.de";


/*PIN-BELEGUNG des Displays, auskommentiert weil Code deutlich kürzer wird, wenn man die Werte in Arrays(siehe oben) packt
int D1 = 2;
int D2 = 0;
int D3 = 4;
int D4 = 16;

int A = 17;
int B = 18;
int C = 22;
int D = 21;
int E = 19;
int F = 5;
int G = 23;
int DP = 15; //Dezimalpunkt
*/
