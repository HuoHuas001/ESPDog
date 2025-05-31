#include "main.h"

// WiFi配置
const char *SSID = "HuoHua-Dog";
const char *PASSWORD = "123456789";
const byte DNS_PORT = 53;
IPAddress LOCAL_IP(192, 168, 1, 184);
IPAddress GATEWAY(192, 168, 1, 1);
IPAddress SUBNET(255, 255, 255, 0);

DNSServer dnsServer;
AsyncWebServer server(80);


// 延迟函数(处理DNS请求)
void delayMs(int ms) {
  dnsServer.processNextRequest();
  delay(ms);
}

//DogServo定义
void DogServo::setup(){
  servo.attach(pin);
  stop();
}

void DogServo::setAngle(int angle) {
  angle = constrain(angle, minAngle, maxAngle);
  servo.write(angle);
}

void DogServo::start() { 
  active = true; 
}

void DogServo::stop(){ 
  active = false;
  setAngle((maxAngle + minAngle) / 2); // 回到中间位置
}

bool DogServo::isActive() const {
  return active;
}


void DogServo::swing(int fromAngle, int toAngle, int duration) {
  if (!active) return;
  
  setAngle(fromAngle);
  delayMs(duration / 2);
  setAngle(toAngle);
  delayMs(duration / 2);
}



// 全局对象
DogServo tongue(D6, 0, 180);   // 舌头舵机
DogServo head(D7, 30, 150);    // 头部舵机
DogServo tail(D8, 0, 180);     // 尾巴舵机

// 初始化所有舵机到默认位置
void homeAll() {
  tongue.stop();
  head.stop();
  tail.stop();
}

// 设置WiFi AP模式
void setupWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID, PASSWORD);
  WiFi.softAPConfig(LOCAL_IP, GATEWAY, SUBNET);
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("AP Mode Started. IP: %s\n", ip.toString().c_str());

  //dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", ip);
}

// 处理舵机控制请求
void handleServoRequest(AsyncWebServerRequest *request) {
  if (!request->hasParam("id") || !request->hasParam("angle")) {
    request->send(400, "text/plain", "参数错误");
    return;
  }

  int id = request->getParam("id")->value().toInt();
  int angle = request->getParam("angle")->value().toInt();

  switch (id) {
    case 1: tongue.setAngle(angle); break;
    case 2: head.setAngle(angle); break;
    case 3: tail.setAngle(angle); break;
    case 4: tongue.isActive() ? tongue.stop() : tongue.start(); break;
    case 5: head.isActive() ? head.stop() : head.start(); break;
    case 6: tail.isActive() ? tail.stop() : tail.start(); break;
    case 886: homeAll(); break;
    default: 
      request->send(400, "text/plain", "无效ID");
      return;
  }

  String action = "";
  String servoName = "";
  bool isAuto = false;

  switch (id) {
    case 1: 
      servoName = "舌头";
      action = angle == 0 ? "收回" : "伸出";
      break;
    case 2: 
      servoName = "头部";
      action = angle == 30 ? "左转" : (angle == 150 ? "右转" : "回正");
      break;
    case 3: 
      servoName = "尾巴"; 
      action = angle == 0 ? "垂下" : "翘起";
      break;
    case 4: 
      servoName = "舌头";
      isAuto = true;
      action = tongue.isActive() ? "开始摆动" : "停止摆动";
      break;
    case 5: 
      servoName = "头部";
      isAuto = true;
      action = head.isActive() ? "开始摇头" : "停止摇头";
      break;
    case 6: 
      servoName = "尾巴";
      isAuto = true;
      action = tail.isActive() ? "开始摇尾" : "停止摇尾";
      break;
    case 886: 
      servoName = "所有舵机";
      action = "归位";
      break;
    default:
      servoName = "未知";
      action = "无效操作";
  }

  if (isAuto) {
    Serial.printf("[自动模式] %s%s\n", servoName.c_str(), action.c_str());
  } else {
    Serial.printf("[手动控制] %s%s至角度 %d\n", servoName.c_str(), action.c_str(), angle);
  }
  request->send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // 初始化舵机
  tongue.setup();
  head.setup();
  tail.setup();
  homeAll();

  // 设置WiFi
  setupWiFi();

  delay(1000);

  // 设置Web服务器
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    //Serial.println("Root request received");
    request->send_P(200, "text/html", index_html);
  });

  // 强制所有其他路径返回主页
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("/");  // 重定向到根路径
  });

  server.on("/servo", HTTP_GET, handleServoRequest);
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  
  // 舌头摆动逻辑
  if (tongue.isActive()) {
    tongue.swing(0, 180, 400);
  }
  
  // 头部摆动逻辑
  if (head.isActive()) {
    head.swing(30, 150, 800);
  }
  
  // 尾巴摆动逻辑
  if (tail.isActive()) {
    tail.swing(0, 180, 400);
  }
}