#include "PrintHelpers.h"
#include "Adafruit_Thermal.h"

Adafruit_Thermal printer(&Serial2);

void printerSetup()
{
    Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
    printer.begin();
    printer.setDefault(); // Restore printer to defaults
    printer.setLineHeight(30);
    printer.setSize('M'); // Set type size to Medium
    printer.justify('C'); // Centered text
    printer.print(F("Meshtastic"));
    printer.feed(2);
    printer.justify('L'); // Left justified text
}

void printTextMessage(const uint8_t *data, size_t size)
{
    Serial.write(data, size);
    Serial.println();
    for (size_t i = 0; i < size; ++i)
    {
        printer.print((char)data[i]);
    }
    printer.println();
}

void printPosition(double lat, double lon, int32_t alt)
{
    Serial.print("POS lat=");
    Serial.print(lat, 7);
    Serial.print(" lon=");
    Serial.print(lon, 7);
    Serial.print(" alt=");
    Serial.println(alt);

    printer.print("POS lat=");
    printer.print(lat, 7);
    printer.print(" lon=");
    printer.print(lon, 7);
    printer.print(" alt=");
    printer.println(alt);
}

void printNodeInfo(uint32_t num, const char *name)
{
    Serial.print("NODE ");
    Serial.print(num);
    Serial.print(" ");
    Serial.println(name);

    printer.print("NODE ");
    printer.print(num);
    printer.print(" ");
    printer.println(name);
}

void printBinaryPayload(const uint8_t *data, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        uint8_t b = data[i];
        if (b < 16)
        {
            Serial.print("0");
        }
        Serial.print(b, HEX);
    }
    Serial.println();
}

void printInfo(const char *label, const char *value)
{
    Serial.print(label);
    Serial.print(": ");
    Serial.println(value);

    printer.print(label);
    printer.print(": ");
    printer.println(value);
}
