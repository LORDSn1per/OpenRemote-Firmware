/*
  OpenRemote Dock firmware change log (newest first)

  1.00 - 2026-08-22
    - First bring-up version. Runs on a bare ESP32 dev board with a single
      LED on a GPIO in place of real IR/RF433 transmit hardware - the goal
      of this version is proving the ESP-NOW wire protocol end to end
      against the remote (OpenRemote_1.0.ino 3.08), not driving real
      output devices, which don't exist yet.
    - The four packet structs and their magic values
      (EspNowAnnouncePacket/OREN, EspNowCommandHeader/ORCM,
      EspNowRfLearnStartPacket/ORLS, EspNowRfLearnResultHeader/ORLR) are
      copied byte-for-byte from OpenRemote_1.0.ino, not shared via a common
      header, because the two are separate PlatformIO projects with no
      shared include path today. If either side's copy changes, the other
      must be updated by hand - there is no compiler check that would catch
      a drift between them, only a runtime one (the magic simply won't
      match and the packet is silently ignored).
    - Joins the same Wi-Fi network as the remote (WIFI_SSID/WIFI_PASSWORD
      below) rather than running its own AP or a fixed ESP-NOW channel.
      This is deliberate: Espressif's own coexistence guidance rates
      ESP-NOW RX as stable only in STA mode, and joining the same AP is
      what keeps this dock on the same channel as the remote automatically,
      including if the router changes it later - see the ESP-NOW channel
      discussion this firmware's design came out of.
    - On boot, broadcasts EspNowAnnouncePacket every 500ms for
      PAIRING_WINDOW_MS (30s) so the remote's "Search for devices" (LCD or
      WebConfig) has something to find. Real dock hardware will likely want
      this gated behind a physical pairing button instead of always firing
      on every boot; not implemented here.
    - EspNowCommandHeader (a command the remote wants sent) is acknowledged
      by blinking the LED: PARSED commands get two short blinks, RAW
      commands bit-bang the actual mark/space timings onto the LED so the
      envelope reaching the dock can be seen and roughly timed against the
      original remote's transmit LED, without any carrier modulation. This
      is a stand-in for a real IR LED or CC1101 transmit call, not a
      literal preview of what either would look like.
    - EspNowRfLearnStartPacket gets an honest reply, not a fake capture:
      after RF_LEARN_REPLY_DELAY_MS this always sends back
      EspNowRfLearnResultHeader{ok:0} - "nothing captured" - because no
      RF433 receiver is installed yet. This exists so the remote's
      /api/rf/learn/status doesn't just hang for the full 15s timeout on
      every attempt, and so WebConfig's RF learn flow can be exercised
      end-to-end (request -> honest failure -> UI shows it) before any
      receiver hardware exists. It must not be replaced with a fabricated
      successful capture for testing - that would be indistinguishable from
      the dock actually working, which it doesn't yet.
    - Not verified on hardware - no dev board has been chosen yet.
*/

#include <WiFi.h>
#include <esp_now.h>

// ---------------------------------------------------------------------------
// Wi-Fi - fill these in to match the remote's network before flashing.
// ---------------------------------------------------------------------------
static const char *WIFI_SSID = "";
static const char *WIFI_PASSWORD = "";

// ---------------------------------------------------------------------------
// LED stand-in for the real IR/RF433 transmitter.
// ---------------------------------------------------------------------------
static const int LED_PIN = 2;  // Most ESP32 dev boards break out an LED here -
                                // change to match whatever board turns up.

// ---------------------------------------------------------------------------
// ESP-NOW wire protocol - copied from OpenRemote_1.0.ino (3.08). Keep in
// sync by hand; see the 1.00 changelog entry above.
// ---------------------------------------------------------------------------
static const uint32_t ESPNOW_ANNOUNCE_MAGIC = 0x4F52454EUL;        // "OREN"
static const uint32_t ESPNOW_RF_LEARN_START_MAGIC = 0x4F524C53UL;  // "ORLS"
static const uint32_t ESPNOW_RF_LEARN_RESULT_MAGIC = 0x4F524C52UL; // "ORLR"
static const uint32_t ESPNOW_COMMAND_MAGIC = 0x4F52434DUL;         // "ORCM"

static const uint8_t ESPNOW_TRANSPORT_IR = 0;
static const uint8_t ESPNOW_TRANSPORT_RF433 = 1;

struct __attribute__((packed)) EspNowAnnouncePacket {
  uint32_t magic;
  char name[24];
};

struct __attribute__((packed)) EspNowRfLearnStartPacket {
  uint32_t magic;
  uint32_t timeoutMs;
};

struct __attribute__((packed)) EspNowRfLearnResultHeader {
  uint32_t magic;
  uint8_t ok;
  uint16_t rawCount;
};

struct __attribute__((packed)) EspNowCommandHeader {
  uint32_t magic;
  uint8_t transport;
  uint8_t encoding;  // 0 = PARSED, 1 = RAW
  uint16_t frequencyKhz;
  uint32_t address;
  uint32_t command;
  uint8_t sonyBits;
  char protocol[16];
  uint16_t rawCount;
};

static const char *DOCK_NAME = "OpenRemote Dock (dev board)";
static const uint32_t PAIRING_WINDOW_MS = 30000UL;
static const uint32_t PAIRING_ANNOUNCE_INTERVAL_MS = 500UL;
static const uint32_t RF_LEARN_REPLY_DELAY_MS = 1500UL;

// ---------------------------------------------------------------------------
// Recv callback stays minimal - copy the packet and let loop() do the real
// work, rather than blocking the Wi-Fi/ESP-NOW task with LED timing or a
// send() call from inside the callback itself.
// ---------------------------------------------------------------------------
static const size_t MAX_PENDING_PAYLOAD = 250;
volatile bool pendingPacketReady = false;
uint8_t pendingPacket[MAX_PENDING_PAYLOAD];
size_t pendingPacketLen = 0;
uint8_t pendingSenderMac[6];

void onEspNowDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (!info || !info->src_addr || !data || len <= 0) return;
  if (pendingPacketReady) return;  // Drop rather than overwrite an unhandled packet.
  size_t copyLen = min((size_t)len, MAX_PENDING_PAYLOAD);
  memcpy(pendingPacket, data, copyLen);
  pendingPacketLen = copyLen;
  memcpy(pendingSenderMac, info->src_addr, 6);
  pendingPacketReady = true;
}

void blinkTimes(int count, uint32_t onMs, uint32_t offMs) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(onMs);
    digitalWrite(LED_PIN, LOW);
    if (i < count - 1) delay(offMs);
  }
}

// Bit-bangs the raw mark/space envelope directly onto the LED - no carrier,
// just on/off for each timing in sequence, alternating starting with a mark.
void blinkRawTimings(const uint16_t *timings, uint16_t count) {
  for (uint16_t i = 0; i < count; i++) {
    digitalWrite(LED_PIN, (i % 2 == 0) ? HIGH : LOW);
    delayMicroseconds(timings[i]);
  }
  digitalWrite(LED_PIN, LOW);
}

void handleCommandPacket(const uint8_t *data, size_t len, const uint8_t *senderMac) {
  if (len < sizeof(EspNowCommandHeader)) {
    Serial.println("Dock: command packet too short, ignoring");
    return;
  }
  EspNowCommandHeader header;
  memcpy(&header, data, sizeof(header));
  const char *transportName = header.transport == ESPNOW_TRANSPORT_RF433 ? "RF433" : "IR";

  if (header.encoding == 1) {
    size_t expectedLen = sizeof(header) + (size_t)header.rawCount * sizeof(uint16_t);
    if (len < expectedLen) {
      Serial.println("Dock: RAW command arrived incomplete, ignoring");
      return;
    }
    Serial.printf("Dock: RAW command via %s, %u timing(s) - blinking LED\n",
                  transportName, (unsigned)header.rawCount);
    blinkRawTimings((const uint16_t *)(data + sizeof(header)), header.rawCount);
  } else {
    Serial.printf("Dock: PARSED command via %s, protocol=%s address=0x%lX command=0x%lX\n",
                  transportName, header.protocol, (unsigned long)header.address,
                  (unsigned long)header.command);
    blinkTimes(2, 120, 120);
  }
}

// No real RF433 receiver exists on this dev board yet - always answers
// honestly with ok=0 rather than faking a captured signal. See the 1.00
// changelog entry for why that matters.
void handleRfLearnStartPacket(const uint8_t *data, size_t len, const uint8_t *senderMac) {
  if (len < sizeof(EspNowRfLearnStartPacket)) return;
  Serial.println("Dock: RF433 learn requested - no receiver installed, replying honestly");
  blinkTimes(3, 60, 60);
  delay(RF_LEARN_REPLY_DELAY_MS);

  EspNowRfLearnResultHeader result;
  result.magic = ESPNOW_RF_LEARN_RESULT_MAGIC;
  result.ok = 0;
  result.rawCount = 0;
  esp_err_t sendResult = esp_now_send(senderMac, (const uint8_t *)&result, sizeof(result));
  if (sendResult != ESP_OK) {
    Serial.printf("Dock: could not reply to RF learn request (%d)\n", (int)sendResult);
  }
}

void servicePendingPacket() {
  if (!pendingPacketReady) return;
  uint8_t data[MAX_PENDING_PAYLOAD];
  size_t len = pendingPacketLen;
  uint8_t senderMac[6];
  memcpy(data, pendingPacket, len);
  memcpy(senderMac, pendingSenderMac, 6);
  pendingPacketReady = false;  // Clear before handling so a fresh packet during
                                // the (blocking) blink below isn't dropped forever.

  if (len < sizeof(uint32_t)) return;
  uint32_t magic;
  memcpy(&magic, data, sizeof(magic));

  if (magic == ESPNOW_COMMAND_MAGIC) {
    handleCommandPacket(data, len, senderMac);
  } else if (magic == ESPNOW_RF_LEARN_START_MAGIC) {
    handleRfLearnStartPacket(data, len, senderMac);
  } else {
    Serial.printf("Dock: unrecognised packet, magic=0x%08lX, ignoring\n", (unsigned long)magic);
  }
}

void broadcastAnnounce() {
  EspNowAnnouncePacket packet;
  packet.magic = ESPNOW_ANNOUNCE_MAGIC;
  memset(packet.name, 0, sizeof(packet.name));
  strncpy(packet.name, DOCK_NAME, sizeof(packet.name) - 1);

  uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  if (!esp_now_is_peer_exist(broadcastMac)) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, broadcastMac, 6);
    peer.channel = 0;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
  }
  esp_now_send(broadcastMac, (const uint8_t *)&packet, sizeof(packet));
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nOpenRemote Dock 1.00 (dev board bring-up)");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  if (!WIFI_SSID[0]) {
    Serial.println("Dock: WIFI_SSID is empty - fill it in before flashing. Halting.");
    while (true) blinkTimes(1, 1000, 1000);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Dock: connecting to \"%s\"", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print('.');
  }
  Serial.printf("\nDock: connected, channel %d, IP %s\n", WiFi.channel(),
                WiFi.localIP().toString().c_str());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Dock: esp_now_init failed. Halting.");
    while (true) blinkTimes(2, 200, 200);
  }
  esp_now_register_recv_cb(onEspNowDataRecv);
  Serial.println("Dock: ESP-NOW ready");
}

void loop() {
  static uint32_t bootMs = millis();
  static uint32_t lastAnnounceMs = 0;
  uint32_t now = millis();

  if (now - bootMs < PAIRING_WINDOW_MS) {
    if (now - lastAnnounceMs >= PAIRING_ANNOUNCE_INTERVAL_MS) {
      lastAnnounceMs = now;
      broadcastAnnounce();
    }
  }

  servicePendingPacket();
}
