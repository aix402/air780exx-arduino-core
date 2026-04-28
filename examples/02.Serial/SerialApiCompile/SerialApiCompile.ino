#include <Arduino.h>

class Label : public Printable {
public:
  size_t printTo(Print &print) const override {
    return print.print("Printable");
  }
};

void setup() {
  Serial.begin(921600);
  Serial1.begin(115200, SERIAL_8N1);

  Print &printer = Serial;
  Stream &stream = Serial1;

  String message("String");
  message += '-';
  message += "OK";

  printer.print(F("+ARDUINO: SERIAL_API,"));
  printer.print(message);
  printer.print(",HEX=");
  printer.print(255, HEX);
  printer.print(",BIN=");
  printer.print((unsigned char)3, BIN);
  printer.print(",NEG=");
  printer.print(-42);
  printer.print(",FLOAT=");
  printer.println(3.14159, 2);

  printer.printf("+ARDUINO: PRINTF,%d,%s\r\n", 42, message.c_str());

  Label label;
  printer.println(label);

  stream.setTimeout(10);
  if (stream.available()) {
    char buffer[8];
    (void)stream.readBytes(buffer, sizeof(buffer));
  }
}

void loop() {
  delay(1000);
}
