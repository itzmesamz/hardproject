 /* 3 channel high speed adc 
  *  this v3 include serial.read() to change the parameter of treshold
  *  merge with wifi part
  */

 #include <WiFi.h>
 #include <SD.h>
 #include <SPI.h> 
 
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
char ssid[] = "TP-LINK_3448A4";     //  your network SSID (name) 
char pass[] = "1F3448A4";  // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status


// change to your server
IPAddress server( 192, 168, 0, 150); //ip address of your server 
                                      
                                      
WiFiClient client;
WiFiClient dclient;

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

  if(SD.begin(4) == 0)
  {
    Serial.println(F("SD init fail"));          
  }
  
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
  Serial.print("FTP server:");
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
      printbuff(0);
      digitalWrite(GREEN, LOW);
      digitalWrite(RED, HIGH);
    randNumber=random(100000); //generate a random number
    Serial.println(randNumber);
    fileName=String(randNumber)+".txt"; //fileName="randNumber.txt"

    WriteFile(); //this function writes the samples in file

    if(doFTP()) Serial.println(F("FTP OK"));
    else Serial.println(F("FTP FAIL"));
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

   void WriteFile(){
  logFile = SD.open(fileName, FILE_WRITE);
  if (logFile)
  {
    for(i=0;i<SAMPLES;i++){
      infile= String(values[i][0]) + " " + String(values[i][1]);
      //this will write samples on 4 columns
      logFile.println(infile);
      }
      logFile.close();
      delay(1000);
  }
  else
  {
      Serial.println("Couldn't open log file");
      return;
  }
      
 }



byte doFTP()
{
#ifdef FTPWRITE
  fh = SD.open(fileName,FILE_READ);
#else
  SD.remove(fileName);
  fh = SD.open(fileName,FILE_WRITE);
#endif

  if(!fh)
  {
    Serial.println(F("SD open fail"));
    return 0;    
  }

#ifndef FTPWRITE  
  if(!fh.seek(0))
  {
    Serial.println(F("Rewind fail"));
    fh.close();
    return 0;    
  }
#endif

  Serial.println(F("SD opened"));

  if (client.connect(server,21)) {  
    Serial.println(F("Command connected"));
  } 
  else {
    fh.close();
    Serial.println(F("Command connection failed"));
    return 0;
  }

  if(!eRcv()) return 0;

  client.println(F("USER device1")); //your user have to be defined in fileZilla server

  if(!eRcv()) return 0;

  client.println(F("PASS 123456")); //and also you have to define a password for yor user

  if(!eRcv()) return 0;

  client.println(F("SYST"));

  if(!eRcv()) return 0;

  client.println(F("PASV"));

  if(!eRcv()) return 0;

  char *tStr = strtok(outBuf,"(,");
  int array_pasv[6];
  for ( int i = 0; i < 6; i++) {
    tStr = strtok(NULL,"(,");
    array_pasv[i] = atoi(tStr);
    if(tStr == NULL)
    {
      Serial.println(F("Bad PASV Answer"));    

    }
  }

  unsigned int hiPort,loPort;

  hiPort = array_pasv[4] << 8;
  loPort = array_pasv[5] & 255;

  Serial.print(F("Data port: "));
  hiPort = hiPort | loPort;
  Serial.println(hiPort);

  if (dclient.connect(server,hiPort)) {
    Serial.println(F("Data connected"));
  } 
  else {
    Serial.println(F("Data connection failed"));
    client.stop();
    fh.close();
    return 0;
  }

#ifdef FTPWRITE 
  client.print(F("STOR "));
  client.println(fileName);
#else
  client.print(F("RETR "));
  client.println(fileName);
#endif

  if(!eRcv())
  {
    dclient.stop();
    return 0;
  }

#ifdef FTPWRITE
  Serial.println(F("Writing"));

  byte clientBuf[64];
  int clientCount = 0;

  while(fh.available())
  {
    clientBuf[clientCount] = fh.read();
    clientCount++;

    if(clientCount > 63)
    {
      dclient.write(clientBuf,64);
      clientCount = 0;
    }
  }

  if(clientCount > 0) dclient.write(clientBuf,clientCount);

#else
  while(dclient.connected())
  {
    while(dclient.available())
    {
      char c = dclient.read();
      fh.write(c);      
      Serial.write(c); 
    }
  }
#endif

  dclient.stop();
  Serial.println(F("Data disconnected"));
  SD.remove(fileName); //remove the file from SD
  Serial.println("The file was removed");
  delay(1000);
  REQUEST_EXTERNAL_RESET; //reset Arduino for a new upload
  Serial.println("Arduino was reseted");

  if(!eRcv()) return 0;

  client.println(F("QUIT"));

  if(!eRcv()) return 0;

  client.stop();
  Serial.println(F("Command disconnected"));

  fh.close();
  Serial.println(F("SD closed"));
  
  return 1;
}

byte eRcv()
{
  byte respCode;
  byte thisByte;

  while(!client.available()) delay(1);

  respCode = client.peek();

  outCount = 0;

  while(client.available())
  {  
    thisByte = client.read();    
    Serial.write(thisByte);

    if(outCount < 127)
    {
      outBuf[outCount] = thisByte;
      outCount++;      
      outBuf[outCount] = 0;
    }
  }

  if(respCode >= '4')
  {
    efail();
    return 0;  
  }

  return 1;
}

void efail()
{
  byte thisByte = 0;

  client.println(F("QUIT"));

  while(!client.available()) delay(1);

  while(client.available())
  {  
    thisByte = client.read();    
    Serial.write(thisByte);
  }

  client.stop();
  Serial.println(F("Command disconnected"));
  fh.close();
  Serial.println(F("SD closed"));
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
  

