/*  
    ESP8266 RGBW LED Controller
    - AP Mode Web Interface (192.168.4.1)
    - Hardware: WEMOS D1 Mini Pro ESP8266
    - RGBW 5W Dimming
    - 0926 Original, 1001 Modified, 2026 Updated with Web UI & Bug Fixes
*/

#include <Arduino.h>
#include <OneButton.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ====== WiFi AP 模式設定 ======
const char* ap_ssid = "ESP8266_RGBW_LED";
const char* ap_password = "password123"; // 至少 8 碼

ESP8266WebServer server(80);

// ====== 腳位定義（依 WEMOS D1 mini Pro 實體腳位） ======
#define PIN_IN01 D7   // 開關鍵 (but01) 實體按鈕腳位
#define PIN_IN02 D8   // 調光/切換鍵 (but02) 實體按鈕腳位
#define PIN_ADC0 A0   // 亂數種子輸入腳位

#define PIN_LEDR D6   // 紅色LED
#define PIN_LEDG D5   // 綠色LED
#define PIN_LEDB D2   // 藍色LED
#define PIN_LEDW D1   // 白色LED

#define PIN_LED_POWER D0 // 電源指示燈
#define PIN_LED_1    D3  // 按鍵1指示燈
#define PIN_LED_2    D4  // 按鍵2指示燈 (Active LOW)
#define PIN_IP5306_KEY 3 // 使用 RX (GPIO3) 作為 IP5306 防休眠喚醒腳

// ====== 狀態旗標與變數 ======
volatile bool power_on_flag = false;         // 燈開關主狀態（true:開燈 false:關燈）
volatile int but01_mode_count = 0;           // but01 模式計數：1-白光/2-中亮/3-高亮/4-閃爍/5-火焰
volatile int but02_color_count = 1;          // but02 顏色選擇：1白2紅3綠4藍

volatile bool but01_is_long_pressing = false; // but01 是否長按中
volatile bool but02_is_long_pressing = false; // but02 是否長按中

int current_brightness = 0;      // 目前亮度（0-255）
bool dimming_up = true;          // 調光方向（true:遞增，false:遞減）
unsigned long lastDimmingTick = 0; // 調光上次運作時間

int flame_base_brightness = 25;  // 火焰模式基礎亮度（10%）
bool flame_dimming_up = true;    // 火焰調光方向
unsigned long lastFlameDimTick = 0; // 火焰調光上次運作時間

unsigned long lastBlinkTick = 0; // 閃爍上次切換時間
bool blinkState = false;         // 閃爍狀態（true:亮 false:滅）

unsigned long lastButtonTick = 0;
unsigned long lastLightTick = 0;
unsigned long lastFlameTick = 0;
unsigned long lastIndicatorTick = 0;

int d4_brightness = 0;           // D4 呼吸燈當前亮度
bool d4_breathing_up = true;     // D4 呼吸燈方向
unsigned long lastBreathingTick = 0; // D4 呼吸燈更新時間

// ====== OneButton 按鈕物件 ======
// PIN_IN01(D7): 因實體接 10K 下拉電阻，改為高電位觸發 (activeLow = false)，關閉內部上拉 (pullupActive = false)
OneButton but01(PIN_IN01, false, false); 
// PIN_IN02(D8): 因實體接 10K 下拉電阻，改為高電位觸發 (activeLow = false)，關閉內部上拉 (pullupActive = false)
OneButton but02(PIN_IN02, false, false);

// ====== 按鈕回調函式宣告 ======
void but01_click();
void but01_doubleClick();
void but01_longPressStart();
void but01_longPressStop();
void but02_click();
void but02_doubleClick();
void but02_longPressStart();
void but02_longPressStop();

// ====== 狀態監控輸出 ======
void printStatus() {
  Serial.print("[狀態] Power: "); Serial.print(power_on_flag);
  Serial.print(", Mode: "); Serial.print(but01_mode_count);
  Serial.print(", Color: "); Serial.print(but02_color_count);
  Serial.print(", Brightness: "); Serial.print(current_brightness);
  Serial.print(", DimUp: "); Serial.print(dimming_up);
  Serial.print(", FlameBase: "); Serial.print(flame_base_brightness);
  Serial.print(", FlameDimUp: "); Serial.println(flame_dimming_up);
}

// ====== Web 介面 HTML (儲存於 PROGMEM 以節省 RAM) ======
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-TW">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>RGBW LED 控制面板</title>
  <style>
    :root {
      --bg-color: #121212;
      --panel-bg: rgba(255, 255, 255, 0.1);
      --primary: #bb86fc;
      --text: #e0e0e0;
      --btn-bg: rgba(255, 255, 255, 0.15);
      --btn-active: rgba(255, 255, 255, 0.3);
    }
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background-color: var(--bg-color);
      color: var(--text);
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 20px;
      margin: 0;
      min-height: 100vh;
      box-sizing: border-box;
      user-select: none;
    }
    h1 {
      margin-top: 10px;
      margin-bottom: 30px;
      font-weight: 300;
      letter-spacing: 2px;
      text-align: center;
    }
    .grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 15px;
      width: 100%;
      max-width: 400px;
    }
    .btn {
      background-color: var(--btn-bg);
      border: 1px solid rgba(255,255,255,0.2);
      border-radius: 12px;
      padding: 20px 10px;
      color: var(--text);
      font-size: 16px;
      font-weight: 500;
      cursor: pointer;
      backdrop-filter: blur(10px);
      transition: background-color 0.2s, transform 0.1s;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      gap: 8px;
    }
    .btn:active, .btn.active {
      background-color: var(--btn-active);
      transform: scale(0.96);
    }
    .btn-full {
      grid-column: 1 / -1;
    }
    .icon {
      font-size: 24px;
    }
    .status {
      margin-top: 30px;
      padding: 15px;
      background: var(--panel-bg);
      border-radius: 12px;
      width: 100%;
      max-width: 400px;
      text-align: center;
      font-size: 14px;
      color: #aaa;
    }
  </style>
</head>
<body>
  <h1>RGBW 控制面板</h1>
  
  <div class="grid">
    <button class="btn btn-full" onclick="sendCmd('b1l')">
      <span class="icon">🔌</span>
      <span>電源開/關 (b1l)</span>
    </button>

    <button class="btn" onclick="sendCmd('b1c')">
      <span class="icon">💡</span>
      <span>模式/開燈 (b1c)</span>
    </button>
    
    <button class="btn" onclick="sendCmd('b1d')">
      <span class="icon">🔥</span>
      <span>火焰模式 (b1d)</span>
    </button>

    <button class="btn btn-full" onclick="sendCmd('b2c')">
      <span class="icon">🎨</span>
      <span>切換顏色 (b2c)</span>
    </button>

    <button class="btn btn-full" id="btn-dim">
      <span class="icon">☀️</span>
      <span>按住調光 (b2ld / b2lu)</span>
    </button>
  </div>

  <div class="status" id="statusMsg">連線中...</div>

  <script>
    function sendCmd(cmd) {
      document.getElementById('statusMsg').innerText = "發送: " + cmd;
      fetch('/cmd?action=' + cmd)
        .then(response => response.text())
        .then(text => document.getElementById('statusMsg').innerText = "狀態: " + text)
        .catch(err => document.getElementById('statusMsg').innerText = "錯誤: " + err);
    }

    // 長按調光邏輯處理
    const btnDim = document.getElementById('btn-dim');
    let isPressing = false;

    function startPress(e) {
      e.preventDefault();
      if(isPressing) return;
      isPressing = true;
      btnDim.classList.add('active');
      sendCmd('b2ld'); // 長按開始
    }

    function endPress(e) {
      if(!isPressing) return;
      e.preventDefault();
      isPressing = false;
      btnDim.classList.remove('active');
      sendCmd('b2lu'); // 長按結束
    }

    // 電腦滑鼠事件
    btnDim.addEventListener('mousedown', startPress);
    window.addEventListener('mouseup', endPress);
    
    // 手機觸控事件
    btnDim.addEventListener('touchstart', startPress, {passive: false});
    window.addEventListener('touchend', endPress);
    window.addEventListener('touchcancel', endPress);

    // 取得初始狀態
    sendCmd('status');
  </script>
</body>
</html>
)rawliteral";

// ====== 處理 Web 指令 ======
void executeCommand(String cmd) {
  if (cmd == "b1c") { but01_click(); Serial.println("Web: b1c"); }
  else if (cmd == "b1d") { but01_doubleClick(); Serial.println("Web: b1d"); }
  else if (cmd == "b1l") { but01_longPressStart(); delay(100); but01_longPressStop(); Serial.println("Web: b1l"); }
  else if (cmd == "b1ld") { but01_longPressStart(); Serial.println("Web: b1ld"); }
  else if (cmd == "b1lu") { but01_longPressStop(); Serial.println("Web: b1lu"); }
  else if (cmd == "b2c") { but02_click(); Serial.println("Web: b2c"); }
  else if (cmd == "b2d") { but02_doubleClick(); Serial.println("Web: b2d"); }
  else if (cmd == "b2l") { but02_longPressStart(); delay(100); but02_longPressStop(); Serial.println("Web: b2l"); }
  else if (cmd == "b2ld") { but02_longPressStart(); Serial.println("Web: b2ld"); }
  else if (cmd == "b2lu") { but02_longPressStop(); Serial.println("Web: b2lu"); }
  else if (cmd == "status") { /* Just return status */ }
  else { Serial.println("Web: Unknown command"); }
}

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleCmd() {
  if (server.hasArg("action")) {
    String action = server.arg("action");
    executeCommand(action);
    
    String statusStr = "Power: " + String(power_on_flag ? "ON" : "OFF") + 
                       " | Mode: " + String(but01_mode_count) + 
                       " | Color: " + String(but02_color_count) + 
                       " | Brightness: " + String(current_brightness);
    server.send(200, "text/plain", statusStr);
  } else {
    server.send(400, "text/plain", "Missing action");
  }
}

// ====== Serial 指令模擬按鈕 ======
void handleSerialInput() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    executeCommand(cmd);
    printStatus();
  }
}

// ====== 初始化 ======
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== ESP8266 RGBW LED Controller Setup ===");

  // 設置 PWM 範圍為 0-255 (ESP8266 預設為 0-1023)
  analogWriteRange(255);

  // 設置所有腳位模式
  pinMode(PIN_LEDR, OUTPUT);
  pinMode(PIN_LEDG, OUTPUT);
  pinMode(PIN_LEDB, OUTPUT);
  pinMode(PIN_LEDW, OUTPUT);
  pinMode(PIN_LED_POWER, OUTPUT);
  pinMode(PIN_LED_1, OUTPUT);
  pinMode(PIN_LED_2, OUTPUT);
  pinMode(PIN_IP5306_KEY, OUTPUT);
  digitalWrite(PIN_IP5306_KEY, HIGH); // IP5306 喚醒腳平時保持高電位

  // 全部 LED 關閉
  analogWrite(PIN_LEDR, 0); analogWrite(PIN_LEDG, 0); analogWrite(PIN_LEDB, 0); analogWrite(PIN_LEDW, 0);

  // 電源指示燈常亮
  digitalWrite(PIN_LED_POWER, HIGH);

  // 亂數種子（用 ADC 讀值）
  randomSeed(analogRead(PIN_ADC0));

  // OneButton 長按觸發時間設定（600ms）
  but01.setPressTicks(600);
  but02.setPressTicks(600);

  // 綁定按鈕回調
  but01.attachClick(but01_click);
  but01.attachDoubleClick(but01_doubleClick);
  but01.attachLongPressStart(but01_longPressStart);
  but01.attachLongPressStop(but01_longPressStop);

  but02.attachClick(but02_click);
  but02.attachDoubleClick(but02_doubleClick);
  but02.attachLongPressStart(but02_longPressStart);
  but02.attachLongPressStop(but02_longPressStop);

  // --- WiFi AP 模式設定 ---
  Serial.println("Starting WiFi AP Mode...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP Address: ");
  Serial.println(IP);

  // --- Web Server 路由設定 ---
  server.on("/", handleRoot);
  server.on("/cmd", handleCmd);
  server.begin();
  Serial.println("HTTP Server Started.");

  Serial.println("可用Serial/Web指令：b1c、b1d、b1l、b1ld、b1lu、b2c、b2d、b2l、b2ld、b2lu"); 
}

// ====== 主循環 ======
void loop() {
  unsigned long now = millis();

  // 處理 Web 請求
  server.handleClient();

  // 處理Serial模擬按鈕
  handleSerialInput();

  // 按鈕輪詢（若有外接物理按鈕）
  if (now - lastButtonTick >= 10) {
    but01.tick();
    but02.tick();
    lastButtonTick = now;
  }

  // ====== IP5306 防休眠喚醒機制 ======
  // 每 15 秒送出一個 100ms 的 LOW 脈衝到 KEY 腳位
  static unsigned long lastKeepAliveTick = 0;
  static bool isKeepAlivePulsing = false;
  
  if (!isKeepAlivePulsing && (now - lastKeepAliveTick >= 15000)) {
    digitalWrite(PIN_IP5306_KEY, LOW);
    isKeepAlivePulsing = true;
    lastKeepAliveTick = now;
  }
  if (isKeepAlivePulsing && (now - lastKeepAliveTick >= 100)) {
    digitalWrite(PIN_IP5306_KEY, HIGH);
    isKeepAlivePulsing = false;
    lastKeepAliveTick = now;
  }

  // ====== 指示燈邏輯 ======
  digitalWrite(PIN_LED_POWER, HIGH); // 電源指示燈常亮

  if (power_on_flag) {
    digitalWrite(PIN_LED_1, HIGH); // D3 (But01) 開機後保持亮
    
    // D4 (But02) 呼吸燈效果 (每 15ms 更新一次)
    if (now - lastBreathingTick >= 15) {
      if (d4_breathing_up) {
        d4_brightness += 2;
        if (d4_brightness >= 255) { d4_brightness = 255; d4_breathing_up = false; }
      } else {
        d4_brightness -= 2;
        if (d4_brightness <= 0) { d4_brightness = 0; d4_breathing_up = true; }
      }
      analogWrite(PIN_LED_2, d4_brightness);
      lastBreathingTick = now;
    }
  } else {
    digitalWrite(PIN_LED_1, LOW); // 關機時 D3 熄滅
    analogWrite(PIN_LED_2, 0);    // 關機時 D4 熄滅
  }

  // === 一般亮度/顏色/閃爍 ===
  if (power_on_flag && but01_mode_count != 5) {
    if (now - lastLightTick >= 10) {
      // ---- 調光（but02長按）自動反向邏輯 ----
      if (but02_is_long_pressing && now - lastDimmingTick >= 100) {
        if (dimming_up) {
          current_brightness = min(255, current_brightness + 13);
          if (current_brightness >= 255) {
            current_brightness = 255;
            dimming_up = false; // 到頂自動反向
          }
        } else {
          current_brightness = max(13, current_brightness - 13);
          if (current_brightness <= 13) {
            current_brightness = 13;
            dimming_up = true; // 到底自動反向
          }
        }
        lastDimmingTick = now;
      }

      // ---- 閃爍模式處理 ----
      if (but01_mode_count == 4) {
        if (now - lastBlinkTick >= 200) {
          blinkState = !blinkState;
          lastBlinkTick = now;
        }
        if (blinkState) {
          analogWrite(PIN_LEDR, 0); analogWrite(PIN_LEDG, 0); analogWrite(PIN_LEDB, 0); analogWrite(PIN_LEDW, 0);
        } else {
          switch (but02_color_count) {
            case 1: analogWrite(PIN_LEDW, current_brightness); break;
            case 2: analogWrite(PIN_LEDR, current_brightness); break;
            case 3: analogWrite(PIN_LEDG, current_brightness); break;
            case 4: analogWrite(PIN_LEDB, current_brightness); break;
          }
        }
      } else {
        // ---- 標準RGBW模式 ----
        // 舊版本在這邊每10ms強制重置 current_brightness，已移除以修正調光 Bug
        
        switch (but02_color_count) {
          case 1: // 白光
            analogWrite(PIN_LEDR, 0); analogWrite(PIN_LEDG, 0); analogWrite(PIN_LEDB, 0); analogWrite(PIN_LEDW, current_brightness);
            break;
          case 2: // 紅光
            analogWrite(PIN_LEDR, current_brightness); analogWrite(PIN_LEDG, 0); analogWrite(PIN_LEDB, 0); analogWrite(PIN_LEDW, 0);
            break;
          case 3: // 綠光
            analogWrite(PIN_LEDR, 0); analogWrite(PIN_LEDG, current_brightness); analogWrite(PIN_LEDB, 0); analogWrite(PIN_LEDW, 0);
            break;
          case 4: // 藍光
            analogWrite(PIN_LEDR, 0); analogWrite(PIN_LEDG, 0); analogWrite(PIN_LEDB, current_brightness); analogWrite(PIN_LEDW, 0);
            break;
        }
      }
      lastLightTick = now;
    }
  }

  // === 火焰燈模式 ===
  if (power_on_flag && but01_mode_count == 5) {
    // 獨立火焰長按調光邏輯，使其平滑不卡頓
    if (but02_is_long_pressing && now - lastFlameDimTick >= 100) {
      if (flame_dimming_up) {
        flame_base_brightness = min(255, flame_base_brightness + 13);
        if (flame_base_brightness >= 255) {
          flame_base_brightness = 255;
          flame_dimming_up = false; // 自動反向
        }
      } else {
        flame_base_brightness = max(25, flame_base_brightness - 13); // 不低於10%
        if (flame_base_brightness <= 25) {
          flame_base_brightness = 25;
          flame_dimming_up = true; // 自動反向
        }
      }
      lastFlameDimTick = now;
    }

    // 隨機火焰閃爍
    if (now - lastFlameTick >= random(50, 150)) {
      int max_brightness = min(255, flame_base_brightness);
      int red_brightness = random(int(max_brightness * 0.7), max_brightness);
      int green_brightness = random(int(max_brightness * 0.4), int(max_brightness * 0.8));
      int blue_flash = random(0, 100);
      int red_flash = random(0, 100);

      analogWrite(PIN_LEDR, red_brightness);
      analogWrite(PIN_LEDG, green_brightness);
      analogWrite(PIN_LEDB, 0);
      analogWrite(PIN_LEDW, 0);

      // 隨機閃現藍光（高溫核心）
      if (blue_flash < 10) {
        analogWrite(PIN_LEDB, random(int(max_brightness * 0.4), max_brightness));
      }
      // 隨機閃現紅光（餘燼）
      if (red_flash < 10) {
        analogWrite(PIN_LEDR, random(int(max_brightness * 0.8), max_brightness));
      }
      lastFlameTick = now;
    }
  }

  // 關燈時關閉所有LED
  if (!power_on_flag) {
    analogWrite(PIN_LEDR, 0); analogWrite(PIN_LEDG, 0); analogWrite(PIN_LEDB, 0); analogWrite(PIN_LEDW, 0);
  }
}

// ====== but01 按鈕（開關/模式/火焰）回調 ======
void but01_click() {
  if (!power_on_flag) {
    // 沒開機時，短按直接開機並進入模式 1
    power_on_flag = true;
    but01_mode_count = 1;
    but02_color_count = 1;
    current_brightness = 255 * 0.3;
    dimming_up = true;
    flame_base_brightness = 25;
    flame_dimming_up = true;
    return;
  }
  
  if (but01_mode_count == 5) but01_mode_count = 1;
  else but01_mode_count = (but01_mode_count % 4) + 1;

  // 切換不同亮度等級
  switch (but01_mode_count) {
    case 1: current_brightness = 255 * 0.3; break;
    case 2: current_brightness = 255 * 0.6; break;
    case 3: current_brightness = 255 * 0.9; break;
    case 4: current_brightness = 255 * 0.4; break; // 閃爍模式
  }
  dimming_up = true; // 每次點擊都預設調光方向往上
}

void but01_doubleClick() {
  if (!power_on_flag) return;
  // 雙擊切換火焰模式
  if (but01_mode_count == 5) {
    but01_mode_count = 1; current_brightness = 255 * 0.3;
  } else {
    but01_mode_count = 5;
    flame_base_brightness = 25;
    flame_dimming_up = true;
  }
}

void but01_longPressStart() { but01_is_long_pressing = true; }
void but01_longPressStop() {
  if (but01_is_long_pressing) {
    if (power_on_flag) {
      power_on_flag = false; but01_mode_count = 0; but02_color_count = 1;
    } else {
      power_on_flag = true; but01_mode_count = 1; but02_color_count = 1; current_brightness = 255 * 0.3;
      flame_base_brightness = 25; flame_dimming_up = true;
    }
  }
  but01_is_long_pressing = false;
}

// ====== but02 按鈕（顏色/調光）回調 ======
void but02_click() {
  if (!power_on_flag || but01_mode_count == 5) return;
  but02_color_count = (but02_color_count % 4) + 1;
}

void but02_doubleClick() {}
void but02_longPressStart() {
  but02_is_long_pressing = true;
  if (but01_mode_count == 5) {
    flame_dimming_up = !flame_dimming_up;
  }
}

void but02_longPressStop() { but02_is_long_pressing = false; }
