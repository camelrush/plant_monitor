#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// install library
//   [NTPClient] by Fabrice Weinberg ... for Getting Now Time.
//   [ArduinoJson] by Benoit Blanchon ... for making API Message.
//   [DHT118266] by AMD16 ... for DHT11 sensor.
//   [PubSubClient] by Nick O'Leary ... for MQTT Protocol.

#define PIN_MOIST 33
#define PIN_TEMP  5

#define AWS_IOT_PUBLISH_TOPIC   "moisture-sensor/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "moisture-sensor/sub"
#define THINGNAME "moisture-sensor"
#define SLEEP_SEC 600

#define DHTTYPE DHT11
#define TEMP_FAILVAL 100 
#define HUMI_FAILVAL 100 

#define MOIST_FAILVAL 2700
#define MOIST_MAXVAL 2600
#define MOIST_MINVAL 1200

const char* ssid     = "(set here your wifi ssid)";
const char* password = "(set here your wifi password)";

const char* AWS_IOT_ENDPOINT = "a3ufrbqbd4cwta-ats.iot.ap-northeast-1.amazonaws.com";

struct tm timeInfo;

// Amazon Root CA 1
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQ...

( set here root ca1 certificate. )

...68SQvvG5
-----END CERTIFICATE-----
)EOF";

// Device Certificate
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
MIIDW...

( set here device certificate. )

..........................................Bpdgw==
-----END CERTIFICATE-----

)KEY";
 
// Device Private Key
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
MIIEpA...

( set here device private key. )

..........................................t7SyA==
-----END RSA PRIVATE KEY-----
 
)KEY";

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

DHT dht(PIN_TEMP, DHTTYPE);


bool getMoisture(int *m){

  int   moist = 0;
  float val = 0;
  bool  result = false;

  // read sensor(max 3 times)
  for (int i=0; i<3; i++){
    val = analogRead(PIN_MOIST);
    if (val < MOIST_FAILVAL){
      result = true;
      break;
    }
    delay(100);
  }

  // read failed.
  if (!result) return false;

  Serial.print("Moisture(sensor): ");
  Serial.println(val);

  // fit to range.
  if (val < MOIST_MINVAL) val = MOIST_MINVAL;
  if (val > MOIST_MAXVAL) val = MOIST_MAXVAL;

  // convert to percent value.
  val = 1.0f - ((float)(val - (float)MOIST_MINVAL) 
              / (float)(MOIST_MAXVAL - MOIST_MINVAL));
  moist = round(val * 100);

  Serial.print("Moisture: ");
  Serial.println(moist);

  *m = moist;
  return true;
}


bool getTempHumi(int* t, int* h)
{

  int humi;
  int temp;
  bool result = false;
  
  // read sensor(max 3 times)
  for (int i=0; i<3 ; i++) {
    humi = dht.readHumidity();
    temp = dht.readTemperature();

    if (temp < TEMP_FAILVAL && humi < HUMI_FAILVAL)  { 
      result = true;
      break;
    }
    delay(100);
  }

  // read failed.
  if (!result) { 
    return false;
  }

  Serial.print("Humidity: ");
  Serial.print(humi);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" *C");

  *t = temp;
  *h = humi;
  return true;

}

bool connectWifi(){

  int cnt = 0;

  Serial.println();
  Serial.println("******************************************************");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      cnt++;
      if (cnt >= 10) return false; 
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  return true;

}

bool connectAWS(){

  Serial.println("Start Connecting to AWS IoT");

  // Set AWS Credentials.
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to AWS IoT.
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  // Create a message handler;
  client.setCallback(messageHandler);

  Serial.println("Connecting to AWS IoT");

  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return false;
  }

  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");

  return true;

}

void publishMsg(String dt, int m, int h, int t) 
{
  StaticJsonDocument<200> doc;

  doc["datetime_value"] = dt;
  doc["moisture"] = m;
  doc["humidity"] = h;
  doc["temperature"] = t;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  // Debug JSON in SerialConsole
  Serial.println(jsonBuffer);
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}
   
void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}

void setup() {

  // initialize port.
  Serial.begin(115200);

  // setting NTP.
  configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");

}

void loop() {

  int  h;       // Humidity
  int  t;       // Temperature
  int  m;       // Moisture
  char dt[20];  // Datetime

  // get humidity & temperature value.
  if (!getTempHumi(&t, &h)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(100);
    return;    
  }

  // get moisture percent value.
  if (!getMoisture(&m)){
    Serial.println("Failed to read from Moisture sensor!");
    delay(100);
    return;    
  }

  // connect to network.
  if (!connectWifi()) {
    Serial.println("Failed to connect network!");
    delay(100);
    return;
  }

  // get datetime JP local.
  while (!getLocalTime(&timeInfo)){
    delay(100);
  };
  sprintf(dt, "%04d%02d%02d%02d%02d%02d",
          timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
          timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
  Serial.println(dt);

  // connect to AWS IoT Core.
  if (!connectAWS()) {
    Serial.println("Failed to connect IoT Core!");
    delay(100);
    return;
  };

  // publish to AWS
  publishMsg(dt, m, h, t);

  // deep sleep
  esp_sleep_enable_timer_wakeup(SLEEP_SEC*1000000);
  esp_deep_sleep_start();  
}
