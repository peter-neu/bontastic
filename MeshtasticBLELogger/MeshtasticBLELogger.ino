#include <Arduino.h>
#include <string>

// Choose ONE BLE stack:
// If you're using NimBLE-Arduino (recommended for ESP32):
#include <NimBLEDevice.h>
// If you use the older ESP32 BLE Arduino, adjust includes accordingly.

// ---------- CONFIGURATION (hardcoded PoC) ----------

// Change this to match your Meshtastic device's BLE name,
// or leave as "Meshtastic" to connect to the first matching device.
static const char *MESHTASTIC_DEVICE_NAME = "🚐_cdb5";
static const uint32_t BLE_PASSKEY = 123456; // Fixed PIN for connection

// These UUIDs are taken from the current Meshtastic BLE API docs
// (MeshBluetoothService). Check docs if they change in the future.
// Service: https://meshtastic.org/docs/development/device/client-api/#bluetooth-meshbluetoothservice
static const char *MESHTASTIC_SERVICE_UUID = "6ba1b218-15a8-461f-9fa8-5dcae273eafd";
// Characteristics:
// FromRadio (read):      2c55e69e-4993-11ed-b878-0242ac120002
// ToRadio   (write):     f75c76d2-129e-4dad-a1dd-7866124401e7
// FromNum   (r/n/w):     ed9da18c-a800-4f66-a670-aa7547e34453 (not used yet in this PoC)
static const char *MESHTASTIC_CHAR_FROM_RADIO_UUID = "2c55e69e-4993-11ed-b878-0242ac120002"; // read
static const char *MESHTASTIC_CHAR_TO_RADIO_UUID = "f75c76d2-129e-4dad-a1dd-7866124401e7";   // write

// ---------- FORWARD DECLARATIONS ----------
void startScan();
void processFromRadioPacket(const uint8_t *data, size_t length);

// ---------- GLOBALS ----------
static bool g_connected = false;
static bool g_doConnect = false;
static const NimBLEAdvertisedDevice *g_advDevice = nullptr;

static NimBLEClient *g_client = nullptr;
static NimBLERemoteService *g_radioService = nullptr;
static NimBLERemoteCharacteristic *g_charToRadio = nullptr;
static NimBLERemoteCharacteristic *g_charFromRadio = nullptr;

// ---------- NANOPB / PROTO INCLUDES ----------
//
// Protobuf definitions are in the src/protobufs/ subdirectory
// Nanopb runtime is in the src/nanopb/ subdirectory
// Note: The 'src' directory is automatically added to the include path by Arduino IDE.
extern "C"
{
#include "src/protobufs/meshtastic.pb.h" // Contains ToRadio, FromRadio, MeshPacket etc.
#include "src/nanopb/pb_decode.h"
#include "src/nanopb/pb_encode.h"
}

// ---------- BLE CALLBACKS ----------

class ClientCallbacks : public NimBLEClientCallbacks
{
  void onConnect(NimBLEClient *pClient) override
  {
    Serial.println("BLE connected");
    g_connected = true;
  }

  void onDisconnect(NimBLEClient *pClient, int reason) override
  {
    Serial.print("BLE disconnected (reason ");
    Serial.print(reason);
    Serial.println(")");
    g_connected = false;
    // Do not clear g_client here to avoid race conditions in connectToMeshtastic
    // g_client = nullptr;
    g_radioService = nullptr;
    g_charToRadio = nullptr;
    g_charFromRadio = nullptr;
    // For PoC, just start scanning again
    startScan();
  }
};

// Notification callback for FromRadio characteristic
void fromRadioNotifyCallback(
    NimBLERemoteCharacteristic *pRemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify)
{
  Serial.print("FromRadio notification, length = ");
  Serial.println(length);

  // Process protobuf message here
  processFromRadioPacket(pData, length);
}

// Scan callback: find our Meshtastic device by name
class ScanCallbacks : public NimBLEScanCallbacks
{
  void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override
  {
    const std::string name = advertisedDevice->getName();
    if (!name.empty())
    {
      Serial.print("Found device: ");
      Serial.println(name.c_str());
    }

    if (name == MESHTASTIC_DEVICE_NAME)
    {
      Serial.println("Found target Meshtastic device. Stopping scan and preparing to connect.");
      NimBLEDevice::getScan()->stop();
      g_advDevice = advertisedDevice;
      g_doConnect = true;
    }
  }
};

// ---------- BASIC PROTOBUF PROCESSING ----------
//
// This is a MINIMAL example that:
// 1. Decodes a FromRadio protobuf message.
// 2. Prints application-level text messages coming from the public channel.
//
// Adjust based on the exact Meshtastic .proto definitions you generate.
//

// Helper: print a MeshPacket (very simplified)
static void printMeshPacket(const meshtastic_MeshPacket &pkt)
{
  // In Meshtastic, text messages are usually in the decoded portnum PAYLOAD
  // For a minimal PoC, we'll just show packet summary and raw bytes.
  Serial.println("MeshPacket:");
  Serial.print("  from: ");
  Serial.println(pkt.from);
  Serial.print("  to:   ");
  Serial.println(pkt.to);
  Serial.print("  channel: ");
  Serial.println(pkt.channel);

  // Look for text payload in pkt.decoded or pkt.payload, depending on proto version.
  // Check actual field names/types from generated header.

  if (pkt.which_payload_variant == meshtastic_MeshPacket_decoded_tag)
  {
    const meshtastic_Data &decoded = pkt.decoded;
    if (decoded.portnum == meshtastic_PortNum_TEXT_MESSAGE_APP)
    {
      // TEXT message
      Serial.print("  [TEXT] ");
      // decoded.payload is bytes; for PoC assume it's UTF‑8.
      for (size_t i = 0; i < decoded.payload.size; ++i)
      {
        Serial.print((char)decoded.payload.bytes[i]);
      }
      Serial.println();
    }
    else
    {
      Serial.println("  Non‑text packet type.");
    }
  }
  else
  {
    Serial.println("  Encrypted or unknown packet type.");
  }
}

// Decode FromRadio -> MeshPacket(s) and print messages
void processFromRadioPacket(const uint8_t *data, size_t length)
{
  pb_istream_t stream = pb_istream_from_buffer(data, length);

  meshtastic_FromRadio fromRadioMsg = meshtastic_FromRadio_init_zero;

  if (!pb_decode(&stream, meshtastic_FromRadio_fields, &fromRadioMsg))
  {
    Serial.print("Failed to decode FromRadio: ");
    Serial.println(PB_GET_ERROR(&stream));
    return;
  }

  // The exact field that contains the MeshPacket depends on the version of the proto:
  // older: fromRadioMsg.packet
  // newer: fromRadioMsg.data, etc.
  // Adjust based on the generated header.
  if (fromRadioMsg.which_payload_variant == meshtastic_FromRadio_packet_tag)
  {
    const meshtastic_MeshPacket &pkt = fromRadioMsg.packet;
    // Filter by channel if needed; public channel is typically index 0.
    if (pkt.channel == 0)
    {
      printMeshPacket(pkt);
    }
    else
    {
      Serial.print("Received packet for non‑public channel: ");
      Serial.println(pkt.channel);
    }
  }
  else
  {
    Serial.println("FromRadio message does not contain a MeshPacket payload (ignoring).");
  }
}

// ---------- BLE CONNECTION LOGIC ----------

bool connectToMeshtastic()
{
  if (!g_advDevice)
  {
    Serial.println("No advertised device to connect to.");
    return false;
  }

  // Clean up old client if it exists
  if (g_client)
  {
    NimBLEDevice::deleteClient(g_client);
    g_client = nullptr;
  }

  Serial.println("Creating BLE client...");
  g_client = NimBLEDevice::createClient();
  g_client->setClientCallbacks(new ClientCallbacks(), false);

  Serial.println("Connecting to device...");
  if (!g_client->connect(g_advDevice))
  {
    Serial.println("Failed to connect.");
    NimBLEDevice::deleteClient(g_client);
    g_client = nullptr;
    return false;
  }

  Serial.println("Connected. Proceeding to discover Meshtastic service...");

  // --- Debug: list all services and look for Meshtastic UUID ---
  Serial.println("Discovering services...");
  const std::vector<NimBLERemoteService *> &services = g_client->getServices(true);
  if (services.empty())
  {
    Serial.println("No services found on device.");
  }
  else
  {
    Serial.print("Found ");
    Serial.print(services.size());
    Serial.println(" services:");
    for (auto *svc : services)
    {
      Serial.print("  Service UUID: ");
      Serial.println(svc->getUUID().toString().c_str());
    }
  }

  Serial.print("Looking up Meshtastic service UUID: ");
  Serial.println(MESHTASTIC_SERVICE_UUID);
  g_radioService = g_client->getService(MESHTASTIC_SERVICE_UUID);
  if (!g_radioService)
  {
    Serial.println("Failed to find Meshtastic service by UUID, but keeping connection open.");
    return false; // report failure but do not disconnect; caller can decide next steps
  }

  Serial.println("Getting ToRadio characteristic (write)...");
  g_charToRadio = g_radioService->getCharacteristic(MESHTASTIC_CHAR_TO_RADIO_UUID);
  if (!g_charToRadio)
  {
    Serial.println("Failed to find ToRadio characteristic.");
    return false;
  }

  Serial.println("Getting FromRadio characteristic (notify)...");
  g_charFromRadio = g_radioService->getCharacteristic(MESHTASTIC_CHAR_FROM_RADIO_UUID);
  if (!g_charFromRadio)
  {
    Serial.println("Failed to find FromRadio characteristic.");
    return false;
  }

  if (g_charFromRadio->canNotify())
  {
    Serial.println("Subscribing to FromRadio notifications...");
    if (!g_charFromRadio->subscribe(true, fromRadioNotifyCallback))
    {
      Serial.println("Failed to subscribe FromRadio.");
      return false;
    }
  }
  else
  {
    Serial.println("FromRadio characteristic does not support notifications.");
  }

  // ---- Minimal "pairing"/handshake (optional PoC) ----
  // Many Meshtastic firmwares will just work when you subscribe and send ToRadio commands.
  // If your device requires a specific pairing message, encode and send here using ToRadio.
  //
  // Example stub (no real content, for PoC only):
  //
  // meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_zero;
  // // Fill toRadio fields as required by your firmware (e.g. request node info)
  // uint8_t buffer[256];
  // pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  // if (pb_encode(&ostream, meshtastic_ToRadio_fields, &toRadio)) {
  //   g_charToRadio->writeValue(buffer, ostream.bytes_written, false);
  // }

  Serial.println("Meshtastic BLE connection configured, waiting for messages...");
  return true;
}

// Start BLE scan to find Meshtastic device
void startScan()
{
  Serial.println("Starting BLE scan...");
  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(new ScanCallbacks(), false);
  scan->setInterval(45);
  scan->setWindow(15);
  scan->setActiveScan(true);
  scan->start(0, false); // 0 = continuous scan
}

// ---------- ARDUINO SETUP/LOOP ----------

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    // Wait for USB serial connection
  }
  Serial.println();
  Serial.println("Meshtastic ESP32 BLE Logger - minimal PoC");

  // Initialize BLE
  NimBLEDevice::init("ESP32-Meshtastic-Logger");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); // highest power

  // Security configuration for fixed PIN
  NimBLEDevice::setSecurityAuth(true, true, true); // bonding, mitm, sc
  NimBLEDevice::setSecurityPasskey(BLE_PASSKEY);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY);

  startScan();
}

void loop()
{
  // Do not block the BLE stack; just handle connect request here.
  if (g_doConnect && !g_connected)
  {
    g_doConnect = false;
    connectToMeshtastic();
  }

  // In PoC, nothing else is needed in loop: notifications will be received via callback.
  delay(100);
}