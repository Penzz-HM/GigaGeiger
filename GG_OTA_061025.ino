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
int    frametime = 1; 
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
const byte nvalues = 5;                                                                // rolling average window size
static byte current = 0;                                                                // index for current value
static byte cvalues = 0;                                                                // count of values read (<= nvalues)
static float sum = 0;                                                                   // rolling sum
static float values[nvalues];
float averagedVoltage = 235;                  
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
bool foxhunt = false;
int selectedAP = -1;
String targetSSID = "";
String targetBSSID = "";
bool trackByBSSID = false;
static unsigned long lastCurChannelChange = 0;
static unsigned long lastScan = 0;
const int scanInterval = 1000; // 1 second scan interval
const int lockTimeout = 5000;  //  seconds to lock
bool apLocked = false;
///////////////////////////////////////////
const char* host = "gigaGeiger";
const char* ssid = "GigaGeiger";
const char* password = "Defcon33";
WebServer server(80);
////////////////////////////////
bool game = false;
static int ballX = center_x, ballY = center_y;
static int ballSpeedX = 2, ballSpeedY = 2;
static int paddleWidth = 50;
static int paddleHeight = 10;
static unsigned long lastUpdate = 0;
const int frameDelay = 30;
const int wallMargin = 10;  // inset for walls
const int paddleYOffset = 15; // vertical spacing from top/bottom
static int playerX = center_x - paddleWidth / 2;
int enemyX = center_x - paddleWidth / 2;


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
  Serial.println("init");
  randomSeed (analogRead(0)); 
  tft.setSPISpeed(80000000);
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
  if(sensorValue < 280){ //enable flashmode
      tft.setTextColor (WHITE);    
      tft.setTextSize (2);
      tft.setCursor (center_x-70, center_y);
      tft.print ("FlashMode");            
      delay(750);
      WiFi.begin(ssid, password);
      // Wait for connection
      int ct = 0;
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        ct++;
        if(ct > 10){
          tft.setCursor (center_x-50, center_y + 30);
          tft.print ("WifiNotFound"); 
        }
      }
      tft.setCursor (center_x-50, center_y + 30);
      tft.print (WiFi.localIP());            
      /*use mdns for host name resolution*/
      if (!MDNS.begin(host)) { //http://esp32.local
        while (1) {
          delay(1000);
        }
      }
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
      tft.setRotation (0);  
      //tft.fillScreen (BLACK);
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
  else if( sensorValue > 280 && sensorValue < 570){//enable foxhunt mode
    foxhunt = true;
    //tft.fillScreen(BLACK);
  //display logo text
   tft.setTextColor (WHITE);    
   tft.setTextSize (3);
   tft.setCursor (center_x-70, center_y);
   tft.print ("FOXHUNT");        
   delay(2000);
   tft.fillScreen(BLACK);
  }
  else if( sensorValue > 570 && sensorValue < 1255){//enable foxhunt mode
    game = true;
    //tft.fillScreen(BLACK);
  //display logo text
   tft.setTextColor (WHITE);    
   tft.setTextSize (3);
   tft.setCursor (center_x-70, center_y);
   tft.print ("PONG");        
   delay(2000);
   tft.fillScreen(BLACK);
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
  sensorValue = analogRead(potPin);
  if(sensorValue < 1){
    curChannel = 12;
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
if (game) {


  if (millis() - lastUpdate > frameDelay) {
    lastUpdate = millis();

    // Clear previous positions
    tft.fillRect(ballX, ballY, 5, 5, BLACK);
    tft.fillRect(playerX, tft.height() - paddleYOffset - paddleHeight, paddleWidth, paddleHeight, BLACK);
    tft.fillRect(enemyX, paddleYOffset, paddleWidth, paddleHeight, BLACK);

    // Update ball position
    ballX += ballSpeedX;
    ballY += ballSpeedY;

    // === Map selectedAP (1–12) to 8 zones within walls ===
    const int zones = 8;
    int playfieldWidth = tft.width() - 2 * wallMargin - paddleWidth;
    int zoneWidth = playfieldWidth / (zones - 1);

    int zone = constrain(map(curChannel, 1, 12, 0, zones - 1), 0, zones - 1);
    enemyX = wallMargin + zone * zoneWidth;

    // Player paddle tracks ball (for now – can replace with user control later)
    playerX = constrain(ballX - paddleWidth / 2, wallMargin, tft.width() - wallMargin - paddleWidth);



    // Bounce off left/right walls
    if (ballX <= wallMargin || ballX >= tft.width() - wallMargin - 5)
      ballSpeedX = -ballSpeedX;

    // Bounce off top paddle
    if (ballY <= paddleYOffset + paddleHeight && ballX + 5 > enemyX && ballX < enemyX + paddleWidth) {
      ballSpeedY = -ballSpeedY;
      ballY = paddleYOffset + paddleHeight;
    }

    // Bounce off bottom paddle
    if (ballY + 5 >= tft.height() - paddleYOffset - paddleHeight &&
        ballX + 5 > playerX && ballX < playerX + paddleWidth) {
      ballSpeedY = -ballSpeedY;
      ballY = tft.height() - paddleYOffset - paddleHeight - 5;
    }

    // Reset ball if missed
    if (ballY < 0 || ballY > tft.height()) {
      ballX = center_x;
      ballY = center_y;
      ballSpeedX = 2 * (random(2) * 2 - 1);
      ballSpeedY = 2 * (random(2) * 2 - 1);
    }



    // Draw updated positions
    tft.fillRect(ballX, ballY, 5, 5, WHITE);
    tft.fillRect(playerX, tft.height() - paddleYOffset - paddleHeight, paddleWidth, paddleHeight, WHITE);
    tft.fillRect(enemyX, paddleYOffset, paddleWidth, paddleHeight, WHITE);
  }

  return;
}
  if (foxhunt) {
    static bool initialScanDone = false;
    static int availableAPs = 0;
    if (!apLocked) {
      if (!initialScanDone) {
        tft.setTextColor(WHITE, BLACK);
        tft.setTextSize(2);
        tft.setCursor(center_x - 80, center_y);
        tft.print("Scanning...");
        WiFi.scanNetworks(true);
        int n;
        do {
          delay(50);
          n = WiFi.scanComplete();
        } while (n == WIFI_SCAN_RUNNING);
        availableAPs = n;
        initialScanDone = true;
      }
      // Selecting mode
      if (selectedAP != curChannel) {
        selectedAP = curChannel;
        lastCurChannelChange = millis();
        // Display the current selected AP immediately
        if (availableAPs > selectedAP) {
          String previewBSSID = WiFi.BSSIDstr(selectedAP);
          tft.fillRect(center_x - 110, center_y, 230, 20, BLACK);
          tft.setTextColor(WHITE, BLACK);
          tft.setTextSize(2);
          tft.setCursor(center_x - 100, center_y - 40);
          tft.print("Choose MAC:");
          tft.setTextSize(2);
          tft.setCursor(center_x - 110, center_y);
          tft.print(previewBSSID);
          tft.setTextSize(1);
          tft.setCursor(center_x - 90, center_y + 40);
          tft.print("Use dial choose MAC");
          tft.setCursor(center_x - 90, center_y + 50);
          tft.print("Wait 5 sec to lock");
          targetSSID = WiFi.SSID(selectedAP);
          targetBSSID = WiFi.BSSIDstr(selectedAP);
        }
        else{
          String previewBSSID = WiFi.BSSIDstr(availableAPs-1);
          tft.fillRect(center_x - 110, center_y, 230, 20, BLACK);
          tft.setTextColor(WHITE, BLACK);
          tft.setTextSize(2);
          tft.setCursor(center_x - 100, center_y - 40);
          tft.print("Choose MAC:");
          tft.setTextSize(2);
          tft.setCursor(center_x - 110, center_y);
          tft.print(previewBSSID);
          tft.setTextSize(1);
          tft.setCursor(center_x - 90, center_y + 40);
          tft.print("Use dial choose MAC");
          tft.setCursor(center_x - 90, center_y + 50);
          tft.print("Wait 5 sec to lock");
          targetSSID = WiFi.SSID(availableAPs-1);
          targetBSSID = WiFi.BSSIDstr(availableAPs-1);
        
        }
      }
      if (millis() - lastCurChannelChange > lockTimeout) {
        // Lock after no change
        apLocked = true;
        tft.fillScreen(BLACK); // Clear screen once
        tft.setTextColor(WHITE, BLACK);
        tft.setTextSize(2);
        tft.setCursor(center_x - 40, center_y);
        tft.print("Locked: ");
        tft.setCursor(center_x - 110, center_y + 30);
        tft.print(targetBSSID);
        delay(500); // Let the user see the lock
        tft.fillScreen(BLACK); // Clean again before RSSI hunt
        tft.setTextColor(WHITE, BLACK);
        tft.setTextSize(2);
        tft.setCursor(center_x - 80, center_y);
        tft.print("Scanning...");
      }
    } else {
      // Locked mode - track RSSI
      if (millis() - lastScan > scanInterval) {
        WiFi.scanNetworks(true);
        lastScan = millis();
      }
      int n = WiFi.scanComplete();
      if (n != WIFI_SCAN_RUNNING) {
        int bestMatch = -1;
        for (int i = 0; i < n; ++i) {
          if (WiFi.BSSIDstr(i) == targetBSSID || WiFi.SSID(i) == targetSSID) {
            bestMatch = i;
            break;
          }
        }
        if (bestMatch != -1) {
          int rssi = WiFi.RSSI(bestMatch);
          // Display locked AP RSSI
          tft.fillRect(center_x - 60, center_y + 30, 60, 100, BLACK);
          tft.setTextColor(WHITE, BLACK);
          tft.setTextSize(2);
          tft.setCursor(center_x - 110, center_y);
          tft.print(targetBSSID);
          tft.setTextSize(2);
          tft.setCursor(center_x - 50, center_y + 30);
          tft.print(rssi);
        }
      }
    }
  }
  else{
    esp_wifi_set_promiscuous_rx_cb(&sniffer);  // Set callback for captured packets
    esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);  // Set the channel
    EEPROM.write(2, curChannel);
    EEPROM.commit();
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
      double pkt = pkts;
      double pktCnt =  ((pkt/200)*40);
      pkts=0;
      averagedVoltage = (230 + pktCnt); //230 is '0' 270 is '100'
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
   tft.print(pkt); //pkt
}