
 /* 3 channel high speed adc 
  *  this v3 include serial.read() to change the parameter of treshold
  *  merge with wifi part
  */

#include <WiFi.h>
#include <SPI.h> 
#include <ArduinoHttpClient.h>
 
#define SAMPLES 22500
#define CHANNELS 2 // channels(c0,c1,c2,c3) for buffer
#define RED 6
#define GREEN 5
#define SYSRESETREQ    (1<<2)
#define VECTKEY        (0x05fa0000UL)
#define VECTKEY_MASK   (0x0000ffffUL)
#define AIRCR          (*(uint32_t*)0xe000ed0cUL) // fixed arch-defined address
#define REQUEST_EXTERNAL_RESET (AIRCR=(AIRCR&VECTKEY_MASK)|VECTKEY|SYSRESETREQ)

long randNumber; //for generating random number

File logFile;
String infile="";
#define FTPWRITE

File fh;
//You will use TP-LINK (configured as Access Point)
char ssid[] = "edmi";     //  your network SSID (name) 
char pass[] = "jesusisback";  // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status


// change to your server
IPAddress server( 192, 168, 43, 147); //ip address of your server 
int port = 3000;
                                      
                                      
WiFiClient wifi;
HttpClient client = HttpClient(wifi, server, port);

char outBuf[128];
char outCount;
String fileName=""; //declare filename as a string, random generate its name and concatenate with ".txt"

uint16_t values[SAMPLES][CHANNELS];
unsigned int i,j,l;
uint16_t Buff_Temp;
uint16_t treshold = 100;
unsigned int rms;
float rms2=0;
float rmsmean[10];
float rmsmean2;
float meanrms;
unsigned long start_time;
unsigned long stop_time;
float rmscalc(uint16_t valbuf[SAMPLES][CHANNELS],int ch);
void printbuffer(char channel);
void calcmean(float val);
char k=0;
String inString = "";

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200); 
  l=0;
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  randomSeed(analogRead(0)); 
  pinMode(10,OUTPUT);
  digitalWrite(10,HIGH);
  ADC->ADC_CHER = 0xE0; //enable channels 7, 6, 5 (Arduino pins A0, A1, A2)
  ADC->ADC_CR=2;
  
  REG_ADC_MR = (REG_ADC_MR & 0xFFF0FFFF) | 0x00020000; //ADC STARTUP to 768. 768 / 21MHz = 36.6uS startup time
  REG_ADC_MR = (REG_ADC_MR & 0xFFFFFF0F) | 0x00000080; //enable FREERUN mode
  analogReadResolution(12); //resolution to 12bits

  /*if(SD.begin(4) == 0)
  {
    Serial.println(F("SD init fail"));          
  }*/
  
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    // don't continue:
    while(true);
  } 
  
 // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) { 
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:    
    status = WiFi.begin(ssid, pass);

    // wait 1 seconds for connection:
    delay(100);
  }
   
  // you're connected now, so print out the data:
  Serial.print("You're connected to the network");
  printCurrentNet();
  printWifiData();
  Serial.print("Server:");
  Serial.println(server);
    
}

void loop() {
  l++;
  for(i=0;i<SAMPLES;i++){
    while((ADC->ADC_ISR & 0x80)==0);  //wait until channel 3 is complete                        
    values[i][0]=ADC->ADC_CDR[7];
    values[i][1]=ADC->ADC_CDR[6];
    Buff_Temp=ADC->ADC_CDR[5];
    
  }
  //rms=rmscalc(values,0);  
  
  //start_time = micros(); 
  rms2=(rmscalc(values,0));
  calcmean(rms2);
  //calculating mean rms, basically rms2 will be put in 10 buffer than mean value will be obtained
 
  if(l>10){
    digitalWrite(GREEN, HIGH);
    if (rms2>treshold) {
      //printbuff(0);
      digitalWrite(GREEN, LOW);
      digitalWrite(RED, HIGH);
    randNumber=random(100000); //generate a random number

    Serial.println(randNumber);
    fileName=String(randNumber)+".txt"; //fileName="randNumber.txt"
    start_time=micros();
    stop_time=micros();
    Serial.println("time writing:");
    Serial.println((float)(stop_time-start_time)/1000);
    if(sendData()) Serial.println(F("POST OK"));
    else Serial.println(F("POST FAIL"));
    delay(5000);
}
  //Serial.println(stop_time-start_time); 
  //Serial.print("Average time per conversion: ");
  //Serial.println((float)(stop_time-start_time)/SAMPLES);

  if (Serial.available() > 0) {
    int inChar = Serial.read();
    if (isDigit(inChar)) {
      // convert the incoming byte to a char and add it to the string:
      inString += (char)inChar;
    }
    // if you get a newline, print the string, then the string's value:
    if (inChar == '\n') {
      treshold=inString.toInt();
      Serial.print("treshold");
      Serial.println(treshold);
      // clear the string for new input:
      inString = "";
    }
  }
  if (l>100000) l=20;
  }
} 
void printbuff(char channel){
  Serial.print("current:  ");
  Serial.println(channel);
  for(j=0;j<SAMPLES;j++)
  {
  Serial.print(values[j][channel]);
  Serial.print(";");
  }
  Serial.println(";");
  Serial.println("accoustic: 0 ");
  Serial.println("temperature: 0");
  
}
float rmscalc(uint16_t valbuf[SAMPLES][CHANNELS],int ch){
    
    uint32_t sum=0;
    uint64_t sumsquares=0;
    int32_t val;
    int32_t mean;
    //calculate mean
        
    for(j=0;j<SAMPLES;j++)
    {
    val=(int32_t)valbuf[j][ch];
    sum += (uint32_t)val;
    }
    mean=sum/SAMPLES;
    // Serial.println(mean);
    //calculate rms
    for(j=0;j<SAMPLES;j++)
    {
    val=(int32_t)valbuf[j][ch];
    sumsquares += (uint64_t)(val-mean)*(val-mean);
    }
    //Serial.println(sumsquares);
    return (sqrt(sumsquares/SAMPLES)*3300/4096);
  }
  void calcmean(float val){
  rmsmean[k]=val;
  k++;
  //Serial.println(val);
  if (k>9){
    k=0;
    rmsmean2=0;
  for(i=0;i<10;i++){
    rmsmean2+=rmsmean[i];
    
  }
  meanrms=rmsmean2/10;
   //stop_time = micros(); 
  Serial.print("current:");
  Serial.println(meanrms);
  }}

bool sendData() {
  if (!client.connect(server,port)) {
    Serial.println(F("Command connection failed"));
    return false;
  }
  
  Serial.println(F("Command connected"));

  

  return true;
}

void printWifiData() {
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println(ip);
  
  // print your MAC address:
  byte mac[6];  
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  Serial.print(mac[5],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.println(mac[0],HEX);
 
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);    
  Serial.print("BSSID: ");
  Serial.print(bssid[5],HEX);
  Serial.print(":");
  Serial.print(bssid[4],HEX);
  Serial.print(":");
  Serial.print(bssid[3],HEX);
  Serial.print(":");
  Serial.print(bssid[2],HEX);
  Serial.print(":");
  Serial.print(bssid[1],HEX);
  Serial.print(":");
  Serial.println(bssid[0],HEX);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption,HEX);
  Serial.println();
}
  

