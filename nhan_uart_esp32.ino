#include <HardwareSerial.h>
#include <stdlib.h>
#include <string.h>

#include <ArduinoJson.h>
#include<WiFi.h>
#include <WebServer.h> // Thêm thư viện WebServer
#include <Ticker.h>

#include<Firebase_ESP_Client.h>
#include"addons/TokenHelper.h"
#include"addons/RTDBHelper.h"
#include <EEPROM.h>
#include <time.h>

Ticker blinker;
// ket thu them
typedef struct {
  uint8_t srcid;
  float   temp;
  float   hum1;
  float   hum2;
} FrameDataF_t;

 FrameDataF_t f;
// code them tu 
#define ledPin 2
#define btnPin 0
#define PUSHTIME 5000

#define API_KEY "AIzaSyB_aDU1LR4WPYFR7DPktUHW3tQdmKxYkuM"// can thay
#define DATABASE_URL "https://doan22-e15f5-default-rtdb.asia-southeast1.firebasedatabase.app/"// can thay
FirebaseData fbdo, fbdo_s2, fbdo_s1,fbdo_s3;// can thay
FirebaseAuth auth;
FirebaseConfig config;
// bien luu thoi gian tuc
struct tm timeinfo;
//bien dem thoi gian de thuc hien
unsigned long deleteHisMillis = 0;
unsigned long deleteWarMillis = 0;
unsigned long sendDataPrevMillis = 0;
unsigned long prevMillisMain = 0;
unsigned long prevMillisHis = 0;
unsigned long prevMillisWar = 0;
unsigned long sendDataWarning=0;
unsigned long countHis = 0;
int checktime=0;
// 
unsigned long countWar=0;
bool signupOk = false;
float Temp=0.0;
float Hum1=0.0;
float Hum2=0.0;
int Node=0;
unsigned long lineSet= 0;
unsigned long timeSetHis=0;
int retry = 0;
float tempSet=0.0;
time_t now;
// Máy chủ NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600; // GMT+7 cho Việt Nam
const int   daylightOffset_sec = 0;

WebServer webServer(80); // Khởi tạo đối tượng WebServer cổng 80

int wifiMode; // 0: Chế độ cấu hình, 1: Chế độ kết nối, 2: Mất WiFi
unsigned long blinkTime = millis();
unsigned long lastTimePress = millis();
String ssid;
String password;
String uid;
String name;
void checkButton(); 

void blinkLed(uint32_t t){
  if(millis() - blinkTime > t){
    digitalWrite(ledPin, !digitalRead(ledPin));
    blinkTime = millis();
  }
}
void ledControl(){
  if(digitalRead(btnPin) == LOW){
    if(millis() - lastTimePress < PUSHTIME){
      blinkLed(1000);
    }else{
      blinkLed(50);
    }
  }else{
    if(wifiMode == 0){
      blinkLed(50);
    }else if(wifiMode == 1){
      blinkLed(3000);
    }else if(wifiMode == 2){
      blinkLed(300);
    }
  }
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
        // Sự kiện khi ESP32 đã kết nối với AP thành công (chưa lấy IP)
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("WiFi connected!");
             wifiMode = 1; 
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      
                Serial.println("WiFi lost connection.");
                wifiMode = 2; 

              
                delay(2000);
            break;
        default:
           
            break;
  }
}

void setupWifi(){
  WiFi.onEvent(WiFiEvent); // Đăng ký chương trình bắt sự kiện WiFi
  if(ssid != ""){
    Serial.println("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);  // lưu config vào flash
    Serial.print("Đang kết nối tới WiFi");
    // Vòng lặp chờ cho đến khi kết nối thành công
       // Vòng lặp chờ đến khi kết nối thành công
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
       wifiMode = 1;
       checkButton();
    }
    Serial.print("📡 Địa chỉ IP: ");
    Serial.println(WiFi.localIP());
    Serial.println("\n✅ Đã hoàn thành kết nối WiFi!");
  }else{
    Serial.println("ESP32 WiFi network created!");
    WiFi.mode(WIFI_AP);
    String ssid_ap = "ESP32" ;
    WiFi.softAP(ssid_ap.c_str());
    Serial.println("Access Point name: " + ssid_ap);
    Serial.println("Web server access address: " + WiFi.softAPIP().toString());
    wifiMode = 0;
  }
}
void updateTime() {
  time(&now);
  localtime_r(&now, &timeinfo);
}
void setupWebServer(){
     webServer.on("/scanWifi", []{
       Serial.println("Scanning WiFi network...");
       int wifi_nets = WiFi.scanNetworks(true, true);
       const unsigned long t = millis();
       while(wifi_nets < 0 && millis() - t < 10000){
         delay(20);
         wifi_nets = WiFi.scanComplete();
        }
       DynamicJsonDocument doc(200);
       for(int i = 0; i < wifi_nets; ++i){
          Serial.println(WiFi.SSID(i));
          doc.add(WiFi.SSID(i));
        }
       String wifiList = "";
       serializeJson(doc, wifiList);
       Serial.println("WiFi list: " + wifiList);
       webServer.send(200, "application/json", wifiList);
      });
    webServer.on("/saveWifi", []() {
      Serial.println(" da vao saveWifi");
      String ssid_temp = webServer.arg("ssid");
      String password_temp = webServer.arg("pass");
       String uid_temp = webServer.arg("uid");
        String name_temp = webServer.arg("name");
   
      EEPROM.writeString(0,ssid_temp);
      EEPROM.writeString(32,password_temp);
       EEPROM.writeString(100,uid_temp);
       EEPROM.writeString(200,name_temp);
      EEPROM.commit();
      webServer.send(200,"text/plain","Wifi has been saved!");
    });

   webServer.on("/reStart", []{
      
    webServer.send(200, "text/plain", "ESP32 is restarting!");
    delay(3000);
    ESP.restart();
   });
 
  webServer.begin(); 
}

void checkButton(){
  if(digitalRead(btnPin) == LOW){
    Serial.println("Nhấn và giữ 5 giây để reset về mặc định!");
    if(millis() - lastTimePress > PUSHTIME){
      for(int i = 0; i < 256; i++){
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      Serial.println("Đã xóa dữ liệu EEPROM!");
      delay(2000);
      ESP.restart();
    }
    delay(1000);
  }else{
    lastTimePress = millis();
  }
}

String chuyenDoi (int time){
  String timeNew;
  if (time<10){
    timeNew="0"+String(time);
  }
  else{
    timeNew=String(time);
  }
  return timeNew;
}

void setTimeHis(){// sua
 if (Firebase.ready()&& signupOk){
    if(!Firebase.RTDB.readStream(&fbdo_s2))
    Serial.printf("stream 2 (Nhiet do dat canh bao) begin error,%s\n\n", fbdo_s2.errorReason().c_str());
    if (fbdo_s2.streamAvailable()){
      if(fbdo_s2.dataType()=="int"){
        timeSetHis=fbdo_s2.intData()*1000;
        Serial.println("Successfull read from"+fbdo_s2.dataPath()+ timeSetHis+"("+fbdo_s2.dataType()+")");
     }
   }
  }
}

void getTime(){
 retry = 0;
  while (!getLocalTime(&timeinfo) && retry < 20) {
    Serial.println("⏳ Đang đồng bộ thời gian...");
    delay(300);
    retry++;
  }

  if (retry == 20) {
    Serial.println("❌ Không thể lấy thời gian!");
  } else {
    Serial.println("✅ Lấy thời gian thành công!");

    // Lấy luôn epoch time
    time(&now);
    if (now > 100000) {
      Serial.print("Epoch time: ");
      Serial.println(now);
    } else {
      Serial.println("⚠️ Epoch chưa hợp lệ!");
    }
  }
}
void checkLineHis() {
  if (Firebase.RTDB.getInt(&fbdo,"/"+uid+"/"+name+"/"+String(Node)+ "/Count/CountHis")) {
   countHis=fbdo.intData();
   if(countHis > 5){// sua 
    QueryFilter query;
    query.orderBy("$key"); 
    query.limitToFirst(1);  // chỉ lấy 1 bản ghi cũ nhất

     if (Firebase.RTDB.getJSON(&fbdo,"/"+uid+"/"+name+"/"+String(Node)+ "/History", &query)) {
       FirebaseJson json = fbdo.jsonObject();
       size_t count = json.iteratorBegin();   
       String key, value;
       int type;
       json.iteratorGet(0, type, key, value);
       Serial.printf("Key: %s, Value: %s\n", key.c_str(), value.c_str());
       Firebase.RTDB.deleteNode(&fbdo,"/"+uid+"/"+name+"/"+String(Node)+ "/History/" + key);
       json.iteratorEnd();
       query.clear();
      }
     countHis = fbdo.intData()-1;
     Firebase.RTDB.setInt(&fbdo, "/"+uid+"/"+name+"/"+String(Node)+"/Count/CountHis", countHis);
   }
 }
}

void checkLineWar(){  
 if (Firebase.RTDB.getInt(&fbdo, "/"+uid+"/"+name+"/"+String(Node)+"/Count/CountWar")) {
   countWar=fbdo.intData();
   if(countWar > 5){// can sua
    QueryFilter query;
    query.orderBy("$key"); 
    query.limitToFirst(1);  // chỉ lấy 1 bản ghi cũ nhất

     if (Firebase.RTDB.getJSON(&fbdo,"/"+uid+"/"+name+"/"+String(Node)+ "/WarHistory", &query)) {
       FirebaseJson json = fbdo.jsonObject();
       size_t count = json.iteratorBegin();   
       String key, value;
       int type;
       json.iteratorGet(0, type, key, value);
       Serial.printf("Key: %s, Value: %s\n", key.c_str(), value.c_str());
       Firebase.RTDB.deleteNode(&fbdo,"/"+uid+"/"+name+"/"+String(Node)+ "/WarHistory/" + key);
       json.iteratorEnd();
       query.clear();
      }
     countWar = fbdo.intData()-1;
     Firebase.RTDB.setInt(&fbdo, "/"+uid+"/"+name+"/"+String(Node)+"/Count/CountWar", countWar);
   }
 }
}

void hisdatabase(float hum1,float hum2,float temp){
 if (Firebase.ready()&& signupOk &&((millis()-prevMillisHis)>timeSetHis|| prevMillisHis==0)){
  prevMillisHis=millis();  
   updateTime();
 
  // String time = chuyenDoi(timeinfo.tm_mday) + "-" + 
  //             chuyenDoi(timeinfo.tm_mon + 1) + "_" +   // tm_mon bắt đầu từ 0 → cộng 1
  //             chuyenDoi(timeinfo.tm_hour) + ":" + 
  //             chuyenDoi(timeinfo.tm_min) + ":" + 
  //             chuyenDoi(timeinfo.tm_sec);
  //Serial.println(time);
  FirebaseJson json;
 // json.set("Time", time);
  json.set("Temp", temp);
  json.set("Hum1", hum1);
  json.set("Hum2", hum2);
  String path ="/"+uid+"/"+name+"/"+String(Node)+"/"+ "History/" + String(now);
   if (Firebase.RTDB.setJSON(&fbdo, path.c_str(),&json)) {
      Serial.print(" - successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
      
      if (Firebase.RTDB.getInt(&fbdo,"/"+uid+"/"+name+"/"+String(Node)+ "/Count/CountHis")) {
        countHis = fbdo.intData() + 1;
        Firebase.RTDB.setInt(&fbdo, "/"+uid+"/"+name+"/"+String(Node)+"/Count/CountHis", countHis);
      }
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
    
 }
}

void getTempWar(){// sua
  if (Firebase.ready()&& signupOk){
    if(!Firebase.RTDB.readStream(&fbdo_s1))
     Serial.printf("stream 1 (Nhiet do dat canh bao) begin error,%s\n\n", fbdo_s1.errorReason().c_str());
    if (fbdo_s1.streamAvailable()){
      if(fbdo_s1.dataType()=="string"){
        sendDataWarning=0;
        tempSet=fbdo_s1.stringData().toFloat();
        Serial.println("Successfull read from"+fbdo_s1.dataPath()+ String(tempSet) +"("+fbdo_s1.dataType()+")");
      }
    }
  }
}

void getLine(){
  if (Firebase.ready()&& signupOk){
    if(!Firebase.RTDB.readStream(&fbdo_s3))
     Serial.printf("stream 3  begin error,%s\n\n", fbdo_s3.errorReason().c_str());
    if (fbdo_s3.streamAvailable()){
      if(fbdo_s3.dataType()=="int"){
        lineSet=fbdo_s3.intData();
        Serial.println("Successfull read from"+fbdo_s3.dataPath()+ String(lineSet) +"("+fbdo_s3.dataType()+")");
        lineSet*=4;
      }
    }
  }
}

void warDatabase(float hum1,float hum2,float temp){
  if (Firebase.ready()&& signupOk &&((millis()-prevMillisWar)>5000|| prevMillisWar==0)){
   prevMillisWar=millis();
   if (temp>tempSet &&((millis()-sendDataWarning)>50000|| sendDataWarning==0)) {
    sendDataWarning=millis();
     updateTime();
  //  getTimeEpoch();
//  String time = chuyenDoi(timeinfo.tm_mday) + "-" + 
//               chuyenDoi(timeinfo.tm_mon + 1) + "_" +   // tm_mon bắt đầu từ 0 → cộng 1
//               chuyenDoi(timeinfo.tm_hour) + ":" + 
//               chuyenDoi(timeinfo.tm_min) + ":" + 
//               chuyenDoi(timeinfo.tm_sec);
    FirebaseJson json;
   // json.set("Time", time);
    json.set("TempWar", tempSet);
    json.set("Temp", temp);
 
  String path = "/"+uid+"/"+name+"/"+String(Node)+"/"+"WarHistory/" + String(now);
  if (Firebase.RTDB.setJSON(&fbdo, path.c_str(),&json)) {
      Serial.print(" - successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
      if (Firebase.RTDB.getInt(&fbdo,"/"+uid+"/"+name+"/"+String(Node)+ "/Count/CountWar")) {
        countWar = fbdo.intData() + 1;
        Firebase.RTDB.setInt(&fbdo,"/"+uid+"/"+name+"/"+String(Node)+ "/Count/CountWar", countWar);
      }
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
  }
 }
}
void truyenChinh(float hum1,float hum2,float temp){
  if (Firebase.ready()&& signupOk &&((millis()-sendDataPrevMillis)>5000|| sendDataPrevMillis==0)){
  sendDataPrevMillis=millis();
  if (Firebase.RTDB.setFloat(&fbdo,"/"+uid+"/"+name+"/"+String(Node)+ "/Sensor/temp", temp)) {
      Serial.println();
      Serial.print(temp);
      Serial.print(" - successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.setFloat(&fbdo,"/"+uid+"/"+name+"/"+String(Node)+ "/Sensor/hum1", hum1)) {
      Serial.println();
      Serial.print(hum1);
      Serial.print(" - successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
     if (Firebase.RTDB.setFloat(&fbdo,"/"+uid+"/"+name+"/"+String(Node)+ "/Sensor/hum2", hum2)) {
      Serial.println();
      Serial.print(hum2);
      Serial.print(" - successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
 }
}
void setWifi(){
     // connection firebase
  config.api_key=API_KEY;
  config.database_url=DATABASE_URL;
  if (Firebase.signUp(&config,&auth,"","")){
    Serial.println("singup ok");
    signupOk=true;
  }else{
    Serial.printf("%s\n",config.signer.signupError.message.c_str());
  }
  config.token_status_callback= tokenStatusCallback;
  Firebase.reconnectWiFi(true);
  fbdo.setBSSLBufferSize(4096, 1024);
  Firebase.begin(&config, &auth);

  
  delay(1500);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
}
if (!Firebase.RTDB.pathExisted(&fbdo, "/"+uid+"/"+name+"/"+String(Node)+"/Warning/TempWarSet")) {
    Firebase.RTDB.setFloat(&fbdo, "/"+uid+"/"+name+"/"+String(Node)+"/Warning/TempWarSet", tempSet);
    Serial.println("✅ Tạo TempWarSet mặc định = 35.0°C");
}
if (!Firebase.RTDB.pathExisted(&fbdo, "/"+uid+"/"+name+"/"+String(Node)+"/TimeSetInHistory/TimeSet")) {
    Firebase.RTDB.setFloat(&fbdo, "/"+uid+"/"+name+"/"+String(Node)+"/TimeSetInHistory/TimeSet", timeSetHis);
    Serial.println("✅ Tạo timeSetHis mặc định = 30 phút");
}
if (!Firebase.RTDB.pathExisted(&fbdo, "/"+uid+"/"+name +"/"+String(Node)+"/LineSet/LineSet")) {
    Firebase.RTDB.setFloat(&fbdo, "/"+uid+"/"+name+"/"+String(Node)+ "/LineSet/LineSet", lineSet);
    Serial.println("✅ Tạo lineSet mặc định 100 dòng");
}
if (!Firebase.RTDB.pathExisted(&fbdo, "/"+uid+"/"+name+"/"+String(Node)+ "/Count/CountHis")) {
    Firebase.RTDB.setInt(&fbdo, "/"+uid+"/"+name+"/"+String(Node)+ "/Count/CountHis", 0);
    Serial.println("0 dong cai ban dau");
}
if (!Firebase.RTDB.pathExisted(&fbdo, "/"+uid+"/"+name+"/"+String(Node)+ "/Count/CountWar")) {
    Firebase.RTDB.setInt(&fbdo, "/"+uid+"/"+name+"/"+String(Node)+ "/Count/CountWar", 0);
    Serial.println("0 dong cai ban dau");
}

if (!Firebase.RTDB.beginStream(&fbdo_s1, "/"+uid+"/"+name+"/"+String(Node)+"/Warning/TempWarSet"))
  Serial.printf("stream 1 (Nhiet do dat canh bao) begin error,%s\n\n", fbdo_s1.errorReason().c_str());
if (Firebase.RTDB.getFloat(&fbdo, "/"+uid+"/"+name+"/"+String(Node)+"/Warning/TempWarSet")) {
   tempSet=fbdo.floatData();}
 if (!Firebase.RTDB.beginStream(&fbdo_s2, "/"+uid+"/"+name+"/"+String(Node)+"/TimeSetInHistory/TimeSet"))
  Serial.printf("stream 2 (Truyen du lieu luu tru len database ) begin error,%s\n\n", fbdo_s2.errorReason().c_str());
if (Firebase.RTDB.getInt(&fbdo,"/"+uid+"/"+name+"/"+String(Node)+"/TimeSetInHistory/TimeSet")) {
   timeSetHis=fbdo.intData()*1000;}
if (!Firebase.RTDB.beginStream(&fbdo_s3,"/"+uid+"/"+name+"/"+String(Node)+ "/LineSet/LineSet"))
  Serial.printf("stream 3 (Nhan du lieu so dong hien thi ) begin error,%s\n\n", fbdo_s3.errorReason().c_str());
   if (Firebase.RTDB.getInt(&fbdo,"/"+uid+"/"+name+"/"+String(Node)+ "/LineSet/LineSet")) {
   lineSet=fbdo.intData();}
// if (!Firebase.RTDB.beginStream(&fbdo_s1, "/"+uid+"/"+name+"/"+String(Node)+"/Warning/TempWarSet"))// sua
//   Serial.printf("stream 1 (Nhiet do dat canh bao) begin error,%s\n\n", fbdo_s1.errorReason().c_str());

//   if (!Firebase.RTDB.beginStream(&fbdo_s2, "/"+uid+"/"+name+"/"+String(Node)+"/TimeSetInHistory/TimeSet/"))
//   Serial.printf("stream 2 (Truyen du lieu luu tru len database ) begin error,%s\n\n", fbdo_s2.errorReason().c_str());

// if (!Firebase.RTDB.beginStream(&fbdo_s3,"/"+uid+"/"+name+"/"+String(Node)+ "/LineSet/LineSet/"))
//   Serial.printf("stream 3 (Nhan du lieu so dong hien thi ) begin error,%s\n\n", fbdo_s3.errorReason().c_str());

}
void loopWifi(){
  if (checktime==0){
   getTime();
   checktime=1;}
 if ((millis()-prevMillisMain)>5000){
  prevMillisMain=millis(); 
  Hum1=f.hum1 ;// can sua
  Hum2=f.hum2 ;// can sua
  Temp=f.temp;// can sua
 }
 getLine();
  if ((millis()-deleteHisMillis)>10000){
    deleteHisMillis=millis(); 
  checkLineHis();
  }
  if ((millis()-deleteWarMillis)>10000){
    deleteWarMillis=millis(); 
  checkLineWar();
  }
  truyenChinh(Hum1,Hum2,Temp);
  setTimeHis();
  hisdatabase(Hum1,Hum2, Temp);
  getTempWar();
  warDatabase(Hum1,Hum2, Temp);
}

class Config {
public:
  void begin(){
    
    pinMode(ledPin, OUTPUT);
    pinMode(btnPin, INPUT_PULLUP);
    blinker.attach_ms(50, ledControl);
    
    EEPROM.begin(256);
    char ssid_temp[32], password_temp[64], uid_temp[64], name_temp[32];
    EEPROM.readString(0,ssid_temp, sizeof(ssid_temp));
    EEPROM.readString(32,password_temp,sizeof(password_temp));
    EEPROM.readString(100,uid_temp,sizeof(uid_temp));
    EEPROM.readString(200,name_temp,sizeof(name_temp));
    ssid = String(ssid_temp);
    password = String(password_temp);
    uid=String(uid_temp);
    name=String(name_temp);
    setupWifi(); // Thiết lập WiFi
    if(wifiMode == 0 ) setupWebServer();
    if (wifiMode==1){
      setWifi();
    }
     delay(100);  // Đợi WiFi khởi động
  }

  void run(){
    checkButton();
     if(wifiMode == 0  ) webServer.handleClient();
     if (wifiMode==1 ){
      loopWifi();
     }
  }
} wifiConfig;


static bool extractFloat(const char *s, const char *key, float *out)
{
  const char *p = strstr(s, key);
  if (!p) return false;
  p += strlen(key);
  char *endp = nullptr;
  float v = strtof(p, &endp);
  if (endp == p) return false;
  *out = v;
  return true;
}

static bool extractInt(const char *s, const char *key, int *out)
{
  const char *p = strstr(s, key);
  if (!p) return false;
  p += strlen(key);
  *out = atoi(p);
  return true;
}

bool parseFrameLineFloat(const char *line, FrameDataF_t *out)
{
 // if (!line || !out) return false;

  // if (strstr(line, "Frame VALID") != nullptr) out->valid = true;
  // else if (strstr(line, "Frame INVALID") != nullptr) out->valid = false;
 // else return false;

  int v;
  if (!extractInt(line, "SrcID=", &v)) return false;
  out->srcid = (uint8_t)v;

  if (!extractFloat(line, "Temp=", &out->temp)) return false;
  if (!extractFloat(line, "Hum1=",  &out->hum1))  return false;
  if (!extractFloat(line, "Hum2=",  &out->hum2))  return false;

  return true;
}

HardwareSerial MySerial(2);

static bool readLine(HardwareSerial &uart, char *buf, size_t buflen)
{
  size_t n = uart.readBytesUntil('\n', buf, buflen - 1);
  if (n == 0) return false;
  buf[n] = '\0';
  if (n > 0 && buf[n - 1] == '\r') buf[n - 1] = '\0';
  return true;
}

void setup() {
  Serial.begin(9600);
  MySerial.begin(9600, SERIAL_8N1, 16, 17);
  MySerial.setTimeout(200); // tuỳ chọn, đỡ chờ lâu
  wifiConfig.begin();
}

void loop() {
   char c[] = "Frame VALID: SrcID=1, DestID=0, Type=3, Temp=23, Hum1=53, Hum2=26";
  
 
  if (parseFrameLineFloat(c, &f)) {
       Serial.printf("PARSED: src=%u temp=%.2f hum1=%.2f hum2=%.2f\r\n",
                    f.srcid, f.temp, f.hum1, f.hum2);
     } else {
       Serial.println("Parse FAIL");}
delay(2000);
Temp=f.temp;
Hum1 =f.hum1;
Hum2 =f.hum2;
Node=f.srcid;

wifiConfig.run();

//static char lineBuf[220];
  // if (readLine(MySerial, lineBuf, sizeof(lineBuf))) {
  //   Serial.print("RAW: ");
  //   Serial.println(lineBuf);

  //   if (parseFrameLineFloat(lineBuf, &f)) {
  //     Serial.printf("PARSED: src=%u temp=%.2f hum1=%.2f hum2=%.2f\r\n",
  //                   f.valid, f.srcid, f.temp, f.hum1, f.hum2);
  //   } else {
  //     Serial.println("Parse FAIL");
  //   }
  // }
}
// void loop() {
//   // while (MySerial.available()) {
//   //   uint8_t b = MySerial.read();
//   //   Serial.printf("%02X ", b);
//   // }
//   while (MySerial.available()) { int c = MySerial.read(); Serial.write(c); // in ra Serial Monitor }
// }}
