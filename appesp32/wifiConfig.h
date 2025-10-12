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
unsigned long countHis = 0;
int checktime=0;
unsigned long countWar=0;
bool signupOk = false;
float Temp=0.0;
float Hum=0.0;
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

// Xử lý sự kiện WiFi
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
        // Sự kiện khi ESP32 đã kết nối với AP thành công (chưa lấy IP)
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("WiFi connected!");
             wifiMode = 1; 
            break;

        // Sự kiện khi ESP32 bị mất kết nối WiFi
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            
                // Nếu số lần mất kết nối < 10 lần thì thử kết nối lại Wi-Fi
                Serial.println("WiFi lost connection.");
                wifiMode = 2; // Đặt chế độ Wi-Fi về 2 (đang cố gắng reconnect)
                       // Đợi ngắt kết nối hoàn tất
              
                delay(2000);
            break;
        // Mặc định khi không khớp với bất kỳ sự kiện nào ở trên
        default:
            Serial.print("No ");
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

   // Route "/saveWifi" lưu SSID và mật khẩu vào EEPROM
    webServer.on("/saveWifi", []() {
      Serial.println(" da vao saveWifi");
      String ssid_temp = webServer.arg("ssid");
      String password_temp = webServer.arg("pass");
       String uid_temp = webServer.arg("uid");
        String name_temp = webServer.arg("name");
      Serial.println("SSID:"+ssid_temp);
      Serial.println("PASS:"+password_temp);
      Serial.println("UID:"+uid_temp);
      Serial.println("NAME:"+name_temp);
      EEPROM.writeString(0,ssid_temp);
      EEPROM.writeString(32,password_temp);
       EEPROM.writeString(100,uid_temp);
       EEPROM.writeString(200,name_temp);
      EEPROM.commit();
      webServer.send(200,"text/plain","Wifi has been saved!");
    });

   webServer.on("/reStart", []{
       Serial.println(" da vao restart");
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
  if (Firebase.RTDB.getInt(&fbdo, "/Count/CountHis")) {
   countHis=fbdo.intData();
   if(countHis > 5){
    QueryFilter query;
    query.orderBy("$key"); 
    query.limitToFirst(1);  // chỉ lấy 1 bản ghi cũ nhất

     if (Firebase.RTDB.getJSON(&fbdo, "/History", &query)) {
       FirebaseJson json = fbdo.jsonObject();
       size_t count = json.iteratorBegin();   
       String key, value;
       int type;
       json.iteratorGet(0, type, key, value);
       Serial.printf("Key: %s, Value: %s\n", key.c_str(), value.c_str());
       Firebase.RTDB.deleteNode(&fbdo, "/History/" + key);
       json.iteratorEnd();
       query.clear();
      }
     countHis = fbdo.intData()-1;
     Firebase.RTDB.setInt(&fbdo, "/Count/CountHis", countHis);
   }
 }
}
void checkLineWar(){  
 if (Firebase.RTDB.getInt(&fbdo, "/Count/CountWar")) {
   countWar=fbdo.intData();
   if(countWar > 5){
    QueryFilter query;
    query.orderBy("$key"); 
    query.limitToFirst(1);  // chỉ lấy 1 bản ghi cũ nhất

     if (Firebase.RTDB.getJSON(&fbdo, "/WarHistory", &query)) {
       FirebaseJson json = fbdo.jsonObject();
       size_t count = json.iteratorBegin();   
       String key, value;
       int type;
       json.iteratorGet(0, type, key, value);
       Serial.printf("Key: %s, Value: %s\n", key.c_str(), value.c_str());
       Firebase.RTDB.deleteNode(&fbdo, "/WarHistory/" + key);
       json.iteratorEnd();
       query.clear();
      }
     countWar = fbdo.intData()-1;
     Firebase.RTDB.setInt(&fbdo, "/Count/CountWar", countWar);
   }
 }
}
void hisdatabase(float hum,float temp){
 if (Firebase.ready()&& signupOk &&((millis()-prevMillisHis)>timeSetHis|| prevMillisHis==0)){
  prevMillisHis=millis();  
  getTime();
  //getTimeEpoch();
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

  String path = "History/" + String(now);
   if (Firebase.RTDB.setJSON(&fbdo, path.c_str(),&json)) {
      Serial.print(" - successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
      
      if (Firebase.RTDB.getInt(&fbdo, "/Count/CountHis")) {
        countHis = fbdo.intData() + 1;
        Firebase.RTDB.setInt(&fbdo, "/Count/CountHis", countHis);
      }
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
    
 }
}

void getTempWar(){
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

void warDatabase(float hum,float temp){
  if (Firebase.ready()&& signupOk &&((millis()-prevMillisWar)>5000|| prevMillisWar==0)){
   prevMillisWar=millis();
   if (temp>tempSet &&((millis()-sendDataWarning)>50000|| sendDataWarning==0)) {
    sendDataWarning=millis();
    getTime();
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
 
  String path = "WarHistory/" + String(now);
  if (Firebase.RTDB.setJSON(&fbdo, path.c_str(),&json)) {
      Serial.print(" - successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
      if (Firebase.RTDB.getInt(&fbdo, "/Count/CountWar")) {
        countWar = fbdo.intData() + 1;
        Firebase.RTDB.setInt(&fbdo, "/Count/CountWar", countWar);
      }
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
  }
 }
}
void truyenChinh(float hum,float temp){
  if (Firebase.ready()&& signupOk &&((millis()-sendDataPrevMillis)>5000|| sendDataPrevMillis==0)){
  sendDataPrevMillis=millis();
  if (Firebase.RTDB.setFloat(&fbdo, "Sensor/temp", temp)) {
      Serial.println();
      Serial.print(temp);
      Serial.print(" - successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.setFloat(&fbdo, "Sensor/hum", hum)) {
      Serial.println();
      Serial.print(hum);
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

  // setup dht22
  DHT22.begin();
  delay(1500);
  // Cấu hình NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
}
  // cau hinh stream
if (!Firebase.RTDB.beginStream(&fbdo_s1, "/Warning/TempWarSet"))
  Serial.printf("stream 1 (Nhiet do dat canh bao) begin error,%s\n\n", fbdo_s1.errorReason().c_str());
  if (!Firebase.RTDB.beginStream(&fbdo_s2, "/TimeSetInHistory/TimeSet/"))
  Serial.printf("stream 2 (Truyen du lieu luu tru len database ) begin error,%s\n\n", fbdo_s2.errorReason().c_str());
if (!Firebase.RTDB.beginStream(&fbdo_s3, "/LineSet/LineSet/"))
  Serial.printf("stream 3 (Nhan du lieu so dong hien thi ) begin error,%s\n\n", fbdo_s3.errorReason().c_str());
   
}
void loopWifi(){
  if (checktime==0){
   getTime();
   checktime=1;}
 if ((millis()-prevMillisMain)>5000){
  prevMillisMain=millis(); 
  Hum= DHT22.runHum();
  Temp=DHT22.runTemp();
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
    if(ssid != ""){
      Serial.println("WiFi name: " + ssid);
      Serial.println("Password: " + password);
      Serial.println("uid: " +uid );
      Serial.println("name: " +name );
    }
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
