# Protobuf Generation Notes

## Source Information

- **Protobufs Repository**: https://github.com/meshtastic/protobufs
- **Commit**: 52fa252f1e01be87ad2f7ab17ceef7882b2a4a93
- **Branch**: master
- **Commit Message**: "Add Thinknode M4 and M6 to the equasion (#815)"

## Generated Files

All `.proto` files from the `meshtastic/` directory were compiled using nanopb.

### Key Proto Files
- `meshtastic/mesh.proto` - Contains `FromRadio`, `ToRadio`, `MeshPacket`, `Data`
- `meshtastic/portnums.proto` - Contains `PortNum` enum including `TEXT_MESSAGE_APP`

### Nanopb Runtime Files
- `pb.h`
- `pb_common.c`, `pb_common.h`
- `pb_decode.c`, `pb_decode.h`
- `pb_encode.c`, `pb_encode.h`

### Generated Protobuf Files
All `.pb.c` and `.pb.h` files from the meshtastic namespace.

## Key Structures and Field Names

### `meshtastic_FromRadio`
Located in `mesh.pb.h`

```c
typedef struct _meshtastic_FromRadio {
    uint32_t id;
    pb_size_t which_payload_variant;
    union {
        meshtastic_MeshPacket packet;
        meshtastic_MyNodeInfo my_info;
        meshtastic_NodeInfo node_info;
        meshtastic_Config config;
        meshtastic_LogRecord log_record;
        uint32_t config_complete_id;
        bool rebooted;
        meshtastic_ModuleConfig moduleConfig;
        meshtastic_Channel channel;
        meshtastic_QueueStatus queueStatus;
        meshtastic_XModem xmodemPacket;
        meshtastic_DeviceMetadata metadata;
        meshtastic_MqttClientProxyMessage mqttClientProxyMessage;
        meshtastic_FileInfo fileInfo;
        meshtastic_ClientNotification clientNotification;
        meshtastic_DeviceUIConfig deviceuiConfig;
    };
} meshtastic_FromRadio;
```

**Usage**: To access the MeshPacket, check if `which_payload_variant == meshtastic_FromRadio_packet_tag`, then access via `payload_variant.packet`.

### `meshtastic_MeshPacket`
Located in `mesh.pb.h`

```c
typedef struct _meshtastic_MeshPacket {
    uint32_t from;
    uint32_t to;
    uint8_t channel;
    pb_size_t which_payload_variant;
    union {
        meshtastic_Data decoded;
        meshtastic_MeshPacket_encrypted_t encrypted;
    };
    uint32_t id;
    uint32_t rx_time;
    float rx_snr;
    uint8_t hop_limit;
    bool want_ack;
    meshtastic_MeshPacket_Priority priority;
    int32_t rx_rssi;
    meshtastic_MeshPacket_Delayed delayed;
    bool via_mqtt;
    uint8_t hop_start;
    meshtastic_MeshPacket_public_key_t public_key;
    bool pki_encrypted;
    uint8_t next_hop;
    uint8_t relay_node;
    uint32_t tx_after;
    meshtastic_MeshPacket_TransportMechanism transport_mechanism;
} meshtastic_MeshPacket;
```

**Usage**: To access decoded data, check if `which_payload_variant == meshtastic_MeshPacket_decoded_tag`, then access via `payload_variant.decoded`.

### `meshtastic_Data`
Located in `mesh.pb.h`

```c
typedef struct _meshtastic_Data {
    meshtastic_PortNum portnum;
    meshtastic_Data_payload_t payload;  // PB_BYTES_ARRAY_T(233)
    bool want_response;
    uint32_t dest;
    uint32_t source;
    uint32_t request_id;
    uint32_t reply_id;
    uint32_t emoji;
    bool has_bitfield;
    uint8_t bitfield;
} meshtastic_Data;
```

**Usage**: The payload field is a byte array with `.size` and `.bytes[]` members.

### `meshtastic_PortNum` enum
Located in `portnums.pb.h`

```c
typedef enum _meshtastic_PortNum {
    meshtastic_PortNum_UNKNOWN_APP = 0,
    meshtastic_PortNum_TEXT_MESSAGE_APP = 1,
    meshtastic_PortNum_REMOTE_HARDWARE_APP = 2,
    meshtastic_PortNum_POSITION_APP = 3,
    meshtastic_PortNum_NODEINFO_APP = 4,
    meshtastic_PortNum_ROUTING_APP = 5,
    meshtastic_PortNum_ADMIN_APP = 6,
    // ... more port numbers
} meshtastic_PortNum;
```

## Arduino Sketch Updates Required

The original sketch expected different field names. Here are the key mappings:

1. **Union variant selector**: Use `which_payload_variant` instead of `has_decoded`
2. **FromRadio packet access**: Use `payload_variant.packet` instead of direct `packet` field
3. **MeshPacket decoded access**: Use `payload_variant.decoded` instead of direct `decoded` field
4. **Check decoded**: Use `which_payload_variant == meshtastic_MeshPacket_decoded_tag` instead of `has_decoded`

## Compilation

These files should compile with the Arduino IDE for ESP32. The nanopb library is designed to be lightweight and work on embedded systems.

All `.pb.c` and `.pb.h` files plus the nanopb runtime files are in the same directory as the `.ino` sketch file. The Arduino IDE will automatically compile all `.c` files.

**Note**: The include paths in the generated files have been fixed to remove the `meshtastic/` subdirectory prefix, since all files are in a flat directory structure.

A wrapper header `meshtastic.pb.h` is provided that includes `mesh.pb.h`, which in turn includes all dependent protobuf headers.
