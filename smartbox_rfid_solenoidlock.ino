#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Definisikan pin yang digunakan
#define SS_PIN 10
#define RST_PIN 9
#define RELAY_PIN 7
#define BUTTON_PIN 2
#define BUZZER_PIN 8

// Inisialisasi objek library
MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variabel sistem
byte masterCards[10][4]; 
int registeredCount = 0;
bool isLockOpen = false;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  
  // Inisialisasi LCD
  lcd.begin();
  lcd.backlight();
  showLCDMessage("Smart Box RFID", "by Gemini");
  delay(2000); 
  
  // Atur pin mode
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); 
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); 
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50); 
    if (digitalRead(BUTTON_PIN) == LOW) {
      long pressStartTime = millis();
      while (digitalRead(BUTTON_PIN) == LOW) { }
      long pressDuration = millis() - pressStartTime;
      
      if (pressDuration >= 3000) {
        deleteMode();
      } else {
        registerMode();
      }
    }
  }

  standbyMode();
}

void standbyMode() {
  showLCDMessage("Scan Kartu...", "Jumlah: " + String(registeredCount));

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    byte id[4];
    for (int i = 0; i < 4; i++) {
      id[i] = mfrc522.uid.uidByte[i];
    }
    
    if (isIDRegistered(id)) {
      openLock();
      showLCDMessage("Box Terbuka", "");
      beep(2, 100); 
    } else {
      showLCDMessage("ID Tidak Dikenal", "");
      Serial.print("ID Tidak Dikenal: ");
      printID(id);
      beep(1, 200); 
    }
    delay(2000); 
  }
}

void registerMode() {
  showLCDMessage("Mode Daftar", "Scan Kartu...");
  
  while (true) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      delay(50); 
      if (digitalRead(BUTTON_PIN) == LOW) {
        showLCDMessage("Mode Standby", "");
        delay(1000);
        return; 
      }
    }
    
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      if (registeredCount < 10) {
        byte newID[4];
        for (int i = 0; i < 4; i++) {
          newID[i] = mfrc522.uid.uidByte[i];
        }
        
        if (!isIDRegistered(newID)) {
          for (int i = 0; i < 4; i++) {
            masterCards[registeredCount][i] = newID[i];
          }
          registeredCount++;
          showLCDMessage("Kartu Terdaftar!", "Jumlah: " + String(registeredCount));
          Serial.print("Kartu baru terdaftar: ");
          printID(newID);
          delay(2000);
          showLCDMessage("Mode Daftar", "Scan Kartu..."); 
        } else {
          showLCDMessage("ID Sudah Ada!", "");
          Serial.print("ID sudah terdaftar: ");
          printID(newID);
          delay(2000);
          showLCDMessage("Mode Daftar", "Scan Kartu...");
        }
      } else {
        showLCDMessage("Pendaftaran Penuh!", "");
        delay(2000);
      }
    }
  }
}

void deleteMode() {
  showLCDMessage("Mode Hapus", "Scan Kartu...");
  
  while (true) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      delay(50);
      if (digitalRead(BUTTON_PIN) == LOW) {
        showLCDMessage("Mode Standby", "");
        delay(1000);
        return;
      }
    }
    
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      byte idToDelete[4];
      for (int i = 0; i < 4; i++) {
        idToDelete[i] = mfrc522.uid.uidByte[i];
      }
      
      int index = getIDIndex(idToDelete);
      if (index != -1) {
        for (int i = index; i < registeredCount - 1; i++) {
          for (int j = 0; j < 4; j++) {
            masterCards[i][j] = masterCards[i + 1][j];
          }
        }
        registeredCount--;
        showLCDMessage("Kartu Dihapus!", "Jumlah: " + String(registeredCount));
        Serial.print("Kartu dihapus: ");
        printID(idToDelete);
        delay(2000);
        showLCDMessage("Mode Hapus", "Scan Kartu...");
      } else {
        showLCDMessage("ID Tidak Ada", "");
        Serial.print("ID tidak terdaftar: ");
        printID(idToDelete);
        delay(2000);
        showLCDMessage("Mode Hapus", "Scan Kartu...");
      }
    }
  }
}

// Fungsi-fungsi pembantu
bool isIDRegistered(byte *id) {
  for (int i = 0; i < registeredCount; i++) {
    bool match = true;
    for (int j = 0; j < 4; j++) {
      if (id[j] != masterCards[i][j]) {
        match = false;
        break;
      }
    }
    if (match) {
      return true;
    }
  }
  return false;
}

int getIDIndex(byte *id) {
  for (int i = 0; i < registeredCount; i++) {
    bool match = true;
    for (int j = 0; j < 4; j++) {
      if (id[j] != masterCards[i][j]) {
        match = false;
        break;
      }
    }
    if (match) {
      return i;
    }
  }
  return -1;
}

void openLock() {
  digitalWrite(RELAY_PIN, LOW); // Aktifkan relay untuk membuka kunci
  isLockOpen = true; // Set status box terbuka
  Serial.println("Lock Terbuka.");
  delay(1000); // Tunggu 1 detik
  digitalWrite(RELAY_PIN, HIGH); // Matikan relay untuk menutup kunci kembali
  isLockOpen = false;
  Serial.println("Lock Tertutup Otomatis.");
}

void closeLock() {
  // Fungsi ini tidak lagi dibutuhkan, karena openLock() sudah otomatis menutup
}

void beep(int count, int duration) {
  for (int i = 0; i < count; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(duration);
    digitalWrite(BUZZER_PIN, LOW);
    if (i < count - 1) { 
      delay(duration);
    }
  }
}

void showLCDMessage(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void printID(byte *id) {
  for (int i = 0; i < 4; i++) {
    if (id[i] < 0x10) {
      Serial.print("0");
    }
    Serial.print(id[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}
