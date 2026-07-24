# ESP8266 RGBW LED Controller / 控制器使用說明

[English Version](#english-version) | [中文版](#中文版)

---

## English Version

This project uses a WEMOS D1 Mini Pro (ESP8266) to control RGBW LEDs, supporting dual control via physical buttons and a WiFi Web Interface (AP Mode).

### 1. Hardware Wiring Guide

#### LED Control Pins (with MOSFETs for power amplification)
*   **White (W):** D1 (GPIO5)
*   **Blue (B):** D2 (GPIO4)
*   **Green (G):** D5 (GPIO14)
*   **Red (R):** D6 (GPIO12)

#### Status Indicators
*   **Power Indicator:** D0 (GPIO16) - Constantly ON after boot
*   **Button Status LED 1:** D3 (GPIO0) - Constantly ON when lights are on
*   **Button Status LED 2:** D4 (GPIO2 / Built-in LED) - Breathing light effect when lights are on

#### IP5306 Keep-Alive Connection (UPS functionality)
*   **Keep-Alive Pin:** RX (GPIO3)
    *   *Wiring:* Connect a physical wire from the `RX` pin on the Wemos board to the `KEY` pad on the IP5306 module. The program automatically sends a low-level pulse every 15 seconds to prevent the IP5306 from shutting down under low current loads.

#### Physical Button Wiring (Important!)
*   **Power / Mode Button (But01):** Connected to **D7 (GPIO13)**.
    *   *Wiring:* The physical circuit includes a 10K pull-down resistor to GND. Connect one side of the button to D7 and the other side to **3.3V** (Active High). The program is configured for active-high triggering with internal pull-ups disabled.
*   **Color / Dimming Button (But02):** Connected to **D8 (GPIO15)**.
    *   *Wiring:* Due to ESP8266 boot constraints, D8 must be held low during boot. The circuit includes a **10K pull-down resistor to GND**. Connect one side of the button to D8 and the other side to **3.3V (VCC)** (Active High). The program is configured for active-high triggering with internal pull-ups disabled to avoid voltage conflicts.

---

### 2. Physical Button Operations

#### But01 (D7) - Power & Mode Button
*   **Long Press:** Power ON / OFF
*   **Single Click:** If powered off, clicking will directly power on the lights. If powered on, switches brightness levels and modes (30% -> 60% -> 90% -> Blink Mode)
*   **Double Click (when powered on):** Enter / Exit "Flame Mode"

#### But02 (D8) - Color & Dimming Button
*   **Single Click (when powered on, non-flame mode):** Switch colors (White -> Red -> Green -> Blue)
*   **Long Press (when powered on):** Stepless dimming (release and long press again to reverse dimming direction). In flame mode, long press adjusts the base brightness of the flame.

---

### 3. WiFi Web Control (AP Mode)

After booting, the ESP8266 automatically broadcasts a WiFi signal. You can connect using a smartphone or computer to control the lights.

1. **Connect to WiFi:**
   *   **Network Name (SSID):** `ESP8266_RGBW_LED`
   *   **Password:** `password123`
2. **Open Control Panel:**
   *   Open a web browser on your device.
   *   Enter the URL: `http://192.168.4.1`
3. **Web Interface Operations:**
   *   The web interface provides graphical buttons corresponding directly to the physical buttons.
   *   **One-Click Power On:** Clicking the "Mode/Power On (b1c)" button while the system is off will instantly power on the lights for intuitive operation.
   *   **Dimming:** The web dimming button supports "press and hold" for continuous dimming. Release to stop. The experience is identical to the physical button.

---

### 4. Serial Monitor Commands (115200 bps)

If connected to a computer, you can view status updates via the Arduino IDE Serial Monitor or send the following commands to simulate button presses:
*   `b1l`: But01 Long Press (Power ON/OFF)
*   `b1c`: But01 Single Click (Switch Mode)
*   `b1d`: But01 Double Click (Flame Mode)
*   `b2c`: But02 Single Click (Switch Color)
*   `b2ld`: But02 Long Press Start (Start Dimming)
*   `b2lu`: But02 Long Press Stop (Stop Dimming)

<br><br><br>

---

## 中文版

本專案使用 WEMOS D1 Mini Pro (ESP8266) 來控制 RGBW LED，並支援實體按鈕與 WiFi 網頁（AP 模式）雙重控制。

### 1. 硬體接線說明

#### LED 控制腳位 (搭配 MOSFET 放大功率)
*   **白光 (W):** D1 (GPIO5)
*   **藍光 (B):** D2 (GPIO4)
*   **綠光 (G):** D5 (GPIO14)
*   **紅光 (R):** D6 (GPIO12)

#### 狀態指示燈
*   **電源指示燈:** D0 (GPIO16) - 開機後常亮
*   **按鍵狀態燈 1:** D3 (GPIO0) - 開燈後保持常亮
*   **按鍵狀態燈 2:** D4 (GPIO2 / 內建 LED) - 開燈後呈現呼吸燈效果

#### IP5306 防休眠連線 (不斷電系統)
*   **防休眠喚醒腳:** RX (GPIO3)
    *   *接法：* 從 Wemos 板上的 `RX` 腳位，拉一條實體線連接到 IP5306 模組的 `KEY` 接點。程式會自動每 15 秒送出脈衝，防止 IP5306 在低電流時自動斷電。

#### 實體按鈕接線 (重要！)
*   **開關 / 模式鍵 (But01):** 接在 **D7 (GPIO13)**。
    *   *接法：* 實體電路接有 10K 下拉電阻至 GND。按鈕一端接 D7，另一端建議接 **3.3V**（Active High）。程式已設定為高電位觸發且關閉內部上拉電阻。
*   **顏色 / 調光鍵 (But02):** 接在 **D8 (GPIO15)**。
    *   *接法：* 因 ESP8266 開機限制，D8 必須保持低電位。實體電路已接了 **10K 下拉電阻至 GND**。按鈕一端接 D8，另一端接 **3.3V (VCC)**（Active High）。程式已設定為高電位觸發且關閉內部上拉電阻，以避免電壓衝突。

---

### 2. 實體按鈕操作方式

#### But01 (D7) - 電源與模式鍵
*   **長按：** 開機 / 關機
*   **單擊：** 若為關機狀態，點擊將直接開機亮燈；若為開機狀態，切換亮度等級與模式 (30% -> 60% -> 90% -> 閃爍模式)
*   **雙擊 (開機狀態下)：** 進入 / 退出「火焰燈模式」

#### But02 (D8) - 顏色與調光鍵
*   **單擊 (開機狀態下，非火焰模式)：** 切換顏色 (白 -> 紅 -> 綠 -> 藍)
*   **長按 (開機狀態下)：** 無段調光（放開後再次長按可反向調光）。若在火焰燈模式下，長按可調整火焰的基礎亮度。

---

### 3. WiFi 網頁控制 (AP 模式)

ESP8266 啟動後會自動發射 WiFi 訊號，您可以直接使用手機或電腦連線並進行控制。

1. **連接 WiFi：**
   *   **網路名稱 (SSID):** `ESP8266_RGBW_LED`
   *   **密碼:** `password123`
2. **開啟控制面板：**
   *   打開手機或電腦的網頁瀏覽器。
   *   在網址列輸入：`http://192.168.4.1`
3. **網頁介面操作：**
   *   網頁提供了與實體按鈕完全對應的圖形化介面。
   *   **一鍵開燈：** 點擊網頁上的「模式/開燈 (b1c)」，若尚未開機，系統會直接開機亮燈，操作更直覺。
   *   **調光操作：** 網頁上的調光按鈕支援「按住不放」進行連續調光，放開即停止，操作體驗與實體按鈕一致。

---

### 4. Serial 序列埠監控與指令 (115200 bps)

若您連接電腦，可透過 Arduino IDE 序列埠監控視窗觀看狀態，或輸入以下指令模擬按鈕：
*   `b1l`: But01 長按 (開/關機)
*   `b1c`: But01 單擊 (切換模式)
*   `b1d`: But01 雙擊 (火焰模式)
*   `b2c`: But02 單擊 (切換顏色)
*   `b2ld`: But02 長按開始 (開始調光)
*   `b2lu`: But02 長按結束 (停止調光)
