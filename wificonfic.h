#include <EEPROM.h> // Tên WiFi và mật khẩu lưu vào ô nhớ 0 -> 96
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h> // Thêm thư viện WebServer
#include <Ticker.h>
#include <HardwareSerial.h>

#define UART_RX 16
#define UART_TX 17
#define UART_BAUD 115200
#define ENABLE_RTDB          // bật Realtime Database
//#define ENABLE_FCM         // tắt FCM
//#define ENABLE_FIRESTORE   // tắt Firestore
//#define ENABLE_STORAGE     // tắt Storage


#include<Firebase_ESP_Client.h>
#include"addons/TokenHelper.h"
#include"addons/RTDBHelper.h"
//#include"dodht22.h"


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
float Soil=0.0;
int node=0;
unsigned long lineSet= 0;
unsigned long timeSetGateway=1800;
float soilSet=100;
float humSet=100;
float tempSet=100;
int retry = 0;

String uartLine = "";
int srcId = -1;
float temp1 = 0;
float hum1 = 0;
float hum2 = 0;
int lostId=-1;


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

void deleteNodeById(const String& uid, const String& gateway, int nodeId) {
  // ---- CHỌN 1 CÁCH ĐẶT TÊN NODE ----
  // C1: nodeKey = "3"
  // String nodeKey = String(nodeId);

  // C2: nodeKey = "Node_03"
  

  String path = "/" + uid + "/" + gateway + "/nodes/" + nodeId;
  Serial.print("Delete path: "); Serial.println(path);

  if (Firebase.RTDB.deleteNode(&fbdo, path.c_str())) {
    return ;
  } else {
    Serial.print("Delete fail: ");
    Serial.println(fbdo.errorReason());
    return ;
  }
}

bool parseNodeLost(const String &line, int &nodeIdOut) {
  // Ví dụ line: "NODE ID SO: 3 MAT KET NOI"

  int p = line.indexOf("NODE ID SO");
  if (p < 0) return false;

  int colon = line.indexOf(':', p);
  if (colon < 0) return false;

  // Lấy phần sau dấu :
  String idPart = line.substring(colon + 1);
  idPart.trim();   // bỏ khoảng trắng đầu cuối

  // idPart lúc này là: "3 MAT KET NOI"
  // toInt() sẽ tự lấy số đầu tiên -> 3
  nodeIdOut = idPart.toInt();

  return (nodeIdOut > 0);
}
bool getValueInt(const String &s, const char *key, int &out) {
  int p = s.indexOf(key);
  if (p < 0) return false;
  p += strlen(key);

  int end = s.indexOf(',', p);
  if (end < 0) end = s.length();

  String v = s.substring(p, end);
  v.trim();
  out = v.toInt();
  return true;
}

bool getValueFloat(const String &s, const char *key, float &out) {
  int p = s.indexOf(key);
  if (p < 0) return false;
  p += strlen(key);

  int end = s.indexOf(',', p);
  if (end < 0) end = s.length();

  String v = s.substring(p, end);
  v.trim();
  out = v.toFloat();
  return true;
}

bool parseFrame(const String &line, int &src, float &t, float &h1, float &h2) {
  // chỉ parse khi đúng frame
  if (!line.startsWith("Frame VALID:")) return false;

  bool ok = true;
  ok &= getValueInt(line, "SrcID=", src);
  ok &= getValueFloat(line, "Temp1=", t);
  ok &= getValueFloat(line, "Hum1=", h1);
  ok &= getValueFloat(line, "Hum2=", h2);

  return ok;
}


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
void clearRange(int start, int len){
  for(int i = 0; i < len; i++) {
    EEPROM.write(start + i, 0);
  }
}
void saveWifiToEEPROMAndRestart(const String& newSsid, const String& newPass) {
  // Nếu không thay đổi thì khỏi làm gì
  Serial.printf("%s\n", newSsid);
   Serial.printf("%s\n", newPass);
  if (newSsid == ssid && newPass == password) return;
  Serial.printf("khac, %s\n", newSsid);
   Serial.printf("khac, %s\n", newPass);
  Serial.println("✅ WiFi changed from Firebase -> saving to EEPROM...");

   clearRange(0, 32);    // vùng ssid
  clearRange(32, 64);   // vùng pass

  EEPROM.writeString(0, newSsid);
  EEPROM.writeString(32, newPass);
  EEPROM.commit();

  Serial.println("✅ Saved. Restarting ESP32...");
  delay(1500);
  ESP.restart();
}
void handleWifiStream() {
  if (!Firebase.ready() || !signupOk ) return;

  if (!Firebase.RTDB.readStream(&fbdo_s3)) {
    // stream lỗi thì bỏ qua, lần sau đọc lại
    return;
  }

  if (!fbdo_s3.streamAvailable()) return;
Serial.printf("nhan thay su thay doi co ssid va password");
  // Trường hợp Firebase gửi cả node wifi dưới dạng JSON
  if (fbdo_s3.dataType() == "json") {
    Serial.printf("nhan thay su thay doi co ssid va password bang json");
    FirebaseJson& js = fbdo_s3.jsonObject();

    FirebaseJsonData d1, d2;
    js.get(d1, "ssid");
    js.get(d2, "password");

    String newSsid = d1.success ? d1.to<String>() : "";
    String newPass = d2.success ? d2.to<String>() : "";

    // Chỉ xử lý nếu có dữ liệu hợp lệ
    if (newSsid.length() > 0) {
      Serial.println("📥 Firebase wifi update: ssid=" + newSsid);
      saveWifiToEEPROMAndRestart(newSsid, newPass);
    }
    return;
  }

  // // Trường hợp Firebase update từng field (ssid hoặc password)
  // String path = fbdo_setting.dataPath();   // ví dụ "/ssid" hoặc "/password"
  // String full = fbdo_setting.dataType();   // type

  // if (path == "/ssid" && fbdo_setting.dataType() == "string") {
  //   lastWifiSsid = fbdo_setting.stringData();
  // } else if (path == "/password" && fbdo_setting.dataType() == "string") {
  //   lastWifiPass = fbdo_setting.stringData();
  // }

  // Nếu đã nhận đủ 2 cái thì restart
//  if (lastWifiSsid.length() > 0 && lastWifiPass.length() > 0) {
//   saveWifiToEEPROMAndRestart(lastWifiSsid, lastWifiPass);
// }
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

void setTimeHis(){// thoi gian canh bao thay doi 
 if (Firebase.ready()&& signupOk){
    if(!Firebase.RTDB.readStream(&fbdo_s2))
    Serial.printf("stream 2 (Nhiet do dat canh bao) begin error,%s\n\n", fbdo_s2.errorReason().c_str());
    if (fbdo_s2.streamAvailable()){
      if(fbdo_s2.dataType()=="int"){
        timeSetGateway=fbdo_s2.intData()*1000;
        Serial.println("Successfull read from"+fbdo_s2.dataPath()+ timeSetGateway+"("+fbdo_s2.dataType()+")");
        uint8_t p= (uint8_t) timeSetGateway;
        Serial2.printf("%d",p);
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


void hisdatabase(float hum,float temp,float soil){// can sua khi co du lieu thi truyen luon k cho 30 phut
 if ((Firebase.ready()&& signupOk) ||( prevMillisHis==0)){
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
  json.set("Soil", soil);
  json.set("Temp", temp);
  json.set("Hum", hum);
long long timeNow=now*1000;
  String path = "/"+ uid+"/"+name+"/nodes"+"/"+String(node)+ "/Sensor/" + String(timeNow);
   if (Firebase.RTDB.setJSON(&fbdo, path.c_str(),&json)) {
      Serial.print(" - successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");

    
 }
}
}
void loopUart() {
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\r') continue;
// node=random(1, 4);
//  Hum= random(5, 10)/1.0; 
//  Temp=  random(5, 10)/1.0; 
//  Soil= random(5, 10)/1.0; 
// int srcId = -1;
// float temp1 = 0;
// float hum1 = 0;
// float hum2 = 0;
// int lostId=-1;
//   
//   hisdatabase(Hum, Temp, Soil);// co nam

    if (c == '\n') {
      uartLine.trim();
      if (uartLine.length() > 0) {
        Serial.print("Raw: ");
        Serial.println(uartLine);
 if (parseNodeLost(uartLine, lostId)) {
          Serial.printf(">>> NODE ID %d MAT KET NOI\n", lostId);
// su ly mat node
     deleteNodeById(uid,name, lostId);



        }
        else {
          Serial.println("Parse FAIL (khong dung dinh dang frame xoa node  )");
        }
        if (parseFrame(uartLine, srcId, temp1, hum1, hum2)) {
          Serial.printf("Parsed -> SrcID=%d | Temp1=%.2f | Hum1=%.2f | Hum2=%.2f\n",
                        srcId, temp1, hum1, hum2);
                        // su li nhan thong tin
node=srcId;
 Temp= temp1 ;
 Hum= hum1 ;
 Soil =hum2 ;
hisdatabase(Hum, Temp, Soil);// co nam

        } 
        else {
          Serial.println("Parse FAIL (khong dung dinh dang frame nhan data  )");
        }
      }
      uartLine = "";
    } else {
      if (uartLine.length() < 300) uartLine += c;
      else uartLine = "";
    }
  }
}

void handleThresholdStream() {
   if (!Firebase.ready() || !signupOk) return;

  // đọc stream 1 lần
  if (!Firebase.RTDB.readStream(&fbdo_s1)) {
    // lỗi stream thì bỏ qua, vòng sau đọc lại
    // Serial.printf("threshold stream read error: %s\n", fbdo_s1.errorReason().c_str());
    return;
  }

  if (!fbdo_s1.streamAvailable()) return;

  sendDataWarning = 0; // reset debounce khi user đổi ngưỡng

  // 1) Firebase gửi cả node threshold dạng JSON
  if (fbdo_s1.dataType() == "json") {
    FirebaseJson &js = fbdo_s1.jsonObject();
    FirebaseJsonData d;

    js.get(d, "temp"); if (d.success) tempSet = d.to<float>();
    js.get(d, "hum");  if (d.success) humSet  = d.to<float>();
    js.get(d, "soil"); if (d.success) soilSet = d.to<float>();

    Serial.printf("✅ Threshold JSON -> temp=%.2f hum=%.2f soil=%.2f\n", tempSet, humSet, soilSet);
    return;
  }

  // 2) Firebase update từng field
  String p = fbdo_s1.dataPath();   // "/temp" hoặc "/hum" hoặc "/soil"

  float v = NAN;
  if (fbdo_s1.dataType() == "float" || fbdo_s1.dataType() == "double") v = fbdo_s1.floatData();
  else if (fbdo_s1.dataType() == "int") v = (float)fbdo_s1.intData();
  else if (fbdo_s1.dataType() == "string") v = fbdo_s1.stringData().toFloat();
  else return;

  if (p == "/temp") {
    tempSet = v;
    Serial.printf("✅ tempSet updated: %.2f\n", tempSet);
  } else if (p == "/hum") {
    humSet = v;
    Serial.printf("✅ humSet updated: %.2f\n", humSet);
  } else if (p == "/soil") {
    soilSet = v;
    Serial.printf("✅ soilSet updated: %.2f\n", soilSet);
  }
}


void setWhenisWifi(){
     // connection firebase
      Serial.begin(115200);
  Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX, UART_TX);
  Serial.println("ESP32 ready. Listening UART2...");
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
  // Cấu hình NTP
  configTime( gmtOffset_sec,daylightOffset_sec,"time.google.com","time.cloudflare.com","pool.ntp.org");
 // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
}
if (!Firebase.RTDB.pathExisted(&fbdo, "/"+uid+"/"+name+"/"+"setting/threshold/temp")) {
    Firebase.RTDB.setFloat(&fbdo, "/"+uid+"/"+name+"/"+"setting/threshold/temp", tempSet);
    Serial.println("✅ Tạo TempWarSet mặc định = 35.0°C");
}

if (!Firebase.RTDB.pathExisted(&fbdo, "/"+uid+"/"+name+"/"+"setting/threshold/hum")) {
    Firebase.RTDB.setFloat(&fbdo,  "/"+uid+"/"+name+"/"+"setting/threshold/hum", humSet);
    Serial.println("✅ Tạo TempWarSet mặc định = 35.0°C");
}

if (!Firebase.RTDB.pathExisted(&fbdo, "/"+uid+"/"+name+"/"+"setting/threshold/soil")) {
    Firebase.RTDB.setFloat(&fbdo,  "/"+uid+"/"+name+"/"+"setting/threshold/soil", soilSet);
    Serial.println("✅ Tạo TempWarSet mặc định = 35.0°C");
}


if (!Firebase.RTDB.pathExisted(&fbdo, "/"+uid+"/"+name+"/"+"setting"+"/Timeset")) {
    Firebase.RTDB.setFloat(&fbdo, "/"+uid+"/"+name+"/"+"setting"+"/Timeset", timeSetGateway);
    Serial.println("✅ Tạo timeSetGateway mặc định = 30 phút");}
  // cau hinh stream

if (!Firebase.RTDB.beginStream(&fbdo_s1, "/"+uid+"/"+name+"/setting/threshold"))
  Serial.printf("stream 1 (Nhiet do dat canh bao) begin error,%s\n\n", fbdo_s1.errorReason().c_str());
if (Firebase.RTDB.getFloat(&fbdo, "/"+uid+"/"+name+"/"+"/setting/threshold/temp")) {
   tempSet=fbdo.floatData();}
if (Firebase.RTDB.getFloat(&fbdo, "/"+uid+"/"+name+"/"+"/setting/threshold/hum")) {
   humSet=fbdo.floatData();}
if (Firebase.RTDB.getFloat(&fbdo, "/"+uid+"/"+name+"/"+"/setting/threshold/soil")) {
   soilSet=fbdo.floatData();}
  if (!Firebase.RTDB.beginStream(&fbdo_s2, "/"+uid+"/"+name+"/"+"/setting/Timeset"))
  Serial.printf("stream 2 (Truyen du lieu luu tru len database ) begin error,%s\n\n", fbdo_s2.errorReason().c_str());
if (Firebase.RTDB.getInt(&fbdo,"/"+uid+"/"+name+"/"+"/setting/Timeset")) {
   timeSetGateway=fbdo.intData();
   Serial.printf("settimeGate: %d\n\n", timeSetGateway);
   }
String wifiPath = "/" + uid + "/" + name + "/setting/Wifisetting";// can fix
  if (!Firebase.RTDB.beginStream(&fbdo_s3, wifiPath)) {
    Serial.printf("❌ wifi stream begin error: %s\n", fbdo_s3.errorReason().c_str());
    return;
  }
Serial.println("✅ Started wifi stream: " + wifiPath);
  // wifiStreamStarted = true;
}
void loopWifi(){
  if (checktime==0){
   getTime();
   checktime=1;}
 if ((millis()-prevMillisMain)>5000){
  prevMillisMain=millis(); 
  // Hum= DHT22.runHum();
  // Temp=DHT22.runTemp();
 }
 loopUart();
  handleThresholdStream();// khong nam
  handleWifiStream();// khong nam 
setTimeHis();// khong nam trong uart
 // warDatabase(Hum, Temp);
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
      setWhenisWifi();
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
