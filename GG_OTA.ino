/*
 * OTAWebUpdater.ino Example from ArduinoOTA Library
 * Rui Santos 
 * Complete Project Details https://randomnerdtutorials.com
 */
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"
#include <Wire.h>
#include <EEPROM.h>
#include "esp_wifi.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

////////////////////////////////////////////////////
//GG code
///////////////////////////////////////
#define TFT_DC  9
#define TFT_CS 10
#define TFT_MOSI  11  //sda
#define TFT_CLK 12  //scl
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK);
#define BLACK      0x0000                                                               // some extra colors
#define BLUE       0x001F
#define RED        0xF800
#define GREEN      0x07E0
#define CYAN       0x07FF
#define MAGENTA    0xF81F
#define YELLOW     0xFFE0
#define WHITE      0xFFFF
#define ORANGE     0xFBE0
#define GREY       0x84B5
#define BORDEAUX   0xA000
//#define AFRICA     0xAB21      
#define AFRICA     0x5d35
#define maxCh 14                                                           // current dial color
#define DEG2RAD 0.0174532925 
int multiplier;
int    frametime = 10; 
int    x_pos;
int    y_pos; 
int    center_x = 120;                                                                  // center x of dial on 240*240 TFT display
int    center_y = 120;                                                                  // center y of dial on 240*240 TFT display
float  pivot_x, pivot_y,pivot_x_old, pivot_y_old;
float  p1_x,p1_y,p2_x,p2_y,p3_x, p3_y, p4_x, p4_y, p5_x, p5_y; 
float  p1_x_old,p1_y_old, p2_x_old, p2_y_old, p3_x_old, p3_y_old;
float  p4_x_old, p4_y_old, p5_x_old, p5_y_old;
float  angleOffset = 3.14;
float  arc_x;
float  arc_y;
int    radius = 120;                                                                    // center y of circular scale                                                   
float  angle_circle = 0;
float  needleAngle = 0;
int    j;                                                            
float  volt = 220;
int    needle_multiplier = 1;
float  needle_setter;              
// voltage rolling averaging stuff 
const byte nvalues = 5;                                                                // rolling average window size
static byte current = 0;                                                                // index for current value
static byte cvalues = 0;                                                                // count of values read (<= nvalues)
static float sum = 0;                                                                   // rolling sum
static float values[nvalues];
float averagedVoltage = 235;                  
   //===== Run-Time variables =====//
uint32_t prevTime    = 0;
uint32_t curTime     = 0;
uint32_t pkts        = 0;
uint32_t no_deauths  = 0;
uint32_t deauths     = 0;
uint8_t curChannel   = 1;
uint32_t maxVal      = 0;
double multiplicator = 0.0;
const int analogInPin = A0;  // ESP8266 Analog Pin ADC0 = A0
bool deau = false;
int sensorValue = 0;  // value read from the pot
const int potPin = 15;
uint16_t val[128];
///////////////////////////////////////////


const char* host = "gigaGeiger";
const char* ssid = "GigaGeiger";
const char* password = "Defcon33";

WebServer server(80);

/*
 * Login page
 */
const char* loginIndex = 
 "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>ESP32 Login Page</b></font></center>"
                "<br>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<td>Username:</td>"
        "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
"</script>";
 
/*
 * Server Index Page
 */
 
const char* serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')" 
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";

void sniffer(void *buf, wifi_promiscuous_pkt_type_t type) {
  pkts++;  // Increment the packet count
  
}



/*
 * setup function
 */
void setup(void) {
  Serial.begin(115200);
  randomSeed (analogRead(0)); 
  tft.begin();    
  tft.fillScreen(BLACK);

  //display logo text
   tft.setTextColor (WHITE);    
   tft.setTextSize (3);
   tft.setCursor (center_x-70, center_y);
   tft.print ("GigaGeiger");      
    tft.setTextSize (2);
   tft.setCursor (center_x-50, center_y + 30);
   tft.print ("By: HM*");   
   delay(2000);
   tft.fillScreen(BLACK);

  //check if in flash mode or not
  sensorValue = analogRead(potPin);
  Serial.println(sensorValue);
  if(sensorValue < 10){
      tft.setTextColor (WHITE);    
      tft.setTextSize (2);
      tft.setCursor (center_x-70, center_y);
      tft.print ("FlashMode");            
      delay(750);
      //tft.fillScreen(BLACK);
      WiFi.begin(ssid, password);
      Serial.println("");

      // Wait for connection
      int ct = 0;
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        ct++;
        if(ct > 10){
          tft.setCursor (center_x-50, center_y + 30);
          tft.print ("WifiNotFound"); 
        }
        
      }
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      tft.setCursor (center_x-50, center_y + 30);
      tft.print (WiFi.localIP());            
      /*use mdns for host name resolution*/
      if (!MDNS.begin(host)) { //http://esp32.local
        Serial.println("Error setting up MDNS responder!");
        while (1) {
          delay(1000);
        }
      }
      Serial.println("mDNS responder started");
      /*return index page which is stored in serverIndex */
      server.on("/", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", loginIndex);
      });
      server.on("/serverIndex", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", serverIndex);
      });
      /*handling uploading firmware file */
      server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
      }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
          Serial.printf("Update: %s\n", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          /* flashing firmware to ESP*/
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          if (Update.end(true)) { //true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          } else {
            Update.printError(Serial);
          }
        }
      });
      server.begin();
        int count = 0;
        while (count <= 60){
            server.handleClient();
            delay(1000);
            count++;
            Serial.println("waiting for 1 min boot");
        }
        WiFi.mode(WIFI_STA);
      // Initialize the Wi-Fi with default configuration
      esp_wifi_set_mode(WIFI_MODE_STA);
      esp_wifi_set_promiscuous(true);  // Enable promiscuous mode
      esp_wifi_set_promiscuous_rx_cb(&sniffer);  // Set callback for captured packets
      esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);  // Set the channel
        EEPROM.begin(10);  // Initialize EEPROM
        curChannel = EEPROM.read(2);
        if (curChannel < 1 || curChannel > maxCh) {
            curChannel = 1;
            EEPROM.write(2, curChannel);
            EEPROM.commit();
        }
bam
      tft.setRotation (0);  
      tft.fillScreen (BLACK);
      tft.drawCircle (center_x, center_y,120, BLACK);             
      pivot_x = center_x;
      pivot_y = center_y+50;
      p1_x_old = center_x; p1_y_old = center_y+50;
      p2_x_old = center_x; p2_y_old = center_y+50;
      p3_x_old = center_x; p3_y_old = center_y+50;
      p4_x_old = center_x; p4_y_old = center_y+50;
      p5_x_old = center_x; p5_y_old = center_y+50;
      volt = 240;                                                                             // initial value setting the needle
      create_dial (curChannel);
      needle_setter = volt;
      needleAngle = (((needle_setter)*DEG2RAD*1.8)-3.14);
      needle();  
      draw_pivot ();
      


  }
  else{
    WiFi.mode(WIFI_STA);
  // Initialize the Wi-Fi with default configuration
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_promiscuous(true);  // Enable promiscuous mode
  esp_wifi_set_promiscuous_rx_cb(&sniffer);  // Set callback for captured packets
  esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);  // Set the channel
    EEPROM.begin(10);  // Initialize EEPROM
    curChannel = EEPROM.read(2);
    if (curChannel < 1 || curChannel > maxCh) {
        curChannel = 1;
        EEPROM.write(2, curChannel);
        EEPROM.commit();
    }
   tft.setRotation (0);  
   tft.fillScreen (BLACK);
   tft.drawCircle (center_x, center_y,120, BLACK);             
   pivot_x = center_x;
   pivot_y = center_y+50;
   p1_x_old = center_x; p1_y_old = center_y+50;
   p2_x_old = center_x; p2_y_old = center_y+50;
   p3_x_old = center_x; p3_y_old = center_y+50;
   p4_x_old = center_x; p4_y_old = center_y+50;
   p5_x_old = center_x; p5_y_old = center_y+50;
   volt = 240;                                                                             // initial value setting the needle
   create_dial (curChannel);
   needle_setter = volt;
   needleAngle = (((needle_setter)*DEG2RAD*1.8)-3.14);
   needle();  
   draw_pivot ();
  }


}




void loop(void) {
  //pkts = 0;
  //<10, 170, 280, 360, 455, 570, 740, 950, 1351, 2280, 4000
  
  sensorValue = analogRead(potPin);
  Serial.println(sensorValue);
  if(sensorValue < 1){
    curChannel = 11;
    deau = true;
  }
  else if(sensorValue < 170){
    curChannel = 11;
    deau = false;
  }
  else if(sensorValue < 280){
    curChannel = 10;
    deau = false;
  }
  else if(sensorValue < 360){
    curChannel = 9;
    deau = false;
  }
  else if(sensorValue <455){
    curChannel = 8;
    deau = false;
  }
  else if(sensorValue < 570){
    curChannel = 7;
    deau = false;
  }
  else if(sensorValue < 725){
    curChannel = 6;
    deau = false;
  }
  else if(sensorValue < 970){
    curChannel = 5;
    deau = false;
  }
  else if(sensorValue < 1255){
    curChannel = 4;
    deau = false;
  }
  else if(sensorValue < 2080){
    curChannel = 3;
    deau = false;
  }
  else if(sensorValue < 4000){
    curChannel = 2;
    deau = false;
  }
  else if(sensorValue < 4100){
    curChannel = 1;
    deau = false;
  }
  
  

  //esp_wifi_set_promiscuous_rx_cb(&sniffer);  // Set callback for captured packets
  esp_wifi_set_promiscuous_rx_cb(&sniffer);  // Set callback for captured packets
  esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);  // Set the channel
  EEPROM.write(2, curChannel);
  EEPROM.commit();
  //Serial.print("pkts: ");
  //Serial.println(pkts);
  //Serial.print("ch: ");
  //Serial.print(curChannel);
 // Serial.print(" ");

///updated the channel on the dial
if(!deau){
    tft.setTextColor (BLACK,AFRICA);    
   tft.setTextSize (1);
   tft.setCursor (center_x+15, center_y+40);
   tft.print ("Chnl:");            
   tft.print(curChannel); 
   tft.print(" "); 
  }
  else{
    tft.setTextColor (BLACK,AFRICA);    
   tft.setTextSize (1);
   tft.setCursor (center_x+15, center_y+40);
   tft.print ("Deauths");            

   tft.print(" "); 
  }
  

   
  //esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);  // Set the channel
  
  double pkt = pkts;
  //Serial.println(String(pkt));
  
  double pktCnt =  ((pkt/200)*40);
  //Serial.print("pkt offset: ");
  //Serial.println(pktCnt);
  pkts=0;

      
   //volt = random (230,250);                                                                // voltage simulator  
   //Serial.print ("volt out of smpt01B: ");
   //Serial.println (volt);  
   //averagedVoltage = movingAverage(volt);
   averagedVoltage = (230 + pktCnt); //230 is '0' 270 is '100'
   //averagedVoltage = 230;
 
  if(!deau){
    displayNumerical (pkt);
    needle_setter = averagedVoltage;     
    needle();
    draw_pivot (); 
  }
  else{
     displayNumerical (deauths);
    needle_setter = averagedVoltage;     
    needle();
    draw_pivot (); 
  }
  
   
  


   delay (frametime);
   
   
}




void needle (){                                                                            // dynamic needle management

   tft.drawLine (pivot_x, pivot_y, p1_x_old, p1_y_old, AFRICA);                            // remove old needle  
   tft.fillTriangle (p1_x_old, p1_y_old, p2_x_old, p2_y_old, p3_x_old, p3_y_old, AFRICA);  // remove old arrow head
   tft.fillTriangle (pivot_x, pivot_y, p4_x_old, p4_y_old, p5_x_old, p5_y_old, AFRICA);    // remove old arrow head
    
   needleAngle = (((needle_setter)*0.01745331*1.8)-3.14);
   p1_x = (pivot_x + ((radius)*cos(needleAngle)));                                         // needle tip
   p1_y = (pivot_y + ((radius)*sin(needleAngle))); 

   p2_x = (pivot_x + ((radius-15)*cos(needleAngle-0.05)));                                 // needle triange left
   p2_y = (pivot_y + ((radius-15)*sin(needleAngle-0.05))); 

   p3_x = (pivot_x + ((radius-15)*cos(needleAngle+0.05)));                                 // needle triange right
   p3_y = (pivot_y + ((radius-15)*sin(needleAngle+0.05))); 

   p4_x = (pivot_x + ((radius-90)*cos(angleOffset+(needleAngle-0.2))));                    // needle triange left
   p4_y = (pivot_y + ((radius-90)*sin(angleOffset+(needleAngle-0.2)))); 

   p5_x = (pivot_x + ((radius-90)*cos(angleOffset+(needleAngle+0.2))));                    // needle triange right
   p5_y = (pivot_y + ((radius-90)*sin(angleOffset+(needleAngle+0.2)))); 
  
   p1_x_old = p1_x; p1_y_old = p1_y;                                                       // remember previous needle position
   p2_x_old = p2_x; p2_y_old = p2_y;                                                                         
   p3_x_old = p3_x; p3_y_old = p3_y;                                                                      

   p4_x_old = p4_x; p4_y_old = p4_y;                                                       // remember previous needle counterweight position
   p5_x_old = p5_x; p5_y_old = p5_y;                                                                      

   tft.drawLine (pivot_x, pivot_y, p1_x, p1_y, BLACK);                                     // create needle 
   tft.fillTriangle (p1_x, p1_y, p2_x, p2_y, p3_x, p3_y, BLACK);                           // create needle tip pointer
   tft.drawLine (center_x-80, center_y+70, center_x+80,center_y+70, WHITE);                // repair floor 
   tft.fillTriangle (pivot_x, pivot_y, p4_x, p4_y, p5_x, p5_y, BLACK);                     // create needle counterweight
}


void create_dial (int ch){

   tft.fillCircle (center_x, center_y,120, AFRICA);                                        // general dial field
   tft.drawCircle (center_x, center_y,118,GREY);  
   tft.drawCircle (center_x, center_y,117,BLACK);
   tft.drawCircle (center_x, center_y,116,BLACK);  
   tft.drawCircle (center_x, center_y,115,GREY);

   for (j= 30; j<75    ; j+=5)
       {
        needleAngle = ((j*DEG2RAD*1.8)-3.14);
        arc_x = (pivot_x + ((radius+15)*cos(needleAngle)));                                // needle tip
        arc_y = (pivot_y + ((radius+15)*sin(needleAngle))); 
        tft.drawPixel  (arc_x,arc_y,BLACK);
        tft.fillCircle (arc_x,arc_y,2, BLACK);
        }

   tft.setTextColor (BLACK,AFRICA);    
   tft.setTextSize (1);
   tft.setCursor (center_x+15, center_y+40);
   tft.print ("Chnl:");            
   tft.print(ch);                                                                                                                                                           
   tft.drawLine (center_x-80, center_y+70, center_x+80,center_y+70, WHITE);                // create floor   
}


void draw_pivot (){
 
   tft.fillCircle (pivot_x, pivot_y,8,RED);               
   tft.drawCircle (pivot_x, pivot_y,8,BLACK);            
   tft.drawCircle (pivot_x, pivot_y,3,BLACK);      
}

 
void displayNumerical (int pkt){

   tft.fillRect (center_x-82, center_y+40, 62,16,AFRICA);
   tft.setTextColor (BLACK);    
   tft.setTextSize (1);
   tft.setCursor (center_x-80, center_y+40);
   tft.print ("Pkt:");   
   tft.print(sensorValue); //pkt
}


float movingAverage (float value) {

   sum += value;                    
   if (cvalues == nvalues)                                                                 // if the window is full, adjust the sum by deleting the oldest value
     sum -= values[current];

   values[current] = value;                                                                // replace the oldest with the latest

   if (++current >= nvalues)
     current = 0;

   if (cvalues < nvalues)
     cvalues += 1;

   return sum/cvalues;
}