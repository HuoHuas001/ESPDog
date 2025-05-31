#include <Arduino.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

Servo Tongue;//舌头舵机
Servo Head;//头部舵机
Servo Tail;//尾巴舵机

bool sTongue = false;
bool sHead = false;
bool sTail = false;

void Home()
{
  Tongue.write(0);
  Head.write(90);
  Tail.write(90);
  sTongue = false;
  sHead = false;
  sTail = false;
}

const char *ssid = "我的🐺";//wifi名
const char *password = "66668888";//密码

const byte DNS_PORT = 53;
DNSServer dnsServer;

AsyncWebServer server(80);
IPAddress local_IP(192, 168, 1, 184);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

void delayMs(int ms)
{
  dnsServer.processNextRequest();
  delay(ms);
}

// 嵌入 HTML 页面，按钮发送请求
const char index_html[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>
    <head>
        <meta charset="utf-8">
        <title>三列按钮居中</title>
        <style>
            @import url('https://fonts.googleapis.com/css2?family=Press+Start+2P&display=swap');

            * {
                box-sizing: border-box;
            }

            body {
                margin: 0;
                font-family: sans-serif;
                display: flex;
                justify-content: center;
                align-items: center;
                height: 100vh;
                width: 100vw;
                background: #f0f0f0;
                overflow-y: scroll;
            }

            .row {
                gap: 10px;
                display: flex;
                width: 100%;
                max-width: 500px;
                /* 限制最大宽度，防止按钮太大 */
            }

        

            .btn {
                flex: 1;
                padding: 1em;
                font-size: 1.1em;
                text-align: center;
                user-select: none;
                font-size: 14px;
                color: #fff;
                background-color: #3c8527;
                border: 4px solid #000;
                padding: 16px 24px;
                cursor: pointer;
                box-shadow:
                    inset -4px -4px 0 #2a5e1c,
                    inset 4px 4px 0 #5eb340,
                    4px 4px 0 #000;
                text-transform: uppercase;
                transition: all 0.1s ease-in-out;
            }

            .btn:hover {
                background-color: #4ca338;
            }

            .btn:active {
                transform: translate(2px, 2px);
                box-shadow:
                    inset -2px -2px 0 #2a5e1c,
                    inset 2px 2px 0 #5eb340,
                    2px 2px 0 #000;
            }
        </style>
    </head>
    <body>
        <div style="width: 100%; padding: 10px;display: flex;flex-direction: column;gap: 10px;align-items: center;">
            
            <h1>我的🐺</h1>
            
            <img style="width: 160px;height: 160px;border-radius: 50%;" src="">         
            <div class="row" style="margin-top: 20px;">
                <div class="btn" onclick="send(4,0)">吐舌头</div>
                <div class="btn" onclick="send(5,0)">摇头</div>
                <div class="btn" onclick="send(6,0)">摇尾巴</div>

            </div>
            <div class="row">
                <div class="btn" onclick="send(2,30)">左转头</div>
                <div class="btn" onclick="send(2,90)">恢复头</div>
                <div class="btn" onclick="send(2,150)">右转头</div>
            </div>
            
            <div class="row">
                <div class="btn" onclick="auto()">自动模式</div>
                <div class="btn"  onclick="location.reload()">停止</div>
            </div>
            
            
        </div>

        <script>
            
            function delay(ms) {
                return new Promise(resolve => setTimeout(resolve, ms));
            }

            function send(id, angel) {
                console.log('发送',id,angel)
                fetch(`/servo?id=${id}&&angle=${angel}`);
            }

      send(886, 0); // 初始化舵机位置

            async function tongue() {
                for (var i = 0; i < 8; i++) {
                    send(1, 0);
                    await delay(200);
                    send(1, 180);
                    await delay(200);
                }
                send(1, 0);
            }

            async function mhead() {
                for (var i = 0; i < 6; i++) {
                    send(2, 30);
                    await delay(500);
                    send(2, 150);
                    await delay(500);
                }
                send(2, 90)
            }

            async function tail() {
                for (var i = 0; i < 6; i++) {
                    send(3, 60);
                    await delay(300);
                    send(3, 120);
                    await delay(300);
                }
                send(3, 90)
            }
            
            async function auto(){
                tongue();
                await delay(2000)
                mhead();
                await delay(2000)
                tail()
                await delay(20000)
                auto();
            }
            
            async function walk(){
                send(2,30)
                send(3, 60);
                await delay(500);
                send(2,150)
                send(3, 120);
                await delay(500);
                walk();
            }
            
        </script>
    </body>
</html>

  )rawliteral";

void setup()
{
  Serial.begin(115200);
  Tongue.attach(D6);
  Head.attach(D7);
  Tail.attach(D8);
  Home();

  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  WiFi.config(local_IP, gateway, subnet);
  IPAddress ip = WiFi.softAPIP();
  dnsServer.start(DNS_PORT, "*", ip);

  // 捕捉所有路径返回 HTML
  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send_P(200, "text/html", index_html); });

  // 处理角度请求
  server.on("/servo", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (request->hasParam("id") && request->hasParam("angle")) {
      int id = request->getParam("id")->value().toInt();
      int angle = request->getParam("angle")->value().toInt();
      angle = constrain(angle, 0, 180); // 限制角度范围

      switch (id) {
        case 1: Tongue.write(angle); break;
        case 2: Head.write(angle); break;
        case 3: Tail.write(angle); break;
        case 4: sTongue = !sTongue;  break;
        case 5: sHead = !sHead; break;
        case 6: sTail = !sTail; break;
        case 886: Home();
      }

      Serial.printf("舵机 %d 设置为角度 %d\n", id, angle);
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "参数错误");
    } });

  server.begin();
}

const int centerAngle = 90;  // 中心位置
const int moveRange = 60;    // 摆动范围20度
const int moveTimes = 10;    // 摆动次数
const int durationMs = 1500; // 总时长1秒

void loop()
{
  dnsServer.processNextRequest();

  if (sTongue)
  {
    Tongue.write(0);
  }
  if (sHead)
  {
    Head.write(30);
  }
  if (sTail)
  {
    Tail.write(0);
  }
  delayMs(200);
  if (sTongue)
  {
    Tongue.write(180);
  }
  delayMs(200);
  if (sTongue)
  {
    Tongue.write(0);
  }
  if (sTail)
  {
    Tail.write(180);
  }
  delayMs(200);
  if (sTongue)
  {
    Tongue.write(180);
  }
  delayMs(200);
  if (sHead)
  {
    Head.write(150);
    delayMs(600);
  }

  
}
