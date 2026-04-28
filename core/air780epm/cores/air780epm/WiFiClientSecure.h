#ifndef AIR780EPM_WIFI_CLIENT_SECURE_H
#define AIR780EPM_WIFI_CLIENT_SECURE_H

#include "AIR780EPMTLSClient.h"

// Compatibility alias only. AIR780EPM uses cellular TLS networking, not WiFi.
typedef AIR780EPMTLSClient WiFiClientSecure;

#endif
