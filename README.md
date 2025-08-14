# Smartflow ESP32 – מדריך בנייה וצריבה (RedBoard + T‑Relay)

המדריך מסביר איך לקמפל קובץ **BIN** יחיד שעובד גם על **SparkFun ESP32 RedBoard** וגם על **LILYGO T‑Relay**, איך לצרוב אותו, ואיך להגדיר את המכשיר בהפעלה הראשונה.

---

## 1) דרישות מקדימות

- **Arduino IDE** 2.x עם חבילת **ESP32 boards** מותקנת  
  (Boards Manager → חפש “ESP32” מאת Espressif → התקן)
- ספריות:
  - `Adafruit BME280 Library`
  - `Adafruit Unified Sensor`
  - `ArduinoJson`
- קבצי פרויקט:
  - `*.ino` (הקוד שלך)
  - `arduino_secrets.h` (פרטי Wi‑Fi ומפתחות גישה)
  - `smartflow_config.h` (ספים, כיול)
- אופציונלי: PlatformIO (VS Code) — הוראות בהמשך.

---

## 2) בחירת לוח (עובד לשני הסוגים)

ב־**Arduino IDE** בחר:

- **Board:** `ESP32 Dev Module` ✅ (כללי ותואם לשני הלוחות)
- **Flash Size:** `4MB (Default)`
- **Partition Scheme (ל־OTA):**  
  - מומלץ: **`Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)`** או  
  - כל פריסה שכוללת OTA (למשל “Minimal SPIFFS (1.9MB APP/190KB SPIFFS)”)
- **Upload Speed:** `921600` (או פחות אם יש בעיות)
- **CPU Frequency:** `240 MHz`
- **Core Debug Level:** `None`
- **PSRAM:** `Disabled` (אלא אם אתה צריך)

> למה `ESP32 Dev Module`? זה פרופיל כללי שעובד עם OTA ומאפשר להגדיר פינים בקוד לפי `deviceType` שנשמר ב־flash.

---

## 3) בניית BIN (Arduino IDE)

1. פתח את הפרויקט ב־Arduino IDE.  
2. הגדר לפי סעיף 2.  
3. בתפריט `Sketch` → **Export Compiled Binary**.  
4. הקובץ יישמר בתיקיית הפרויקט או ליד קובץ ה־INO.

---

## 4) הפעלה ראשונה – הגדרה ב־Serial

חבר טרמינל (Serial Monitor, 9600 baud):

1. **שם מכשיר (פעם אחת):**  
   הקלד: `changeName` → הכנס מספר בין 1–99  
   נשמר כ־`Smartflow_Wifi_XX`.

2. **סוג לוח (פעם אחת):**  
   הקלד: `changeType` → הכנס `T-Relay` או `RedBoard`  
   נשמר ב־flash ומשנה את מיפוי הפינים.

> ברירת מחדל: **T‑Relay**.

---

## 5) OTA – עדכונים מרחוק

הקוד בודק כל דקה (`otaCheckInterval = 60000`) את:

- **version.json** לדוגמה:  
  ```json
  { "version": "4.3", "url": "https://raw.githubusercontent.com/<user>/<repo>/main/firmware_v4.3.bin" }
  ```
- אם `version` שונה — מוריד את ה־BIN ומעדכן.  
- אתחול אוטומטי אחרי עדכון.

---

## 6) מצב Debug (ב־Serial)

- הקלד: `debug`  
- בחר משתנה להצגה (`version`, `temperature`, `humidity`, ועוד)  
- הפסק: `stop`.

---

## 7) תקשורת API

הקוד שולח:
- **POST** ל־`/postData/liveSensorData/` עם כל הנתונים כולל `valvePercent`.
- **PUT** ל־`/putData/liveStatusSensorData` לעדכון קיים.
- **GET** ל־`/getData/liveStatusSensorData` לקבלת פרמטרים דינמיים.

---

## 8) מיפוי פינים

### T‑Relay (ברירת מחדל)
- `sensorPin = 36`
- `IsPowerOnPin = 39`
- `WaterValvePin = 21`
- `PowerControlPin = 19`
- I²C: `SDA=23`, `SCL=22`

### RedBoard
- `sensorPin = A0`
- `IsPowerOnPin = A3`
- `WaterValvePin = 25`
- `PowerControlPin = 14`
- I²C: `SDA=21`, `SCL=22`

---

## 9) PlatformIO (אופציונלי)

`platformio.ini` בסיסי:
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 9600
build_flags =
  -DCORE_DEBUG_LEVEL=0
board_build.partitions = default.csv
lib_deps =
  adafruit/Adafruit BME280 Library
  adafruit/Adafruit Unified Sensor
  bblanchon/ArduinoJson
```

---

## 10) פקודות מהירות (Serial)

- `changeName` → שינוי שם המכשיר
- `changeType` → שינוי סוג לוח
- `debug` / `stop` → מצב בדיקה

---
