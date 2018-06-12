#include <ArduinoHttpClient.h>
#include <WiFi.h>

char ssid[] = "edmi";
char pass[] = "jesusisback";

char serverAddress[] = "192.168.43.147";
int port = 3000;

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);
int status = WL_IDLE_STATUS;
String response;
int statusCode = 0;

void setup() {
  Serial.begin(9600);
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);

    status = WiFi.begin(ssid, pass);
  }
  

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

void loop() {
  Serial.println("making POST request");
  char contentType[] = "application/octet-stream";
  int samples = 100;
  uint16_t postData[samples];
  for (int i=0; i<samples; i++) {
    postData[i] = random(1000);
  }
  int t = micros();
  client.post("/", contentType, sizeof(postData), (byte*) postData);

  statusCode = client.responseStatusCode();
  response = client.responseBody();
  client.flush();
  client.stop();
  Serial.print("Time elapsed: ");
  Serial.println((micros() - t) / 1000.);

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
  Serial.println("Wait two seconds");
  delay(2000);
}
