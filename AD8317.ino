/*
 AD8317 RF Power Meter with LCD Keypad Shield
 
 ใช้ LCD Keypad Shield (ปุ่มต่อผ่าน A0):
 - RIGHT, UP, DOWN, LEFT, SELECT
 - AD8317 VOUT ต่อที่ A1
 - VCC: 5V
 
 การใช้งาน:
 - หน้าแรก: เลือกความถี่ด้วย UP/DOWN, กด SELECT เพื่อยืนยัน
 - หน้าวัด: แสดง V และ P (กด SELECT เพื่อกลับไปเปลี่ยนความถี่)
 
 *** แก้ไข V_REF ตามที่ผู้ใช้แจ้ง (5.058V) เพื่อความแม่นยำในการวัดแรงดัน
*/

#include <LiquidCrystal.h>

// LCD: RS, EN, D4, D5, D6, D7
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// ===== LCD Keypad Shield Button Configuration =====
int adc_key_val[5] = {30, 150, 360, 535, 760};
int NUM_KEYS = 5;
int key = -1;
int oldKey = -1;

// Key definitions
#define KEY_RIGHT  0
#define KEY_UP     1
#define KEY_DOWN   2
#define KEY_LEFT   3
#define KEY_SELECT 4
#define KEY_NONE   -1

// ===== Frequency Calibration Data =====
struct FrequencyCalibration {
  float freq_GHz;
  float m;
  float b;
};

const int NUM_FREQS = 11;
FrequencyCalibration calData[NUM_FREQS] = {
  {1.0, -45.38166191, 20.28883096},
  {1.2, -45.29367326, 18.89216744},
  {1.4, -45.68448757, 18.38004561},
  {1.6, -46.44526438, 18.51860593},
  {1.8, -46.67740613, 18.02652686},
  {2.0, -46.74204162, 17.37285905},
  {2.2, -45.29367326, 16.97530775},
  {2.4, -47.23687691, 16.66631356},
  {2.6, -47.42169596, 16.49362958},
  {2.8, -46.62330645, 16.16932507},
  {3.0, -46.93142919, 15.04213380}
};

// ===== Global Variables =====
int currentIndex = 0;
int page = 0;  // 0 = เลือกความถี่, 1 = แสดง Power

// Selected calibration values
float selected_m;
float selected_b;
float selected_freq;

// ===== V-REF Calibration Constants (ใช้สำหรับปรับแก้ความคลาดเคลื่อนเพิ่มเติมหาก V_REF ที่วัดได้ยังไม่สมบูรณ์) =====
const float V_CAL_SLOPE = 1.0058; // 1.013 (ค่าเดิมที่กำหนดไว้)
const float V_CAL_INTERCEPT = 0.0; // 0.009 (ค่าเดิมที่กำหนดไว้)

// ===== Arduino ADC Configuration =====
const int V_PIN = A1;      // AD8317 VOUT ต่อที่ A1 (A0 ใช้สำหรับปุ่ม)
const int READS = 300;
// *** แก้ไขค่า V_REF จาก 5.0 เป็น 5.058 ตามที่ผู้ใช้แจ้ง ***
const float V_REF = 5.061; // ค่าแรงดันอ้างอิงที่วัดได้จริง
// *******************************************************

// ===== Functions =====

// อ่านปุ่มจาก LCD Keypad Shield
int get_key(unsigned int input) {
  for (int k = 0; k < NUM_KEYS; k++) {
    if (input < adc_key_val[k])
      return k;
  }
  return KEY_NONE;
}

// แสดงหน้าเลือกความถี่
void showFreqSelection() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select Freq:");
  lcd.setCursor(0, 1);
  lcd.print("> ");
  lcd.print(calData[currentIndex].freq_GHz, 1);
  lcd.print(" GHz");
}

// แสดงหน้าวัด Power
void showPowerMeasurement(float voltage, float power) {
  lcd.clear();
  
  // บรรทัดที่ 1: Frequency และ V
  lcd.setCursor(0, 0);
  lcd.print(selected_freq, 1);
  lcd.print("GHz V:");
  lcd.print(voltage, 3);
  lcd.print("V");
  
  // บรรทัดที่ 2: P
  lcd.setCursor(0, 1);
  lcd.print("P:");
  lcd.print((int)power);  //ทศนิยม floor=ปัดลง ceil=ปัดขึ้น int=ปัดทิ้ง
  lcd.print("dBm");
}

void setup() {
  // Setup LCD
  lcd.begin(16, 2);
  
  // Setup Serial
  Serial.begin(9600);
  Serial.println("=== AD8317 RF Power Meter ===");
  Serial.println("Use UP/DOWN to select frequency");
  Serial.println("Press SELECT to confirm");
  
  // แสดงหน้าเลือกความถี่
  showFreqSelection();
  
  // ตั้งค่าเริ่มต้น
  selected_freq = calData[currentIndex].freq_GHz;
  selected_m = calData[currentIndex].m;
  selected_b = calData[currentIndex].b;
}

void loop() {
  // ===== อ่านปุ่ม =====
  int adc_key_in = analogRead(0);  // อ่านจาก A0 (ปุ่ม)
  key = get_key(adc_key_in);
  
  if (key != oldKey) {
    delay(50);  // debounce
    adc_key_in = analogRead(0);
    key = get_key(adc_key_in);
    oldKey = key;
    
    if (key >= 0) {
      // ===== หน้าเลือกความถี่ =====
      if (page == 0) {
        switch (key) {
          case KEY_UP:
            currentIndex++;
            if (currentIndex >= NUM_FREQS) currentIndex = 0;
            showFreqSelection();
            Serial.print("Selected: ");
            Serial.print(calData[currentIndex].freq_GHz, 1);
            Serial.println(" GHz");
            break;
            
          case KEY_DOWN:
            currentIndex--;
            if (currentIndex < 0) currentIndex = NUM_FREQS - 1;
            showFreqSelection();
            Serial.print("Selected: ");
            Serial.print(calData[currentIndex].freq_GHz, 1);
            Serial.println(" GHz");
            break;
            
          case KEY_SELECT:
            // บันทึกค่าที่เลือก
            selected_freq = calData[currentIndex].freq_GHz;
            selected_m = calData[currentIndex].m;
            selected_b = calData[currentIndex].b;
            
            // เปลี่ยนไปหน้าวัด
            page = 1;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Measuring...");
            lcd.setCursor(0, 1);
            lcd.print(selected_freq, 1);
            lcd.print(" GHz");
            delay(1000);
            
            Serial.print("=== Confirmed: ");
            Serial.print(selected_freq, 1);
            Serial.println(" GHz ===");
            break;
        }
      }
      // ===== หน้าแสดง Power =====
      else if (page == 1) {
        if (key == KEY_SELECT) {
          // กลับไปหน้าเลือกความถี่
          page = 0;
          showFreqSelection();
          Serial.println("=== Back to frequency selection ===");
        }
      }
    }
  }
  
  // ===== ถ้าอยู่หน้าวัด Power =====
  if (page == 1) {
    // 1. อ่านค่า ADC (Averaging)
    long sum = 0;
    for (int i = 0; i < READS; i++) {
      sum += analogRead(V_PIN);  // อ่านจาก A1
      delay(5);
    }
    
    // 2. คำนวณแรงดัน (ใช้ V_REF = 5.058)
    float V_Raw = ((float)sum / READS) * (V_REF / 1023.0);
    float V_Calibrated = (V_CAL_SLOPE * V_Raw) + V_CAL_INTERCEPT;
    
    // 3. คำนวณ Power (dBm)
    float P_dBm_Raw;
    if (V_Calibrated > 0.0001) {
      P_dBm_Raw = (selected_m * V_Calibrated) + selected_b;
    } else {
      P_dBm_Raw = -99.9;
    }
    
    // 4. Clamp ค่า
    float P_dBm_Clamped = P_dBm_Raw;
    if (P_dBm_Clamped > 0.0) {
      P_dBm_Clamped = 0.0;
    } else if (P_dBm_Clamped < -40.0) {
      P_dBm_Clamped = -40.0;
    }
    
    // 5. แสดงผล
    showPowerMeasurement(V_Calibrated, P_dBm_Clamped);
    
    // 6. Serial output
    Serial.print("F:");
    Serial.print(selected_freq, 1);
    Serial.print("GHz\t");
    Serial.print("V:");
    Serial.print(V_Calibrated, 3);
    Serial.print("V\t");
    Serial.print("P:");
    Serial.print(P_dBm_Clamped, 3);
    Serial.println("dBm");
    
    delay(100);  // ลด delay เพื่อให้อัปเดตเร็วขึ้น
  }
}
