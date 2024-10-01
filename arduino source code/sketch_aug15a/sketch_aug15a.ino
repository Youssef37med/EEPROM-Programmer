#define SHIFT_DATA A0
#define SHIFT_CLK A1
#define SHIFT_LATCH 4
#define EEPROM_D0 5
#define EEPROM_D7 12
#define WRITE_EN 13
#define READ_EN A2

typedef enum
{
  AT28C16 = 0,
  AT28C64,
  AT28C256,
  AT28C512,  
}chip_type;

/*
 * Send the chip type.
 */
 void sendChipType(chip_type chip)
 {
  Serial.write(chip);
 }

/*
 * Output the address bits and outputEnable signal using shift registers.
 */
void setAddress(int address, bool outputEnable) {
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, (address >> 8) | (outputEnable ? 0x00 : 0x80));
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, address);

  digitalWrite(SHIFT_LATCH, LOW);
  digitalWrite(SHIFT_LATCH, HIGH);
  digitalWrite(SHIFT_LATCH, LOW);
}


/*
 * Read a byte from the EEPROM at the specified address.
 */
byte readEEPROM(int address) {
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    pinMode(pin, INPUT);
  }
  setAddress(address, /*outputEnable*/ true);
  digitalWrite(READ_EN, LOW);
  
  byte data = 0;
  for (int pin = EEPROM_D7; pin >= EEPROM_D0; pin -= 1) {
    data = (data << 1) + digitalRead(pin);
  }
  digitalWrite(READ_EN, HIGH);
  return data;
}


/*
 * Write a byte to the EEPROM at the specified address.
 */
void writeEEPROM(int address, byte data) {
  digitalWrite(READ_EN, HIGH);
  setAddress(address, /*outputEnable*/ false);
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    pinMode(pin, OUTPUT);
  }

  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    digitalWrite(pin, data & 1);
    data = data >> 1;
  }
  digitalWrite(WRITE_EN, LOW);
  delayMicroseconds(1);
  digitalWrite(WRITE_EN, HIGH);
  delay(10);
}

void eraseEEPROM()
{
  // Erase entire EEPROM
  for (int address = 0; address <= 2047; address += 1) {
    writeEEPROM(address, 0xff);

    if (address % 64 == 0) {
      Serial.print(".");
    }
  }
}

/*
 * Read the contents of the EEPROM and print them to the serial monitor.
 */
void printContents() {
  for (int base = 0; base <= 2047; base += 16) {
    byte data[16];
    for (int offset = 0; offset <= 15; offset += 1) {
      data[offset] = readEEPROM(base + offset);
    }

    char buf[80];
    sprintf(buf, "%03x:  %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

    Serial.println(buf);
  }
}


void setup() {
  // put your setup code here, to run once:
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);
  digitalWrite(WRITE_EN, HIGH);
  pinMode(WRITE_EN, OUTPUT);
  pinMode(READ_EN, OUTPUT);

  digitalWrite(READ_EN, HIGH);
  digitalWrite(WRITE_EN, HIGH);
  
  Serial.begin(57600);

}


void loop() {
  if (Serial.available() > 0) {
    char commandd = Serial.read();
    int address = 0;
    byte data = 0;
    
    switch (commandd) {
      case 'r': // Read EEPROM
        delayMicroseconds(500);
        while (Serial.available() == 2) {
          address = Serial.read() << 8;
          address |= Serial.read();
          data = readEEPROM(address);
          Serial.write(data);
        }
        break;
        
      case 'w': // Write EEPROM
        delayMicroseconds(500);
        while (Serial.available() == 3) {
          address = Serial.read() << 8;
          address |= Serial.read();
          data = Serial.read();
          writeEEPROM(address, data);
          Serial.write(1);  // Acknowledge
        }
        break;
        
      case 'p': // Print EEPROM contents
        printContents();
        break;

      case 'e': // Print EEPROM contents
        eraseEEPROM();
        break;

      case 'v': // Print EEPROM contents
        sendChipType(AT28C16);
        break;
        
        
      default:
        break;
    }
}
}
