#pragma once

#include <Arduino.h>

void printTextMessage(const uint8_t *data, size_t size);
void printPosition(double lat, double lon, int32_t alt);
void printNodeInfo(uint32_t num, const char *name);
void printBinaryPayload(const uint8_t *data, size_t size);
void printInfo(const char *label, const char *value);
void printerSetup();

#define TX_PIN 2
#define RX_PIN 1
