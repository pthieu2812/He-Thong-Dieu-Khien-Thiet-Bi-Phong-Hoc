#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include "Nextion.h"
#include "RTClib.h"
#include "string.h"
#include "SPI.h"
#include "MFRC522.h"
#include "Wire.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

#define RST_PIN         15
#define SS_PIN          5   

#define mqtt_server "m16.cloudmqtt.com"
#define mqtt_user "bcpibocf"
#define mqtt_pwd "f3oGkdUCyYuI"
const uint16_t mqtt_port = 16664;
#define mqtt_topic_pub "KhuA/A103"
#define mqtt_topic_sub "KhuA/A103"

const int acPin = 13;
const int fanPin = 2;
const int ledPin = 4;

const long utcOffsetInSeconds = 25200;

String a = "";
int endHour;
int endMinute;
int tempSec;
long accessSec;
long accessRfidSec;
bool rfidFlag = false;
bool endFlag = false;
bool connectstatusFlag = false;
bool acFlag = false;
bool lampFlag = false;
bool fanFlag = false;
bool acTempFlag = false;
bool lampTempFlag = false;
bool fanTempFlag = false;
bool timeSetFlag = false;
bool accessFlag = false;
bool writeFlag = false;
bool addRfidFlag = false;
FirebaseData firebaseData;
WiFiClient espClient;
PubSubClient client(espClient);
RTC_DS3231 rtc;
MFRC522 mfrc522(SS_PIN, RST_PIN);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 7*60*60);

void callback(char* topic, byte* payload, unsigned int length);

//Nextion tool init
//Page
NexPage entry = NexPage(0, 0, "entry"); //Entry page
NexPage MainPage = NexPage(1, 0, "MainPage"); //Main Page
NexPage DevicePage = NexPage(2, 0, "DevicePage"); //Device select Page
NexPage SetTimePage = NexPage(3, 0, "SetTimePage"); //Set end Time Page
NexPage SuccessPage = NexPage(4, 0, "SuccessPage"); // SuccessPage
NexPage FailPage = NexPage(5, 0, "FailPage"); //Fail Page
NexPage AddRfidPage = NexPage(7, 0, "AddRfidPage"); // Add new Rfid Page
NexPage TurnOffAllPage = NexPage(9, 0, "OffAllPage"); // Add new Rfid Page
//TextBox
NexText connectTextBox = NexText(0, 1, "t0"); //Connecting TextBox
NexText endTime = NexText(3, 14, "t4");
NexText acStatusText = NexText(1, 7, "acStatus");
NexText lampStatusText = NexText(1, 8, "lampStatus");
NexText fanStatusText = NexText(1, 11, "fanStatus");
NexText endTimeStatus = NexText(1, 9, "endTime");
NexText addNewRfidStatus = NexText(7, 1, "newrfid");
//Button
NexButton supportBtn = NexButton(0, 2, "b0"); //Button Setup wifi
NexButton acOnBtn = NexButton(2, 4, "b1"); 
NexButton acOffBtn = NexButton(2, 5, "b2");
NexButton lampOnBtn = NexButton(2, 6, "b3");
NexButton lampOffBtn = NexButton(2, 7, "b4");
NexButton fanOnBtn = NexButton(2, 10, "b6");
NexButton fanOffBtn = NexButton(2, 11, "b7");
NexButton nextBtn = NexButton(2, 8, "b5");
NexButton t2EndBtn = NexButton(3, 2, "b0");
NexButton t3EndBtn = NexButton(3, 3, "b1");
NexButton t4EndBtn = NexButton(3, 4, "b2");
NexButton t5EndBtn = NexButton(3, 5, "b3");
NexButton t8EndBtn = NexButton(3, 6, "b4");
NexButton t9EndBtn = NexButton(3, 7, "b5");
NexButton t10EndBtn = NexButton(3, 8, "b6");
NexButton t11EndBtn = NexButton(3, 9, "b7");
NexButton doneBtn = NexButton(3, 12, "b8");
NexButton rfidBackBtn = NexButton(7, 2, "rfidBack");

void tftUpdate();

//listen list
NexTouch *nex_listen_list[] =
{
  &supportBtn,
  &acOnBtn,
  &acOffBtn,
  &lampOnBtn,
  &lampOffBtn,
  &fanOnBtn,
  &fanOffBtn,
  &nextBtn,
  &t2EndBtn,
  &t3EndBtn,
  &t4EndBtn,
  &t5EndBtn,
  &t8EndBtn,
  &t9EndBtn,
  &t10EndBtn,
  &t11EndBtn,
  &doneBtn,
  &rfidBackBtn,
  NULL
};



void rfidBackBtn_release(void *ptr) {
  addNewRfidStatus.setText("Vui long nhap the RFID");
  writeFlag = false;
  addRfidFlag = false;
  entry.show();
}

void acOnBtn_release(void *ptr){
  acTempFlag = true;
  acOnBtn.Set_background_color_bco(63488);
  acOffBtn.Set_background_color_bco(48631);
  accessSec = millis();
}

void acOffBtn_release(void *ptr){
  acTempFlag = false;
  acOnBtn.Set_background_color_bco(48631);
  acOffBtn.Set_background_color_bco(63488);
  accessSec = millis();
}

void lampOnBtn_release(void *ptr){
  lampTempFlag = true;
  lampOnBtn.Set_background_color_bco(63488);
  lampOffBtn.Set_background_color_bco(48631);
  accessSec = millis();
}

void lampOffBtn_release(void *ptr){
  lampTempFlag = false;
  lampOnBtn.Set_background_color_bco(48631);
  lampOffBtn.Set_background_color_bco(63488);
  accessSec = millis();
}

void fanOnBtn_release(void *ptr){
  fanTempFlag = true;
  fanOnBtn.Set_background_color_bco(63488);
  fanOffBtn.Set_background_color_bco(48631);
  accessSec = millis();
}

void fanOffBtn_release(void *ptr){
  fanTempFlag = false;
  fanOnBtn.Set_background_color_bco(48631);
  fanOffBtn.Set_background_color_bco(63488);
  accessSec = millis();
}


void t2EndBtn_release(void *ptr) {
  endHour = 8;
  endMinute = 40;
  endTime.setText("8:40:00");
  timeSetFlag = true;
  accessSec = millis();
}

void t3EndBtn_release(void *ptr) {
  endHour = 9;
  endMinute = 40;
  endTime.setText("9:40:00");
  timeSetFlag = true;
  accessSec = millis();
}

void t4EndBtn_release(void *ptr) {
  endHour = 10;
  endMinute = 40;
  endTime.setText("10:40:00");
  timeSetFlag = true;
  accessSec = millis();
}

void t5EndBtn_release(void *ptr) {
  endHour = 11;
  endMinute = 30;
  endTime.setText("11:30:00");
  timeSetFlag = true;
  accessSec = millis();
}

void t8EndBtn_release(void *ptr) {
  endHour = 14;
  endMinute = 10;
  endTime.setText("14:10:00");
  timeSetFlag = true;
  accessSec = millis();
}

void t9EndBtn_release(void *ptr) {
  endHour = 15;
  endMinute = 10;
  endTime.setText("15:10:00");
  timeSetFlag = true;
}

void t10EndBtn_release(void *ptr) {
  endHour = 16;
  endMinute = 10;
  endTime.setText("16:10:00");
  timeSetFlag = true;
  accessSec = millis();
}

void t11EndBtn_release(void *ptr) {
  endHour = 17;
  endMinute = 00;
  endTime.setText("17:00:00");
  timeSetFlag = true;
}

void setTimeFailed() {
  FailPage.show();
  delay(2000);
  SetTimePage.show();
  accessSec = millis();
}


void setStatusDeviceText() {
  if(acFlag) {
    acStatusText.setText(":ON");
  }
  else {
    acStatusText.setText(":OFF");
  }

  if(lampFlag) {
    lampStatusText.setText(":ON");
  }
  else {
    lampStatusText.setText(":OFF");
  }

  if(fanFlag) {
    fanStatusText.setText(":ON");
  }
  else {
    fanStatusText.setText(":OFF");
  }
}

void setDevice(){
  if(acTempFlag) {
    acFlag = true;
    digitalWrite(acPin, HIGH);
  }
  else {
    acFlag = false;
    digitalWrite(acPin, LOW);
  }

  if(lampTempFlag) {
    lampFlag = true;
    digitalWrite(ledPin, HIGH);
  }
  else {
    lampFlag = false;
    digitalWrite(ledPin, LOW);
  }

  if(fanTempFlag) {
    fanFlag = true;
    digitalWrite(fanPin, HIGH);
  }
  else {
    fanFlag = false;
    digitalWrite(fanPin, LOW);
  }
  setStatusDeviceText();
}

void setDataFireBase() {
  if(acFlag) {
    Firebase.setString(firebaseData, "/rooms/A103/acStatus", "ON");
  }
  else {
    Firebase.setString(firebaseData, "/rooms/A103/acStatus", "OFF");
  }

  if(lampFlag) {
    Firebase.setString(firebaseData, "/rooms/A103/lampStatus", "ON");
  }
  else {
    Firebase.setString(firebaseData, "/rooms/A103/lampStatus", "OFF");
  }

  if(fanFlag) {
    Firebase.setString(firebaseData, "/rooms/A103/fanStatus", "ON");
  }
  else {
    Firebase.setString(firebaseData, "/rooms/A103/fanStatus", "OFF");
  }

  if(endFlag) {
    String end = String(endHour) + ":" + String(endMinute);
    Firebase.setString(firebaseData, "/rooms/A103/endTime", end);
  }
  else {
    Firebase.setString(firebaseData, "/rooms/A103/endTime", "OFF");
  }
}

void onSuccess() {
  timeSetFlag = false;
  acTempFlag = false;
  lampTempFlag = false;
  fanTempFlag = false;
  SuccessPage.show();
  setDataFireBase();
  delay(5000);
  MainPage.show();
  setStatusDeviceText();
}

void doneBtn_release(void *ptr) {
  if(!timeSetFlag) {
    setTimeFailed();
  }
  else {
    DateTime now = rtc.now();
    if(now.hour() > endHour) {
      setTimeFailed();
    }
    else if (now.hour() == endHour) {
      if(now.minute() >= endMinute) {
        setTimeFailed();
      }
      else {
        setDevice();
        endFlag = true; 
        onSuccess();
      }
    }
    else {
      endFlag = true;
      setDevice();
      onSuccess();
      Serial2.print("t6.font=1");
      tftUpdate();
      Serial2.print("t6.txt=");
      Serial2.print("\"");
      Serial2.print(endHour);
      Serial2.print(":");
      Serial2.print(endMinute);
      Serial2.print(":00");
      Serial2.print("\"");
      tftUpdate();
    }
  }
}

void nextBtn_release(void *ptr) {
  if(!acTempFlag && !fanTempFlag && !lampTempFlag) {
    setDevice();
    TurnOffAllPage.show();
    endFlag = false;
    timeSetFlag = false;
    setDataFireBase();
    delay(5000);
    MainPage.show();
    setStatusDeviceText();
  }
  else {
    SetTimePage.show();
  }
}
//Function to send update data to HMI Nextion
void tftUpdate() {
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
}

//Callback function
void configModeCallback (WiFiManager *myWiFiManager) {  
  connectTextBox.setText("Entered config mode");
}

void saveConfigCallback () {
connectTextBox.setText("Should save config");
}

void setup() {
  //Init HMI nextion
  nexInit();
  //Init wifi
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.autoConnect("ESP32_DATN", "12345678");
  //Init MQTT client
  client.setServer(mqtt_server, mqtt_port); 
  client.setCallback(callback);
  // Init SPI bus
  SPI.begin();
  // Init MFRC522
	mfrc522.PCD_Init();
	delay(4);				// Optional delay.
  while(WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  //Init timeClient
  timeClient.begin();
  timeClient.update();
  //Init RTC
  rtc.begin();
  rtc.adjust(DateTime(2014, 1, 21, timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds()));
  DateTime now = rtc.now();
  tempSec = now.second();
  //Init pinMode
  pinMode(acPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  //Set button callback function
  nextBtn.attachPop(nextBtn_release, &nextBtn);
  acOnBtn.attachPop(acOnBtn_release, &acOnBtn);
  acOffBtn.attachPop(acOffBtn_release, &acOffBtn);
  lampOnBtn.attachPop(lampOnBtn_release, &lampOnBtn);
  lampOffBtn.attachPop(lampOffBtn_release, &lampOffBtn);
  fanOnBtn.attachPop(fanOnBtn_release, &fanOnBtn);
  fanOffBtn.attachPop(fanOffBtn_release, &fanOffBtn);
  t2EndBtn.attachPop(t2EndBtn_release, &t2EndBtn);
  t3EndBtn.attachPop(t3EndBtn_release, &t3EndBtn);
  t4EndBtn.attachPop(t4EndBtn_release, &t4EndBtn);
  t5EndBtn.attachPop(t5EndBtn_release, &t5EndBtn);
  t8EndBtn.attachPop(t8EndBtn_release, &t8EndBtn);
  t9EndBtn.attachPop(t9EndBtn_release, &t9EndBtn);
  t10EndBtn.attachPop(t10EndBtn_release, &t10EndBtn);
  t11EndBtn.attachPop(t11EndBtn_release, &t11EndBtn);
  doneBtn.attachPop(doneBtn_release, &doneBtn);
  rfidBackBtn.attachPop(rfidBackBtn_release, &rfidBackBtn);

  Firebase.begin("my-test-login-app-a9e3e.firebaseio.com", "nr6Gr43EPrrA0tCQu2FV5nsEZlrESVxXeHbEtGlh");
}

// Hàm call back để nhận dữ liệu
void callback(char* topic, byte* payload, unsigned int length) {
  for (int i = 0; i < length; i++) {
    a = a + (char)payload[i];
  }
  
  String addRfid = "ADDRFID";
  String removeRfid = "REMOVERFID";
  if(!a.compareTo(addRfid.c_str())) {
      AddRfidPage.show();
      accessRfidSec = millis();
      writeFlag = true;
      addRfidFlag = true;
      rfidFlag = true;
  }
  else if(!a.compareTo(removeRfid.c_str())) {
      AddRfidPage.show();
      accessRfidSec = millis();
      writeFlag = true;
      addRfidFlag = false;
      rfidFlag = true;
  }
  else {
      String ledStatus = a.substring(0,5);
      String acStatus = a.substring(6,10);
      String fanStatus = a.substring(11,16);
      String hourEnd = a.substring(25,27);
      String minuteEnd = a.substring(28);

      endHour = atoi(hourEnd.c_str());
      endMinute = atoi(minuteEnd.c_str());

      if(!strcmp(ledStatus.c_str(),"LED:1")) {
        digitalWrite(ledPin, HIGH);
        lampFlag = true;
        endFlag = true;
      }
      else if (!strcmp(ledStatus.c_str(), "LED:0")) {
        digitalWrite(ledPin, LOW);
        lampFlag = false;
      }

      if(!strcmp(acStatus.c_str(),"AC:1")) {
        acFlag = true;
        endFlag = true;
        digitalWrite(acPin, HIGH);
      }
      else if (!strcmp(acStatus.c_str(),"AC:0")) {
        acFlag = false;
        digitalWrite(acPin, LOW);
      }

      if(!strcmp(fanStatus.c_str(),"FAN:1")) {
        digitalWrite(fanPin, HIGH);
        fanFlag = true;
        endFlag = true;
      }
      else if (!strcmp(fanStatus.c_str(),"FAN:0")) {
        fanFlag = false;
        digitalWrite(fanPin, LOW);
      }
      setStatusDeviceText();

      if(acFlag == false && lampFlag == false && fanFlag == false) {
        endFlag = false;
        endTimeStatus.setText("Not Set Yet");
      }

      if(endFlag) {
          String tempEnd = hourEnd + ":" + minuteEnd + ":00";
          Serial2.print("t6.font=1");
          tftUpdate();
          endTimeStatus.setText(tempEnd.c_str());
      }
      setDataFireBase();
  }
      a = "";
}
// Hàm reconnect thực hiện kết nối lại khi mất kết nối với MQTT Broker
void reconnect() {
  // Chờ tới khi kết nối
  while (!client.connected()) {
    // Thực hiện kết nối với topic, mqtt user và pass
    if (client.connect("A103",mqtt_user, mqtt_pwd)) {
      connectTextBox.setText("Connnect Successful");
      connectstatusFlag = true;
      // Khi kết nối sẽ publish thông báo
      client.publish(mqtt_topic_pub, "ESP_reconnected");
      // subscribe Topic
      client.subscribe(mqtt_topic_sub);    Serial.println("Attempting MQTT connection...");
      delay(2000);
      connectTextBox.setText("Vui long nhap the RFID.");
    } 
    else {
      connectstatusFlag = false;
      connectTextBox.setText(" failed, try again in 5 seconds");
      // Đợi 5s
      delay(5000);
    }
  }
}

void updateConnectingTxt() {
  connectTextBox.setText("Connecting.");
  delay(1000);
  connectTextBox.setText("Connecting..");
  delay(1000);
  connectTextBox.setText("Connecting...");
  delay(1000);
}

void reachEndTime() {
  digitalWrite(acPin, LOW);
  digitalWrite(ledPin, LOW);
  digitalWrite(fanPin, LOW);
  acFlag = false;
  lampFlag = false;
  fanFlag = false;
  setStatusDeviceText();
  Serial2.print("t6.font=0");
  tftUpdate();
  endTimeStatus.setText("Not Set Yet!!");
  endFlag = false;
  setDataFireBase();
}
void loop() {
  // Kiểm tra kết nối
  nexLoop (nex_listen_list);
  if(WiFi.status() == WL_CONNECTED) {
      if (!client.connected()) {
        updateConnectingTxt();
        reconnect();
      }
      client.loop();

      //kiem tra the
      if(accessFlag) {
        if(millis() - accessSec >= 10000) {
          entry.show();
          connectTextBox.setText("Vui long nhap the RFID");
          acTempFlag = false;
          lampTempFlag = false;
          accessFlag = false;
          timeSetFlag = false;
        }
      }
      else {
        connectTextBox.setText("Vui long nhap the RFID");
      }

      if(rfidFlag) {
        if(millis() - accessRfidSec >= 5000) {
          entry.show();
          rfidFlag = false;
          writeFlag = false;
          addRfidFlag = false; 
        }
      }
      //Time
      DateTime now = rtc.now();
      if(now.minute() % 10 == 0) {
      //update Time
        timeClient.update();
        rtc.adjust(DateTime(2014, 1, 21, timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds()));
      }
      if(tempSec != now.second()) {
        tempSec = now.second(); 
        Serial2.print("t2.txt=");
        Serial2.print("\"");
        Serial2.print(now.hour());
        Serial2.print(":");
        Serial2.print(now.minute());
        Serial2.print(":");
        Serial2.print(now.second());
        Serial2.print("\"");
        tftUpdate();
        if(endFlag == true) {
          Serial2.print("endTime.font=1");
          tftUpdate();
          Serial2.print("endTime.txt=");
          Serial2.print("\"");
          Serial2.print(endHour);
          Serial2.print(":");
          Serial2.print(endMinute);
          Serial2.print(":00");
          Serial2.print("\"");
          tftUpdate();
          if(endHour < now.hour()) {
            reachEndTime();
          }
          else if(endHour == now.hour()) {
            if(endMinute <= now.minute()) {
              reachEndTime();
            }
          }
        }
        else {
            endTimeStatus.setText("Not Set Yet!!");
        }
      }

      // Xu ly rfid
      // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
      MFRC522::MIFARE_Key key;
      for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

      //some variables we need
      byte block;
      byte len;
      MFRC522::StatusCode status;

      // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
      if ( !mfrc522.PICC_IsNewCardPresent()) {
        return;
      }

      // Select one of the cards
      if ( !mfrc522.PICC_ReadCardSerial()) {
        return;
      }

      accessRfidSec = millis();

      if(writeFlag == false) {
        byte buffer1[18];
        block = 4;
        len = 18;

        status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
        if (status != MFRC522::STATUS_OK) {
        }
        status = mfrc522.MIFARE_Read(block, buffer1, &len);
        if (status != MFRC522::STATUS_OK) {
        }
        else {
          if(buffer1[0] == 1 && 
            buffer1[1] == 2 && 
            buffer1[2] == 3 ) {
            connectTextBox.setText("Thanh cong!");
            delay(2000);
            MainPage.show();
            setStatusDeviceText();
            accessSec = millis();
            accessFlag = true;
            Serial2.print("endTime.font=1");
            tftUpdate();
            Serial2.print("endTime.txt=");
            Serial2.print("\"");
            Serial2.print(endHour);
            Serial2.print(":");
            Serial2.print(endMinute);
            Serial2.print(":00");
            Serial2.print("\"");
            tftUpdate();
          }
          else {
                connectTextBox.setText("Kg phu hop!. Thu lai trong 5s");
                accessFlag = false;
                delay(5000);
                connectTextBox.setText("Nhap the RFID!");
          }
        }
      }
      else {
        byte block = 4;
        status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
        if (status != MFRC522::STATUS_OK) {
          return;
        }
      // Write block
        byte valueBlockA = 4;
        byte addBlock[] = {1, 2, 3, 0,   0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  valueBlockA, ~valueBlockA, valueBlockA, ~valueBlockA};
        byte removeBlock[] ={0, 0, 0, 0,   0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  valueBlockA, ~valueBlockA, valueBlockA, ~valueBlockA};
        char success[] =  "Them the moi thanh cong!";
        char failed[] = "Xoa the thanh cong!";
        char* result;
        if(addRfidFlag) {
          status = mfrc522.MIFARE_Write(block, addBlock, 16);
          result = success;
        }
        else {
          status = mfrc522.MIFARE_Write(block, removeBlock, 16);
          result = failed;
        }
        
        if (status != MFRC522::STATUS_OK) {
          addNewRfidStatus.setText("Ghi len the that bai!");
          return;
        }
        else {
          addNewRfidStatus.setText(result);
          delay(2000);
          addNewRfidStatus.setText("Vui long them the RFID!");
        }
      }

    }
    else {
      updateConnectingTxt();
    }
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
}