#include <SPI.h>
#include <WiFi.h>
#include <WiFiUDP.h>

//Did you pick Art-Net 3-Ch or 4-Ch in DMX tools
#define CHANNELS_PER_LED 3

#define NUM_UNIVERSES 2

// WiFi network name and password:
#define networkName  "Bill.Wi.The.Science.Fi"
#define networkPswd  "ScienceRules1"

#define LED_PIN 5
   
IPAddress ip(192, 168, 1, 222);  //Edit this to your own static IP, or comment out wifi.config below
IPAddress gate(192, 168, 1, 1);
IPAddress subnet(255, 255, 255,0);
   



#define TOTAL_LEDS 512/CHANNELS_PER_LED*NUM_UNIVERSES
//Full Data Length: 4 Header bytes+4 Bytes/LED real data + .5 bits per LED footer
#define SPI_DATA_LEN 4+TOTAL_LEDS*4+TOTAL_LEDS/16 +1

uint8_t spi_data[SPI_DATA_LEN] = {};    

   
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
    xTaskCreatePinnedToCore(loopTask, "loopTask", 4096, NULL, 1, NULL, 0);  //Override core to 0 until ESP32 fixes their stuff
}

void setup()
{
    // Initilize hardware:
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
    SPI.begin();
 
    connectToWiFi(networkName, networkPswd);
    Udp.begin(0x1936);

    //Init LEDs to all off
    for(int i = 4;i<TOTAL_LEDS*4;i+=4) {
        spi_data[i]=0xE0;
    }
    SPI.writeBytes(spi_data,SPI_DATA_LEN);
}


void loop()
{
    if (WiFi.status() != WL_CONNECTED) {  //Auto reconnect
        connectToWiFi(networkName, networkPswd);
    }
    char packetBuffer[2000];
    int numBytes = Udp.parsePacket();
    if ( numBytes ) {
        Udp.read(packetBuffer,numBytes); // read the packet into the buffer
        if (strcmp(packetBuffer,"Art-Net") ==0 ){
               
                
                //Calc starting position of data to update
                if (packetBuffer[14] > NUM_UNIVERSES) packetBuffer[14] = NUM_UNIVERSES;  //  max
                int out_index =4 + 512/CHANNELS_PER_LED*4*packetBuffer[14];//offset by 4 for header + Number of LEDs per universe*4(bytes/led)                  


                for(int in_index = 18;in_index<(512+18);in_index+=CHANNELS_PER_LED,out_index+=4) {
                    //Bridge Tools hardcoded RGBI, APA102 wants IBGR                    
                    //Art-Net provides 0-127 for intensity, but led only takes 0-31, plus upper bits must be 1
                    spi_data[out_index]   = (CHANNELS_PER_LED == 3)?0xFF:(packetBuffer[in_index+3]/4)|0xE0;  //Intensity
                    spi_data[out_index+1] = packetBuffer[in_index+2];  //Red
                    spi_data[out_index+2] = packetBuffer[in_index+1];  //Gree
                    spi_data[out_index+3] = packetBuffer[in_index+0];  //Blue 
                }
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            SPI.writeBytes(spi_data,SPI_DATA_LEN);
        }
    }
}
  
   
void connectToWiFi(const char * ssid, const char * pwd)
{  
    Serial.println();
    Serial.println("------------------------------");
    Serial.println("Connecting to WiFi network: " + String(ssid));
    Serial.println("------------------------------");
   
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pwd);
    WiFi.config(ip,gate,subnet); //Comment this out to disable static IP
   
    while (WiFi.status() != WL_CONNECTED)
    {
      // Blink LED while we're connecting:
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(500);
    }
   
    Serial.println();
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();
  }
  

