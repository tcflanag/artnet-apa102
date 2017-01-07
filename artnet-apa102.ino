#include <SPI.h>
#include <WiFi.h>
#include <WiFiUDP.h>

   
#define CHANNEL_PER_LED 3
  
  // WiFi network name and password:
  const char * networkName = "Fake.SSID";
  const char * networkPswd = "Fake.Pass";
  
   
   
IPAddress ip(192, 168, 1, 222);  //Edit this to your own static IP, or comment out wifi.config below
IPAddress gate(192, 168, 1, 1);
IPAddress subnet(255, 255, 255,0);
   
   
const int BUTTON_PIN = 0;
const int LED_PIN = 5;
const int SDI = 23;
const int CKI = 18;
   
WiFiUDP Udp; 

void loopTask(void *pvParameters)
{
    setup();
    for(;;) {
        loop();
    }
}

extern "C" void app_main()
{
    initArduino();
    xTaskCreatePinnedToCore(loopTask, "loopTask", 4096, NULL, 1, NULL, 0);
}

void setup()
{
    // Initilize hardware:
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    pinMode(SDI, OUTPUT);
    pinMode(CKI, OUTPUT);
    SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
    SPI.begin();
   
   
    // Connect to the WiFi network (see function below loop)
    connectToWiFi(networkName, networkPswd);
   
    digitalWrite(LED_PIN, LOW); // LED offnn
    Udp.begin(0x1936);
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED) {
    log_e("WiFi disconnect Detected");
    connectToWiFi(networkName, networkPswd);
  }
  char packetBuffer[2000];
  int noBytes = Udp.parsePacket();
  String received_command = "";
  if ( noBytes ) {

    // We've received a packet, read the data from it
    Udp.read(packetBuffer,noBytes); // read the packet into the buffer
     digitalWrite(LED_PIN, !digitalRead(LED_PIN));
          
          if (packetBuffer[0] == 'A' && //gross sanity check of Art-Net packet
              packetBuffer[1] == 'r' &&
              packetBuffer[2] == 't' &&
              packetBuffer[3] == '-' &&
              packetBuffer[4] == 'N' &&
              packetBuffer[5] == 'e' &&
              packetBuffer[6] == 't'){
                SPI.transfer(0);//Header bits
                SPI.transfer(0);
                SPI.transfer(0);
                SPI.transfer(0);
                for(int index = 0;index<512;index+=CHANNEL_PER_LED) { // 512 hardcoded max in Artemis Bridge Tools
                    if (CHANNEL_PER_LED == 4){
                      SPI.transfer((packetBuffer[18+index+3]/4)|0xE0); //Art-Net provides 0-127 for intensity, but led only takes 0-31, plus upper bits must be 1
                    }
                    else {
                      SPI.transfer(0xFF);
                    }
                    SPI.transfer(packetBuffer[18+index+2]); //Art-Net provides RGB order, but APA102 LEDs want BGR
                    SPI.transfer(packetBuffer[18+index+1]);
                    SPI.transfer(packetBuffer[18+index+0]);
                    //Serial.print(".");
                }
                SPI.transfer(0);//Footer bits  Should be 1/2 bit per LED  FIXME
                SPI.transfer(0);
                SPI.transfer(0);
                SPI.transfer(0);
  
                //Serial.println("!");
              }
    
    Udp.flush();
  } // end if


}
  

   
  void connectToWiFi(const char * ssid, const char * pwd)
  {
   
    Serial.println();
    Serial.println("------------------------------");
    Serial.println("Connecting to WiFi network: " + String(ssid));
   
   
   
    WiFi.begin(ssid, pwd);
    WiFi.config(ip,gate,subnet); //Comment this out to disable static IP
   
    while (WiFi.status() != WL_CONNECTED)
    {
      // Blink LED while we're connecting:
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(500);
      Serial.print(".");
    }
   
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  

