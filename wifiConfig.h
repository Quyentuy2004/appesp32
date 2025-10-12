#include <EEPROM.h> // Tên WiFi và mật khẩu lưu vào ô nhớ 0 -> 96
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h> // Thêm thư viện WebServer
#include <Ticker.h>

#define ENABLE_RTDB          // bật Realtime Database
//#define ENABLE_FCM         // tắt FCM
//#define ENABLE_FIRESTORE   // tắt Firestore
//#define ENABLE_STORAGE     // tắt Storage


#include<Firebase_ESP_Client.h>
#include"addons/TokenHelper.h"
#include"addons/RTDBHelper.h"
#include"dodht22.h"


Ticker blinker;


#define ledPin 2
#define btnPin 0
#define PUSHTIME 5000

#define API_KEY "AIzaSyB_aDU1LR4WPYFR7DPktUHW3tQdmKxYkuM"
#define DATABASE_URL "https://doan22-e15f5-default-rtdb.asia-southeast1.firebasedatabase.app/"
FirebaseData fbdo, fbdo_s2, fbdo_s1,fbdo_s3;
FirebaseAuth auth;
FirebaseConfig config;

struct tm timeinfo;
unsigned long deleteHisMillis = 0;
unsigned long deleteWarMillis = 0;
unsigned long sendDataPrevMillis = 0;
unsigned long prevMillisMain = 0;
unsigned long prevMillisHis = 0;
unsigned long prevMillisWar = 0;
unsigned long sendDataWarning=0;
unsigned long timeOnl=0;
unsigned long countHis = 0;
int checktime=0;
unsigned long countWar=0;
bool signupOk = false;
float Temp=0.0;
float Hum=0.0;
unsigned long lineSet= 100;
unsigned long timeSetHis=1800000;
int retry = 0;
float tempSet=35.0;
time_t now;
// Máy chủ NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600; // GMT+7 cho Việt Nam
const int   daylightOffset_sec = 0;

const char* ntpServer1 = "time.google.com";
const char* ntpServer2 = "time.cloudflare.com";
const char* ntpServer3 = "pool.ntp.org";

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

// Xử lý sự kiện WiFi
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
 
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            
             wifiMode = 1; 
            break;

    
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            
              
                wifiMode = 2; // Đặt chế độ Wi-Fi về 2 (đang cố gắng reconnect)

              
                delay(2000);
            break;
        // Mặc định khi không khớp với bất kỳ sự kiện nào ở trên
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
    //WiFi.setAutoReconnect(true);
   // WiFi.persistent(true);  // lưu config vào flash
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
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2, ntpServer3);
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
      time(&now); // lưu epoch time
    }
  }else{
    Serial.println("ESP32 WiFi network created!");
    WiFi.mode(WIFI_AP);
    String ssid_ap = "ESP32" ;
    WiFi.softAP(ssid_ap.c_str());

    wifiMode = 0;
  }
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
        
          doc.add(WiFi.SSID(i));
        }
       String wifiList = "";
       serializeJson(doc, wifiList);
    
       webServer.send(200, "application/json", wifiList);
      });
    webServer.on("/saveWifi", []() {
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
 
  webServer.begin(); // Khởi chạy dịch vụ WebServer trên ESP32
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
void setTimeHis(){
  if (Firebase.ready() && signupOk) {
    if (!Firebase.RTDB.readStream(&fbdo_s2)) {
      Serial.println("❌ readStream s2 error: " + fbdo_s2.errorReason());
      return;
    }
    if (fbdo_s2.streamAvailable()) {
      Serial.println("String(fbdo_s2.dataType())");
      if (fbdo_s2.dataType() == "int"||fbdo_s2.dataType() == "String") {
        timeSetHis = fbdo_s2.intData() * 1000; // đổi giây thành ms
        Serial.println("⏳ TimeSetHis updated: " + String(timeSetHis));
      }
    }
  }
}
void updateTime() {
  time(&now); // lấy epoch hiện tại
  localtime_r(&now, &timeinfo); // convert sang struct tm
}

void checkLineHis() {
  if (Firebase.RTDB.getInt(&fbdo,"/"+uid+"/"+name+ "/Count/CountHis")) {
   countHis=fbdo.intData();
   if(countHis > lineSet){
    QueryFilter query;
    query.orderBy("$key"); 
    query.limitToFirst(1);  // chỉ lấy 1 bản ghi cũ nhất

     if (Firebase.RTDB.getJSON(&fbdo, "/"+uid+"/"+name+"/History", &query)) {
       FirebaseJson json = fbdo.jsonObject();
       size_t count = json.iteratorBegin();   
       String key, value;
       int type;
       json.iteratorGet(0, type, key, value);
       Serial.printf("Key: %s, Value: %s\n", key.c_str(), value.c_str());
       Firebase.RTDB.deleteNode(&fbdo, "/"+uid+"/"+name+"/History/" + key);
       json.iteratorEnd();
       query.clear();
      }
     countHis = fbdo.intData()-1;
     Firebase.RTDB.setInt(&fbdo, "/"+uid+"/"+name+"/Count/CountHis", countHis);
   }
 }
}
void checkLineWar(){  
 if (Firebase.RTDB.getInt(&fbdo,"/"+uid+"/"+name+ "/Count/CountWar")) {
   countWar=fbdo.intData();
   if(countWar > lineSet){
    QueryFilter query;
    query.orderBy("$key"); 
    query.limitToFirst(1);  // chỉ lấy 1 bản ghi cũ nhất

     if (Firebase.RTDB.getJSON(&fbdo, "/"+uid+"/"+name+"/WarHistory", &query)) {
       FirebaseJson json = fbdo.jsonObject();
       size_t count = json.iteratorBegin();   
       String key, value;
       int type;
       json.iteratorGet(0, type, key, value);
       Serial.printf("Key: %s, Value: %s\n", key.c_str(), value.c_str());
       Firebase.RTDB.deleteNode(&fbdo, "/"+uid+"/"+name+"/WarHistory/" + key);
       json.iteratorEnd();
       query.clear();
      }
     countWar = fbdo.intData()-1;
     Firebase.RTDB.setInt(&fbdo, "/"+uid+"/"+name+"/Count/CountWar", countWar);
   }
 }
}
void hisdatabase(float hum,float temp){
  
 if (Firebase.ready()&& signupOk &&((millis()-prevMillisHis)>timeSetHis|| prevMillisHis==0)){
  prevMillisHis=millis();  
    updateTime();
  String time = chuyenDoi(timeinfo.tm_mday) + "-" + 
              chuyenDoi(timeinfo.tm_mon + 1) + "_" +   // tm_mon bắt đầu từ 0 → cộng 1
              chuyenDoi(timeinfo.tm_hour) + ":" + 
              chuyenDoi(timeinfo.tm_min) + ":" + 
              chuyenDoi(timeinfo.tm_sec);
  Serial.println(time);
  FirebaseJson json;
  json.set("Time", time);
  json.set("Temp", temp);
  json.set("Hum", hum);

  String path = "/"+uid+"/"+name+ "/History/" + String(now);
   if (Firebase.RTDB.setJSON(&fbdo, path.c_str(),&json)) {
      
      
      if (Firebase.RTDB.getInt(&fbdo, "/"+uid+"/"+name+"/Count/CountHis")) {
        countHis = fbdo.intData() + 1;
        Firebase.RTDB.setInt(&fbdo, "/"+uid+"/"+name+"/Count/CountHis", countHis);
      }
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
    
 }
}

void getTempWar(){
  if (Firebase.ready()&& signupOk){
    if (!Firebase.RTDB.readStream(&fbdo_s1)) {
      Serial.println("❌ readStream s1 error: " + fbdo_s1.errorReason());
      return;
    }
    if (fbdo_s1.streamAvailable()){
      if(fbdo_s1.dataType()=="string"){
        sendDataWarning=0;
        tempSet=fbdo_s1.stringData().toFloat();
      
      }
    }
  }
}

void getLine(){
  if (Firebase.ready()&& signupOk){
    if (!Firebase.RTDB.readStream(&fbdo_s3)) {
      Serial.println("❌ readStream s3 error: " + fbdo_s3.errorReason());
      return;
    }
    if (fbdo_s3.streamAvailable()){
      
      if(fbdo_s3.dataType()=="int"){
        lineSet=fbdo_s3.intData();
      
        lineSet*=4;
      }
    }
  }
}

void warDatabase(float hum,float temp){
  if (Firebase.ready()&& signupOk &&((millis()-prevMillisWar)>5000|| prevMillisWar==0)){
   prevMillisWar=millis();
   if (temp>tempSet &&((millis()-sendDataWarning)>50000|| sendDataWarning==0)) {
    sendDataWarning=millis();
    updateTime();
  //  getTimeEpoch();
 String time = chuyenDoi(timeinfo.tm_mday) + "-" + 
              chuyenDoi(timeinfo.tm_mon + 1) + "_" +   // tm_mon bắt đầu từ 0 → cộng 1
              chuyenDoi(timeinfo.tm_hour) + ":" + 
              chuyenDoi(timeinfo.tm_min) + ":" + 
              chuyenDoi(timeinfo.tm_sec);
    FirebaseJson json;
    json.set("Time", time);
    json.set("TempWar", tempSet);
    json.set("Temp", temp);
 
  String path =  "/"+uid+"/"+name+"/WarHistory/" + String(now);
  if (Firebase.RTDB.setJSON(&fbdo, path.c_str(),&json)) {
     
      if (Firebase.RTDB.getInt(&fbdo, "/"+uid+"/"+name+"/Count/CountWar")) {
        countWar = fbdo.intData() + 1;
        Firebase.RTDB.setInt(&fbdo, "/"+uid+"/"+name+"/Count/CountWar", countWar);
      }
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
  }
 }
}
void capNhatOnl(){
if (Firebase.RTDB.setTimestamp(&fbdo, "/User/"+uid+"/"+name)) {
    Serial.println("Đã cập nhật timestamp!");
  } else {
    Serial.println(fbdo.errorReason());
  }
}
void truyenChinh(float hum,float temp){
  if (Firebase.ready()&& signupOk &&((millis()-sendDataPrevMillis)>5000|| sendDataPrevMillis==0)){
  sendDataPrevMillis=millis();
  if (Firebase.RTDB.setFloat(&fbdo,"/"+uid+"/"+name+ "/Sensor/temp", temp)) {
      
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.setFloat(&fbdo,"/"+uid+"/"+name+ "/Sensor/hum", hum)) {
     
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

  // setup dht22
  DHT22.begin();
  delay(1500);
  // Cấu hình NTP
 
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
}
if (!Firebase.RTDB.pathExisted(&fbdo, "/"+uid+"/"+name+"/Warning/TempWarSet")) {
    Firebase.RTDB.setFloat(&fbdo, "/"+uid+"/"+name+"/Warning/TempWarSet", tempSet);
    Serial.println("✅ Tạo TempWarSet mặc định = 35.0°C");
}
if (!Firebase.RTDB.pathExisted(&fbdo, "/"+uid+"/"+name+"/TimeSetInHistory/TimeSet")) {
    Firebase.RTDB.setFloat(&fbdo, "/"+uid+"/"+name+"/TimeSetInHistory/TimeSet", timeSetHis);
    Serial.println("✅ Tạo timeSetHis mặc định = 30 phút");
}
if (!Firebase.RTDB.pathExisted(&fbdo, "/"+uid+"/"+name+ "/LineSet/LineSet")) {
    Firebase.RTDB.setFloat(&fbdo, "/"+uid+"/"+name+ "/LineSet/LineSet", lineSet);
    Serial.println("✅ Tạo lineSet mặc định 100 dòng");
}
  // cau hinh stream
if (!Firebase.RTDB.beginStream(&fbdo_s1, "/"+uid+"/"+name+"/Warning/TempWarSet"))
  Serial.printf("stream 1 (Nhiet do dat canh bao) begin error,%s\n\n", fbdo_s1.errorReason().c_str());
if (Firebase.RTDB.getFloat(&fbdo, "/"+uid+"/"+name+"/Warning/TempWarSet")) {
   tempSet=fbdo.floatData();}
 if (!Firebase.RTDB.beginStream(&fbdo_s2, "/"+uid+"/"+name+"/TimeSetInHistory/TimeSet"))
  Serial.printf("stream 2 (Truyen du lieu luu tru len database ) begin error,%s\n\n", fbdo_s2.errorReason().c_str());
if (Firebase.RTDB.getInt(&fbdo,"/"+uid+"/"+name+"/TimeSetInHistory/TimeSet")) {
   timeSetHis=fbdo.intData()*1000;}
if (!Firebase.RTDB.beginStream(&fbdo_s3,"/"+uid+"/"+name+ "/LineSet/LineSet"))
  Serial.printf("stream 3 (Nhan du lieu so dong hien thi ) begin error,%s\n\n", fbdo_s3.errorReason().c_str());
   if (Firebase.RTDB.getInt(&fbdo,"/"+uid+"/"+name+ "/LineSet/LineSet")) {
   lineSet=fbdo.intData()*4;}
}
void loopWifi(){
 if(((millis()-prevMillisMain)>5000)||prevMillisMain==0){
  prevMillisMain=millis(); 
  Hum= DHT22.runHum();
  Temp=DHT22.runTemp();
 }
 if(((millis()-timeOnl)>5000)||timeOnl==0){
  timeOnl=millis(); 
 capNhatOnl();
 }
 getLine();
  if ((millis()-deleteHisMillis)>(timeSetHis/5)){
    deleteHisMillis=millis(); 
  checkLineHis();
  }
  if ((millis()-deleteWarMillis)>10000){
    deleteWarMillis=millis(); 
  checkLineWar();
  }
  truyenChinh(Hum,Temp);
  setTimeHis();
  hisdatabase(Hum, Temp);
  getTempWar();
  warDatabase(Hum, Temp);
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
     if(wifiMode == 0) webServer.handleClient();
     if (wifiMode==1){
      loopWifi();
     }
  }
} wifiConfig;
