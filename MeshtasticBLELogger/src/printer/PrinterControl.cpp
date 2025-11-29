#include "PrinterControl.h"
#include <Arduino.h>
#include <NimBLEAdvertising.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <string>
#include "PrintHelpers.h"
#include "Adafruit_Thermal.h"

extern const char *localDeviceName;
extern Adafruit_Thermal printer;

static const char *serviceUuid = "5a1a0001-8f19-4a86-9a9e-7b4f7f9b0002";
static const char *fieldUuids[] = {
    "5a1a0002-8f19-4a86-9a9e-7b4f7f9b0002",
    "5a1a0003-8f19-4a86-9a9e-7b4f7f9b0002",
    "5a1a0004-8f19-4a86-9a9e-7b4f7f9b0002",
    "5a1a0005-8f19-4a86-9a9e-7b4f7f9b0002",
    "5a1a0006-8f19-4a86-9a9e-7b4f7f9b0002",
    "5a1a0007-8f19-4a86-9a9e-7b4f7f9b0002",
    "5a1a0008-8f19-4a86-9a9e-7b4f7f9b0002",
    "5a1a0009-8f19-4a86-9a9e-7b4f7f9b0002",
    "5a1a000a-8f19-4a86-9a9e-7b4f7f9b0002",
    "5a1a000b-8f19-4a86-9a9e-7b4f7f9b0002",
    "5a1a000c-8f19-4a86-9a9e-7b4f7f9b0002"};

enum SettingField : uint8_t
{
    HeatDots,
    HeatTime,
    HeatInterval,
    Density,
    BreakTime,
    LineHeight,
    FontSelect,
    SizeSelect,
    JustifySelect,
    Decorations,
    Feed,
    FieldCount
};

static const uint16_t printerAppearance = 0x03C0;

static NimBLEServer *printerServer;
static NimBLECharacteristic *characteristics[FieldCount];
static const PrinterSettings defaultSettings{11, 120, 40, 10, 2, 30, 0, 0, 0, 0, 0};
static PrinterSettings printerSettings = defaultSettings;
static Preferences printerPrefs;
static bool prefsReady;

static const char *fieldLabel(uint8_t field)
{
    switch (field)
    {
    case HeatDots:
        return "HEAT_DOTS";
    case HeatTime:
        return "HEAT_TIME";
    case HeatInterval:
        return "HEAT_INTERVAL";
    case Density:
        return "DENSITY";
    case BreakTime:
        return "BREAK";
    case LineHeight:
        return "LINE_HEIGHT";
    case FontSelect:
        return "FONT";
    case SizeSelect:
        return "SIZE";
    case JustifySelect:
        return "JUSTIFY";
    case Decorations:
        return "DECOR";
    case Feed:
        return "FEED";
    default:
        return nullptr;
    }
}

static const char *fieldKeys[] = {
    "heatDots",
    "heatTime",
    "heatInterval",
    "density",
    "breakTime",
    "lineHeight",
    "font",
    "size",
    "justify",
    "decorations",
    nullptr};

static uint8_t *fieldSlot(uint8_t field);

static void ensurePrefs()
{
    if (!prefsReady)
    {
        prefsReady = printerPrefs.begin("printer", false);
    }
}

static void loadSettings()
{
    printerSettings = defaultSettings;
    ensurePrefs();
    if (!prefsReady)
    {
        return;
    }
    for (uint8_t i = 0; i < FieldCount; ++i)
    {
        const char *key = fieldKeys[i];
        if (!key)
        {
            continue;
        }
        uint8_t *slot = fieldSlot(i);
        if (!slot)
        {
            continue;
        }
        *slot = printerPrefs.getUChar(key, *slot);
    }
}

static void persistField(uint8_t field)
{
    const char *key = field < FieldCount ? fieldKeys[field] : nullptr;
    if (!key)
    {
        return;
    }
    ensurePrefs();
    if (!prefsReady)
    {
        return;
    }
    uint8_t *slot = fieldSlot(field);
    if (!slot)
    {
        return;
    }
    printerPrefs.putUChar(key, *slot);
}

static uint8_t *fieldSlot(uint8_t field)
{
    switch (field)
    {
    case HeatDots:
        return &printerSettings.heatDots;
    case HeatTime:
        return &printerSettings.heatTime;
    case HeatInterval:
        return &printerSettings.heatInterval;
    case Density:
        return &printerSettings.density;
    case BreakTime:
        return &printerSettings.breakTime;
    case LineHeight:
        return &printerSettings.lineHeight;
    case FontSelect:
        return &printerSettings.font;
    case SizeSelect:
        return &printerSettings.size;
    case JustifySelect:
        return &printerSettings.justify;
    case Decorations:
        return &printerSettings.decorations;
    case Feed:
        return &printerSettings.feedRows;
    default:
        return nullptr;
    }
}

static uint16_t clampField(uint8_t field, int value)
{
    switch (field)
    {
    case HeatDots:
        return constrain(value, 0, 15);
    case HeatTime:
        return constrain(value, 0, 255);
    case HeatInterval:
        return constrain(value, 0, 255);
    case Density:
        return constrain(value, 0, 31);
    case BreakTime:
        return constrain(value, 0, 7);
    case LineHeight:
        return constrain(value, 24, 64);
    case FontSelect:
        return constrain(value, 0, 1);
    case SizeSelect:
        return constrain(value, 0, 2);
    case JustifySelect:
        return constrain(value, 0, 2);
    case Decorations:
        return constrain(value, 0, 15);
    case Feed:
        return constrain(value, 0, 50);
    default:
        return value < 0 ? 0 : value;
    }
}

static NimBLECharacteristic *getCharacteristic(uint8_t field)
{
    if (field >= FieldCount)
    {
        return nullptr;
    }
    return characteristics[field];
}

static void syncField(uint8_t field, bool notify)
{
    NimBLECharacteristic *c = getCharacteristic(field);
    if (!c)
    {
        return;
    }
    uint8_t *slot = fieldSlot(field);
    uint16_t value = slot ? *slot : 0;
    char buffer[8];
    size_t len = snprintf(buffer, sizeof(buffer), "%u", value);
    c->setValue(reinterpret_cast<uint8_t *>(buffer), len);
    if (notify)
    {
        c->notify();
    }
}

static void logField(uint8_t field, uint16_t value)
{
    const char *label = fieldLabel(field);
    if (!label)
    {
        return;
    }
    char text[12];
    snprintf(text, sizeof(text), "%u", value);
    printInfo(label, text);
}

static void applyPrinterConfig()
{
    printer.setHeatConfig(printerSettings.heatDots, printerSettings.heatTime, printerSettings.heatInterval);
    printer.setPrintDensity(printerSettings.density, printerSettings.breakTime);
    printer.setLineHeight(printerSettings.lineHeight);
    printer.setFont(printerSettings.font ? 'B' : 'A');
    printer.setSize(printerSettings.size == 0 ? 'S' : (printerSettings.size == 1 ? 'M' : 'L'));
    printer.justify(printerSettings.justify == 0 ? 'L' : (printerSettings.justify == 1 ? 'C' : 'R'));
    bool bold = printerSettings.decorations & 0x01;
    bool inverse = printerSettings.decorations & 0x02;
    bool strike = printerSettings.decorations & 0x04;
    bool doubleWidth = printerSettings.decorations & 0x08;
    if (bold)
    {
        printer.boldOn();
    }
    else
    {
        printer.boldOff();
    }
    if (inverse)
    {
        printer.inverseOn();
    }
    else
    {
        printer.inverseOff();
    }
    if (strike)
    {
        printer.strikeOn();
    }
    else
    {
        printer.strikeOff();
    }
    if (doubleWidth)
    {
        printer.doubleWidthOn();
    }
    else
    {
        printer.doubleWidthOff();
    }
}

static void applyFeed()
{
    uint8_t rows = printerSettings.feedRows;
    if (!rows)
    {
        return;
    }
    printer.feed(rows);
    printerSettings.feedRows = 0;
    syncField(Feed, true);
}

static void handleWrite(uint8_t field, const std::string &payload)
{
    if (payload.empty())
    {
        return;
    }
    int value = atoi(payload.c_str());
    uint16_t clamped = clampField(field, value);
    if (field == Feed)
    {
        printerSettings.feedRows = static_cast<uint8_t>(clamped);
        logField(field, clamped);
        applyFeed();
        return;
    }
    uint8_t *slot = fieldSlot(field);
    if (!slot)
    {
        return;
    }
    if (*slot == clamped)
    {
        syncField(field, false);
        return;
    }
    *slot = static_cast<uint8_t>(clamped);
    syncField(field, true);
    applyPrinterConfig();
    persistField(field);
    logField(field, clamped);
}

class SettingCallbacks : public NimBLECharacteristicCallbacks
{
public:
    explicit SettingCallbacks(uint8_t f) : field(f) {}

private:
    uint8_t field;

    void onRead(NimBLECharacteristic *, NimBLEConnInfo &) override
    {
        syncField(field, false);
    }

    void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &) override
    {
        handleWrite(field, c->getValue());
    }
};

class PrinterServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *, NimBLEConnInfo &) override {}

    void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int) override
    {
        NimBLEDevice::startAdvertising();
    }
};

static PrinterServerCallbacks serverCallbacks;

void setupPrinterControl()
{
    if (printerServer)
    {
        return;
    }
    printerServer = NimBLEDevice::createServer();
    printerServer->setCallbacks(&serverCallbacks, false);
    printerServer->advertiseOnDisconnect(true);

    NimBLEService *service = printerServer->createService(serviceUuid);
    for (uint8_t i = 0; i < FieldCount; ++i)
    {
        characteristics[i] = service->createCharacteristic(fieldUuids[i], NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
        characteristics[i]->setCallbacks(new SettingCallbacks(i));
    }
    service->start();
    loadSettings();
    applyPrinterConfig();
    for (uint8_t i = 0; i < FieldCount; ++i)
    {
        syncField(i, false);
    }
    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    NimBLEAdvertisementData advData;
    advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
    advData.setAppearance(printerAppearance);
    advData.setName(localDeviceName ? localDeviceName : "Bontastic Printer");
    advData.addServiceUUID(serviceUuid);
    adv->setAdvertisementData(advData);

    NimBLEAdvertisementData scanData;
    scanData.addServiceUUID(serviceUuid);
    adv->setScanResponseData(scanData);

    NimBLEDevice::startAdvertising();
}

void printerControlLoop()
{
}

const PrinterSettings &getPrinterSettings()
{
    return printerSettings;
}
