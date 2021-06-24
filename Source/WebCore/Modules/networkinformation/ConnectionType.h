#pragma once

namespace WebCore {

enum class ConnectionType : uint8_t {
    Bluetooth,
    Cellular,
    Ethernet,
    Mixed,
    None,
    Other,
    Unknown,
    Wifi,
    Wimax,
};

}
