#include "apple_tv_metadata.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <JPEGDEC.h>
#include <ESPmDNS.h>
#include <NetworkClient.h>
#include <NetworkClientSecure.h>
#include <Preferences.h>
#include <mbedtls/pk.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <new>

#include "openremote_dock_adb_identity.h"
#include "openremote_prime_media_helper.h"

namespace {

constexpr uint16_t ADB_PORT = 5555;
constexpr uint32_t ADB_VERSION = 0x01000001;
constexpr uint32_t ADB_MAX_DATA = 4096;
constexpr uint32_t A_CNXN = 0x4e584e43;
constexpr uint32_t A_AUTH = 0x48545541;
constexpr uint32_t A_OPEN = 0x4e45504f;
constexpr uint32_t A_OKAY = 0x59414b4f;
constexpr uint32_t A_CLSE = 0x45534c43;
constexpr uint32_t A_WRTE = 0x45545257;
constexpr uint32_t A_STLS = 0x534c5453;
constexpr uint32_t ADB_AUTH_TOKEN = 1;
constexpr uint32_t ADB_AUTH_SIGNATURE = 2;
constexpr uint32_t ADB_AUTH_RSAPUBLICKEY = 3;
constexpr uint32_t APPLE_POLL_MS = 10000;
constexpr uint32_t PLEX_POLL_MS = 5000;
constexpr uint32_t PLEX_CAPTURE_RETRY_MS = 5000;
constexpr uint8_t PLEX_STOP_CONFIRMATIONS = 2;
constexpr uint32_t STREMIO_POLL_MS = 8000;
constexpr uint32_t PRIME_POLL_MS = 10000;
constexpr uint32_t MEDIA_SESSION_POLL_MS = 2000;
constexpr char PRIME_HELPER_PATH[] =
    "/data/local/tmp/openremote-prime-media-v1.jar";
constexpr size_t PRIME_HELPER_BYTES = 1771;

struct __attribute__((packed)) AdbHeader {
  uint32_t command;
  uint32_t arg0;
  uint32_t arg1;
  uint32_t length;
  uint32_t checksum;
  uint32_t magic;
};

mbedtls_pk_context adbPrivateKey;
bool adbKeyLoaded = false;
IPAddress cachedTlsHost;
uint16_t cachedTlsPort = 0;

int adbRandom(void *, unsigned char *bytes, size_t length) {
  esp_fill_random(bytes, length);
  return 0;
}

bool readExact(NetworkClient &client, uint8_t *bytes, size_t length,
               uint32_t timeoutMs) {
  uint32_t deadline = millis() + timeoutMs;
  uint32_t disconnectedAt = 0;
  size_t offset = 0;
  while (offset < length && (int32_t)(millis() - deadline) < 0) {
    int available = client.available();
    if (available > 0) {
      int got = client.read(bytes + offset, min((size_t)available, length - offset));
      if (got > 0) {
        offset += (size_t)got;
        disconnectedAt = 0;
      }
    } else if (!client.connected()) {
      // Android commonly queues the final WRTE and TCP FIN together. Arduino's
      // connected() can therefore turn false a few scheduler ticks before the
      // last bytes become visible through available(). Give that final packet
      // a short drain window instead of treating the FIN as an empty result.
      if (!disconnectedAt) disconnectedAt = millis();
      if (millis() - disconnectedAt >= 300) break;
      delay(1);
    } else {
      disconnectedAt = 0;
      delay(1);
    }
  }
  return offset == length;
}

bool adbSend(NetworkClient &client, uint32_t command, uint32_t arg0,
             uint32_t arg1, const uint8_t *payload = nullptr,
             uint32_t length = 0) {
  AdbHeader header = {};
  header.command = command;
  header.arg0 = arg0;
  header.arg1 = arg1;
  header.length = length;
  header.magic = command ^ 0xffffffffUL;
  for (uint32_t i = 0; i < length; i++) header.checksum += payload[i];
  if (client.write((const uint8_t *)&header, sizeof(header)) != sizeof(header)) return false;
  return !length || client.write(payload, length) == length;
}

bool adbReceive(NetworkClient &client, AdbHeader &header, uint8_t *payload,
                size_t capacity, uint32_t timeoutMs) {
  if (!readExact(client, (uint8_t *)&header, sizeof(header), timeoutMs)) return false;
  if ((header.command ^ 0xffffffffUL) != header.magic || header.length > capacity) return false;
  if (header.length && !readExact(client, payload, header.length, timeoutMs)) return false;
  uint32_t checksum = 0;
  for (uint32_t i = 0; i < header.length; i++) checksum += payload[i];
  // ADB 1.0.41 (0x01000001) permits both sides to omit the legacy payload
  // checksum. Current Google TV builds therefore send zero even for AUTH and
  // WRTE packets that carry data. Older devices still send the byte sum, so
  // accept either representation while retaining the header magic/length
  // checks above.
  return header.checksum == 0 || checksum == header.checksum;
}

bool loadAdbKey() {
  if (adbKeyLoaded) return true;
  mbedtls_pk_init(&adbPrivateKey);
  const unsigned char *pem = (const unsigned char *)OPENREMOTE_DOCK_ADB_PRIVATE_KEY;
  int rc = mbedtls_pk_parse_key(&adbPrivateKey, pem, strlen((const char *)pem) + 1,
                                nullptr, 0, adbRandom, nullptr);
  adbKeyLoaded = rc == 0;
  if (!adbKeyLoaded) Serial.printf("Apple TV: could not load dock ADB identity (%d)\n", rc);
  return adbKeyLoaded;
}

bool signAdbToken(const uint8_t *token, size_t tokenLength, uint8_t *signature,
                  size_t &signatureLength) {
  if (!loadAdbKey()) return false;
  return mbedtls_pk_sign(&adbPrivateKey, MBEDTLS_MD_SHA1, token, tokenLength,
                         signature, 256, &signatureLength,
                         adbRandom, nullptr) == 0;
}

bool adbConnectLegacy(NetworkClient &client, const IPAddress &host) {
  client.setTimeout(5000);
  if (!client.connect(host, ADB_PORT)) {
    Serial.printf("Apple TV: ADB %s:%u did not accept TCP\n",
                  host.toString().c_str(), (unsigned)ADB_PORT);
    return false;
  }
  static const char banner[] = "host::features=shell_v2,cmd";
  if (!adbSend(client, A_CNXN, ADB_VERSION, ADB_MAX_DATA,
               (const uint8_t *)banner, sizeof(banner))) return false;

  uint8_t payload[ADB_MAX_DATA];
  AdbHeader header;
  if (!adbReceive(client, header, payload, sizeof(payload), 5000)) {
    Serial.println("Apple TV: ADB did not answer CNXN");
    return false;
  }
  if (header.command == A_CNXN) return true;
  if (header.command != A_AUTH || header.arg0 != ADB_AUTH_TOKEN) {
    Serial.printf("Apple TV: ADB answered CNXN with 0x%08lX/%lu\n",
                  (unsigned long)header.command, (unsigned long)header.arg0);
    return false;
  }

  uint8_t signature[256];
  size_t signatureLength = 0;
  if (!signAdbToken(payload, header.length, signature, signatureLength)) {
    Serial.println("Apple TV: could not sign the ADB challenge");
    return false;
  }
  if (!adbSend(client, A_AUTH, ADB_AUTH_SIGNATURE, 0, signature,
               (uint32_t)signatureLength)) {
    Serial.println("Apple TV: could not send the ADB signature");
    return false;
  }
  if (!adbReceive(client, header, payload, sizeof(payload), 5000)) {
    Serial.println("Apple TV: no answer after the ADB signature");
    return false;
  }
  if (header.command == A_CNXN) return true;

  // A new dock identity reaches this once. Android shows its approval dialog;
  // the accepted key is retained by Android and this branch is skipped after
  // every later boot or power outage.
  if (header.command != A_AUTH || header.arg0 != ADB_AUTH_TOKEN) return false;
  Serial.println("Apple TV: dock key is not approved; accept the debugging prompt on the TV");
  size_t publicLength = strlen(OPENREMOTE_DOCK_ADB_PUBLIC_KEY) + 1;
  if (!adbSend(client, A_AUTH, ADB_AUTH_RSAPUBLICKEY, 0,
               (const uint8_t *)OPENREMOTE_DOCK_ADB_PUBLIC_KEY,
               (uint32_t)publicLength)) return false;
  if (!adbReceive(client, header, payload, sizeof(payload), 30000)) {
    Serial.println("Apple TV: debugging approval was not received");
    return false;
  }
  return header.command == A_CNXN;
}

bool discoverAdbTlsPort(const IPAddress &host, uint16_t &port) {
  int found = MDNS.queryService("adb-tls-connect", "tcp");
  for (int i = 0; i < found; i++) {
    if (MDNS.address(i) == host && MDNS.port(i)) {
      port = MDNS.port(i);
      return true;
    }
  }
  return false;
}

bool adbConnectTls(NetworkClientSecure &client, const IPAddress &host,
                   uint16_t port) {
  client.setTimeout(5000);
  client.setHandshakeTimeout(10);
  client.setInsecure();
  client.setCertificate(OPENREMOTE_DOCK_ADB_CERTIFICATE);
  client.setPrivateKey(OPENREMOTE_DOCK_ADB_PRIVATE_KEY);
  client.setPlainStart();
  if (!client.connect(host, port)) {
    Serial.printf("Apple TV: ADB TLS %s:%u did not accept TCP\n",
                  host.toString().c_str(), (unsigned)port);
    return false;
  }

  static const char banner[] = "host::features=shell_v2,cmd";
  if (!adbSend(client, A_CNXN, ADB_VERSION, ADB_MAX_DATA,
               (const uint8_t *)banner, sizeof(banner))) return false;

  uint8_t payload[ADB_MAX_DATA];
  AdbHeader header;
  if (!adbReceive(client, header, payload, sizeof(payload), 5000) ||
      header.command != A_STLS) {
    Serial.println("Apple TV: ADB TLS did not offer STLS");
    return false;
  }
  if (!adbSend(client, A_STLS, header.arg0, 0)) return false;
  if (!client.startTLS()) {
    Serial.println("Apple TV: ADB TLS handshake rejected the dock identity");
    return false;
  }
  if (!adbReceive(client, header, payload, sizeof(payload), 5000)) {
    Serial.println("Apple TV: ADB TLS did not finish CNXN");
    return false;
  }
  return header.command == A_CNXN;
}

bool readLunaContentId(NetworkClient &client, char *contentId,
                       size_t contentIdSize, char *kind, size_t kindSize) {
  // Ask Android to do the cheap filtering. Luna's full buffer includes video
  // decoder diagnostics and can be tens of kilobytes; the only lines useful
  // here contain mediaIdentifier=. Keeping the response small also prevents a
  // long logcat dump from outliving the twelve-second ADB shell deadline.
  static const char command[] =
    "shell:logcat -d -v raw -s Luna:I | grep 'mediaIdentifier=' | tail -n 8";
  const uint32_t localId = 1;
  if (!adbSend(client, A_OPEN, localId, 0, (const uint8_t *)command,
               sizeof(command))) return false;

  uint8_t payload[ADB_MAX_DATA];
  AdbHeader header;
  uint32_t remoteId = 0;
  String line;
  line.reserve(768);
  bool found = false;
  uint32_t deadline = millis() + 12000;
  uint32_t bytesRead = 0;
  uint16_t linesRead = 0;
  uint16_t candidates = 0;
  // ADB closes a one-shot shell as soon as it has queued CLSE. Some Arduino
  // clients report connected()==false while decrypted WRTE bytes are still
  // buffered, so drain available data as well.
  while ((int32_t)(millis() - deadline) < 0) {
    // UIAutomator can spend four to five seconds building ABC iview's
    // accessibility tree without emitting a WRTE packet. Keep the per-packet
    // wait inside the existing twelve-second command deadline so that quiet,
    // successful shell commands are not mistaken for empty ADB responses.
    if (!adbReceive(client, header, payload, sizeof(payload), 6000)) break;
    if (header.command == A_OKAY) {
      remoteId = header.arg0;
      continue;
    }
    if (header.command == A_CLSE) {
      adbSend(client, A_CLSE, localId, remoteId);
      break;
    }
    if (header.command != A_WRTE) continue;
    bytesRead += header.length;
    remoteId = header.arg0;
    for (uint32_t i = 0; i < header.length; i++) {
      char c = (char)payload[i];
      if (c == '\n') {
        linesRead++;
        int marker = line.indexOf("mediaIdentifier=");
        bool episode = line.indexOf("e%3Depisode") >= 0;
        bool movie = line.indexOf("e%3Dmovie") >= 0;
        if (marker >= 0 && (episode || movie)) {
          candidates++;
          int begin = line.indexOf("cid%3D", marker);
          if (begin >= 0) {
            begin += 6;
            int end = line.indexOf("%3A", begin);
            if (end < 0) end = line.length();
            String value = line.substring(begin, end);
            if (value.startsWith("umc.cmc.")) {
              strlcpy(contentId, value.c_str(), contentIdSize);
              strlcpy(kind, episode ? "episode" : "movie", kindSize);
              found = true;
            }
          }
        }
        line = "";
      } else if (line.length() < 1000) {
        line += c;
      }
    }
    adbSend(client, A_OKAY, localId, remoteId);
  }
  if (!found) {
    Serial.printf("Apple TV: Luna returned %lu byte(s), %u line(s), %u candidate(s)\n",
                  (unsigned long)bytesRead, (unsigned)linesRead,
                  (unsigned)candidates);
  }
  return found;
}

bool readShellOutput(NetworkClient &client, const char *command, String &output,
                     size_t maximumBytes) {
  const uint32_t localId = 1;
  if (!adbSend(client, A_OPEN, localId, 0, (const uint8_t *)command,
               (uint32_t)strlen(command) + 1)) return false;

  uint8_t payload[ADB_MAX_DATA];
  AdbHeader header;
  uint32_t remoteId = 0;
  uint32_t deadline = millis() + 12000;
  output = "";
  output.reserve(min(maximumBytes, (size_t)2048));
  while ((int32_t)(millis() - deadline) < 0) {
    if (!adbReceive(client, header, payload, sizeof(payload), 3000)) break;
    if (header.command == A_OKAY) {
      remoteId = header.arg0;
      continue;
    }
    if (header.command == A_CLSE) {
      adbSend(client, A_CLSE, localId, remoteId);
      break;
    }
    if (header.command != A_WRTE) continue;
    remoteId = header.arg0;
    size_t room = output.length() < maximumBytes
                    ? maximumBytes - output.length() : 0;
    size_t take = min(room, (size_t)header.length);
    if (take) output.concat((const char *)payload, take);
    adbSend(client, A_OKAY, localId, remoteId);
  }
  return output.length() > 0;
}

void copyField(const String &value, char *destination, size_t size) {
  strlcpy(destination, value.c_str(), size);
}

String htmlDecode(String value) {
  value.replace("&amp;", "&");
  value.replace("&quot;", "\"");
  value.replace("&#39;", "'");
  value.replace("&apos;", "'");
  // The remote's compact Montserrat faces are ASCII-only. Use a readable
  // separator instead of sending U+00B7, which renders as a square glyph.
  value.replace("&middot;", " - ");
  value.replace("·", " - ");
  return value;
}

String xmlAttribute(const String &tag, const char *name) {
  String marker = String(" ") + name + "=\"";
  int at = tag.indexOf(marker);
  if (at < 0) return "";
  at += marker.length();
  int end = tag.indexOf('"', at);
  if (end < 0) return "";
  return htmlDecode(tag.substring(at, end));
}

String urlEncode(const String &value) {
  static const char hex[] = "0123456789ABCDEF";
  String encoded;
  encoded.reserve(value.length() * 2);
  for (size_t i = 0; i < value.length(); i++) {
    uint8_t c = (uint8_t)value[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' ||
        c == '.' || c == '~') {
      encoded += (char)c;
    } else {
      encoded += '%';
      encoded += hex[c >> 4];
      encoded += hex[c & 15];
    }
  }
  return encoded;
}

String urlDecode(const String &value) {
  String decoded;
  decoded.reserve(value.length());
  for (size_t i = 0; i < value.length(); i++) {
    if (value[i] == '%' && i + 2 < value.length()) {
      auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
      };
      int high = nibble(value[i + 1]);
      int low = nibble(value[i + 2]);
      if (high >= 0 && low >= 0) {
        decoded += (char)((high << 4) | low);
        i += 2;
        continue;
      }
    }
    decoded += value[i] == '+' ? ' ' : value[i];
  }
  return decoded;
}

String tagContent(const String &tag) {
  int at = tag.indexOf("content=\"");
  if (at < 0) return "";
  at += 9;
  int end = tag.indexOf('"', at);
  if (end < 0) return "";
  return htmlDecode(tag.substring(at, end));
}

void compactArtworkUrl(String &url) {
  int slash = url.lastIndexOf('/');
  // Apple's `bb` rendition preserves the 16:9 image and returns only 96x54,
  // leaving black bars when centered in the square widget. `cc` returns a
  // real 96x96 centre crop, ready for direct RGB565 caching.
  // A quality suffix keeps detailed film posters small enough for the C3 to
  // download and decode alongside its 18 KB RGB565 transfer buffer. At 96 px,
  // Apple's quality 60 rendition is visually indistinguishable on the LCD but
  // avoids transient heap pressure from unusually detailed artwork.
  if (slash > 8) url = url.substring(0, slash + 1) + "96x96cc-60.jpg";
}

uint16_t *jpegTarget = nullptr;
uint16_t jpegTargetWidth = 0;
uint16_t jpegTargetHeight = 0;

int drawJpegBlock(JPEGDRAW *block) {
  if (!jpegTarget || !block || !block->pPixels) return 0;
  int sourceX = 0;
  int destinationX = block->x;
  int copyWidth = block->iWidth;
  if (destinationX < 0) {
    sourceX = -destinationX;
    copyWidth -= sourceX;
    destinationX = 0;
  }
  copyWidth = min(copyWidth, (int)jpegTargetWidth - destinationX);
  if (copyWidth <= 0) return 1;
  for (int sourceY = 0; sourceY < block->iHeight; sourceY++) {
    int destinationY = block->y + sourceY;
    if (destinationY < 0 || destinationY >= jpegTargetHeight) continue;
    memcpy(jpegTarget + destinationY * jpegTargetWidth + destinationX,
           block->pPixels + sourceY * block->iWidth + sourceX,
           (size_t)copyWidth * 2);
  }
  return 1;
}

}  // namespace

AppleTvMetadataClient appleTvMetadataClient;

void AppleTvMetadataClient::begin() {
  loadAdbKey();
  Preferences stored;
  stored.begin("plexmeta", true);
  String server = stored.getString("server", "");
  String token = stored.getString("token", "");
  stored.end();
  strlcpy(plexServer_, server.c_str(), sizeof(plexServer_));
  strlcpy(plexToken_, token.c_str(), sizeof(plexToken_));
  if (plexServer_[0] && plexToken_[0]) {
    Serial.printf("Plex: restored server connection %s\n", plexServer_);
  }
}

bool AppleTvMetadataClient::runAdbShell(const IPAddress &googleTv,
                                        const char *command, String &output,
                                        size_t maximumBytes) {
  NetworkClient legacy;
  if (adbConnectLegacy(legacy, googleTv)) {
    bool ok = readShellOutput(legacy, command, output, maximumBytes);
    legacy.stop();
    return ok;
  }
  legacy.stop();

  uint16_t tlsPort = 0;
  bool haveCachedPort = cachedTlsPort && cachedTlsHost == googleTv;
  if (haveCachedPort || discoverAdbTlsPort(googleTv, tlsPort)) {
    if (haveCachedPort) tlsPort = cachedTlsPort;
    NetworkClientSecure secure;
    if (adbConnectTls(secure, googleTv, tlsPort)) {
      cachedTlsHost = googleTv;
      cachedTlsPort = tlsPort;
      bool ok = readShellOutput(secure, command, output, maximumBytes);
      secure.stop();
      return ok;
    }
    secure.stop();
    cachedTlsPort = 0;
  }
  return false;
}

bool AppleTvMetadataClient::pollAndroidMediaSession(
    const IPAddress &googleTv, AndroidMediaSession &session, bool force) {
  uint32_t now = millis();
  if (!force && nextMediaSessionPollMs_ &&
      (int32_t)(now - nextMediaSessionPollMs_) < 0) {
    session = cachedMediaSession_;
    return cachedMediaSession_.valid;
  }
  nextMediaSessionPollMs_ = now + MEDIA_SESSION_POLL_MS;

  // The first session block belongs to Android's current media-button owner.
  // Limit the response at the TV so an ESP32-C3 never has to receive the full
  // dumpsys output, which can contain many abandoned app sessions.
  static const char command[] =
    "shell:cat /proc/uptime; "
    "dumpsys activity activities 2>/dev/null | grep -m1 topResumedActivity; "
    "dumpsys media_session | sed -n "
    "'/Media button session is /,/metadata:/p' | head -n 24";
  String output;
  if (!runAdbShell(googleTv, command, output, 3072)) {
    session = {};
    return false;
  }

  // PlaybackState.position is a base value, not a continuously ticking
  // counter. Its `updated` field uses Android's elapsed-realtime clock, so
  // capture /proc/uptime in the same shell response and advance that base
  // while the session is playing. SmartTube can otherwise return the same
  // forty-second position for many minutes and every remote wake jumps back
  // to it.
  uint64_t androidUptimeMs = 0;
  char *uptimeEnd = nullptr;
  double androidUptimeSeconds = strtod(output.c_str(), &uptimeEnd);
  if (uptimeEnd != output.c_str() && androidUptimeSeconds > 0.0) {
    androidUptimeMs = (uint64_t)(androidUptimeSeconds * 1000.0);
  }

  AndroidMediaSession fresh = {};
  int foregroundAt = output.indexOf("topResumedActivity=");
  if (foregroundAt >= 0) {
    foregroundAt = output.indexOf(" u0 ", foregroundAt);
    if (foregroundAt >= 0) {
      foregroundAt += 4;
      int foregroundEnd = output.indexOf('/', foregroundAt);
      if (foregroundEnd > foregroundAt) {
        String foreground = output.substring(foregroundAt, foregroundEnd);
        foreground.trim();
        copyField(foreground, fresh.foregroundPackageName,
                  sizeof(fresh.foregroundPackageName));
      }
    }
  }
  int ownerAt = output.indexOf("Media button session is ");
  if (ownerAt < 0) {
    fresh.valid = fresh.foregroundPackageName[0] != '\0';
    cachedMediaSession_ = fresh;
    session = fresh;
    return fresh.valid;
  }
  ownerAt += strlen("Media button session is ");
  int ownerEnd = output.length();
  int slashAt = output.indexOf('/', ownerAt);
  int spaceAt = output.indexOf(' ', ownerAt);
  int newlineAt = output.indexOf('\n', ownerAt);
  if (slashAt >= 0 && slashAt < ownerEnd) ownerEnd = slashAt;
  if (spaceAt >= 0 && spaceAt < ownerEnd) ownerEnd = spaceAt;
  if (newlineAt >= 0 && newlineAt < ownerEnd) ownerEnd = newlineAt;
  if (ownerEnd <= ownerAt) {
    cachedMediaSession_ = {};
    session = {};
    return false;
  }
  String packageName = output.substring(ownerAt, ownerEnd);
  packageName.trim();
  if (!packageName.length()) return false;

  fresh.valid = true;
  if (packageName == "null") {
    cachedMediaSession_ = fresh;
    session = fresh;
    return true;
  }
  copyField(packageName, fresh.packageName, sizeof(fresh.packageName));
  int stateAt = output.indexOf("state=PlaybackState {state=");
  if (stateAt >= 0) {
    stateAt += strlen("state=PlaybackState {state=");
    int stateEnd = output.indexOf('(', stateAt);
    String stateName = stateEnd > stateAt
                         ? output.substring(stateAt, stateEnd) : String();
    fresh.playing = stateName == "PLAYING";
    fresh.active = fresh.playing || stateName == "PAUSED" ||
                   stateName == "BUFFERING" || stateName == "CONNECTING" ||
                   stateName == "FAST_FORWARDING" || stateName == "REWINDING";
    int stateLineEnd = output.indexOf('\n', stateEnd);
    if (stateLineEnd < 0) stateLineEnd = output.length();
    int positionAt = output.indexOf("position=", stateEnd);
    if (positionAt >= 0 && positionAt < stateLineEnd) {
      positionAt += strlen("position=");
      uint64_t positionMs = strtoull(output.c_str() + positionAt, nullptr, 10);
      int updatedAt = output.indexOf("updated=", positionAt);
      int speedAt = output.indexOf("speed=", positionAt);
      if (fresh.playing && androidUptimeMs &&
          updatedAt >= 0 && updatedAt < stateLineEnd) {
        updatedAt += strlen("updated=");
        uint64_t updatedMs = strtoull(output.c_str() + updatedAt, nullptr, 10);
        float speed = 1.0f;
        if (speedAt >= 0 && speedAt < stateLineEnd) {
          speedAt += strlen("speed=");
          speed = strtof(output.c_str() + speedAt, nullptr);
        }
        if (updatedMs && androidUptimeMs >= updatedMs && speed > 0.0f) {
          positionMs += (uint64_t)((double)(androidUptimeMs - updatedMs) * speed);
        }
      }
      fresh.positionSeconds = (uint32_t)(positionMs / 1000ULL);
    }
  }
  cachedMediaSession_ = fresh;
  session = fresh;
  return true;
}

bool AppleTvMetadataClient::resolveAbcArtwork(
    const char *title, AbcIviewRichMetadata &metadata) {
  if (!title || !title[0]) return false;
  String slug;
  bool dashPending = false;
  for (const unsigned char *p = (const unsigned char *)title; *p; p++) {
    unsigned char c = *p;
    if (isalnum(c)) {
      if (dashPending && slug.length()) slug += '-';
      slug += (char)tolower(c);
      dashPending = false;
    } else if (slug.length()) {
      dashPending = true;
    }
  }
  if (!slug.length() || slug.length() >= sizeof(lastAbcIviewSlug_))
    return false;
  if (slug == lastAbcIviewSlug_ && metadata.artworkUrl[0]) return true;

  NetworkClientSecure secure;
  secure.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(10000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  String api = String("https://api.iview.abc.net.au/v3/show/") + slug;
  if (!http.begin(secure, api)) return false;
  http.addHeader("Accept", "application/json");
  http.addHeader("User-Agent", "OpenRemote-Dock");
  int status = http.GET();
  if (status != 200) {
    Serial.printf("ABC iview: catalogue returned HTTP %d\n", status);
    http.end();
    return false;
  }

  // Keep only the small top-level image list. The complete show response also
  // embeds every season and would waste most of the C3's RAM if materialised.
  JsonDocument filter;
  filter["images"][0]["url"] = true;
  filter["images"][0]["type"] = true;
  JsonDocument document;
  DeserializationError error = deserializeJson(
      document, *http.getStreamPtr(), DeserializationOption::Filter(filter));
  http.end();
  if (error) {
    Serial.printf("ABC iview: catalogue JSON failed (%s)\n", error.c_str());
    return false;
  }

  String imageUrl;
  for (JsonObjectConst image : document["images"].as<JsonArrayConst>()) {
    const char *type = image["type"] | "";
    const char *url = image["url"] | "";
    if (!url[0]) continue;
    if (!imageUrl.length() || strcmp(type, "portrait") == 0)
      imageUrl = url;
    if (strcmp(type, "portrait") == 0) break;
  }
  if (!imageUrl.startsWith("https://cdn.iview.abc.net.au/")) return false;

  // ABC serves its catalogue originals at 720px or larger. Ask the public
  // image proxy for a compact square so the no-PSRAM C3 can decode it safely,
  // then use the existing 96x96 RGB565/CRC transfer and remote SD cache.
  imageUrl.remove(0, strlen("https://"));
  String compact = String("https://wsrv.nl/?url=") + imageUrl +
                   "&w=192&h=192&fit=cover&output=jpg";
  String key = String("abc://v1/") + slug;
  copyField(compact, metadata.artworkUrl, sizeof(metadata.artworkUrl));
  copyField(key, metadata.artworkKey, sizeof(metadata.artworkKey));
  copyField(slug, lastAbcIviewSlug_, sizeof(lastAbcIviewSlug_));
  return metadata.artworkUrl[0] && metadata.artworkKey[0];
}

bool AppleTvMetadataClient::pollAbcIview(
    const IPAddress &googleTv, AbcIviewRichMetadata &metadata) {
  // ABC iview creates no Android media session. Down reveals its playback
  // controls without pausing; the accessibility tree then contains the show,
  // episode, play state and both clocks. First inspect without a key: profile,
  // home and detail screens all contain TextViews, while ABC's hidden full-
  // screen video controls expose no meaningful accessibility nodes. This
  // guard is what prevents a metadata request from navigating or exiting ABC
  // while someone is choosing a profile such as Jane.
  static const char inspectCommand[] =
    "shell:rm -f /data/local/tmp/openremote_abc.xml; "
    "uiautomator dump /data/local/tmp/openremote_abc.xml >/dev/null 2>&1 & p=$!; "
    "i=0; while kill -0 $p 2>/dev/null && test $i -lt 10; do "
    "echo ORABC_WAIT; sleep 1; i=$((i+1)); done; kill $p 2>/dev/null; "
    "if grep -q 'android.widget.TextView' /data/local/tmp/openremote_abc.xml "
    "2>/dev/null; then echo ORABC_VISIBLE; else echo ORABC_VIDEO; fi; "
    "if test -s /data/local/tmp/openremote_abc.xml; then "
    "sed 's/></>\\n</g' /data/local/tmp/openremote_abc.xml | "
    "grep -E 'Classified|android.widget.TextView|content-desc=\"(Pause|Play)\"' | "
    "head -n 14; fi";

  String output;
  if (!runAdbShell(googleTv, inspectCommand, output, 3072)) {
    Serial.println("ABC iview: could not inspect the current screen");
    return false;
  }
  bool controlsAlreadyVisible =
    output.indexOf("content-desc=\"Pause\"") >= 0 ||
    output.indexOf("content-desc=\"Play\"") >= 0;
  if (!controlsAlreadyVisible && output.indexOf("ORABC_VISIBLE") >= 0) {
    // ABC is open but no video is playing full screen. Clear any previous
    // episode without moving focus or sending Back.
    metadata = {};
    return true;
  }

  // UIAutomator is quiet for several seconds while it builds the hierarchy.
  // Keep the raw ADB shell alive while it runs: Android terminates a detached
  // process when that shell closes. Do not send Back afterward: ABC interprets
  // Back as leave-player, not hide-controls. Its overlay fades by itself.
  static const char revealCommand[] =
    "shell:rm -f /data/local/tmp/openremote_abc.xml; input keyevent 20; "
    "uiautomator dump /data/local/tmp/openremote_abc.xml >/dev/null 2>&1 & p=$!; "
    "i=0; while kill -0 $p 2>/dev/null && test $i -lt 10; do "
    "echo ORABC_WAIT; sleep 1; i=$((i+1)); done; kill $p 2>/dev/null; "
    "if test -s /data/local/tmp/openremote_abc.xml; then "
    "sed 's/></>\\n</g' /data/local/tmp/openremote_abc.xml | "
    "grep -E 'Classified|android.widget.TextView|content-desc=\"(Pause|Play)\"' | "
    "head -n 14; fi";
  if (!controlsAlreadyVisible &&
      !runAdbShell(googleTv, revealCommand, output, 3072)) {
    Serial.println("ABC iview: could not open the playback overlay");
    return false;
  }
  if (output.indexOf("<node") < 0) {
    Serial.println("ABC iview: playback accessibility tree did not finish");
    return false;
  }

  String description;
  int descriptionAt = output.indexOf("content-desc=\"");
  while (descriptionAt >= 0) {
    descriptionAt += strlen("content-desc=\"");
    int end = output.indexOf('"', descriptionAt);
    if (end < 0) break;
    String candidate = output.substring(descriptionAt, end);
    if (candidate.indexOf(", Classified ") >= 0) {
      description = candidate;
      break;
    }
    descriptionAt = output.indexOf("content-desc=\"", end + 1);
  }

  uint32_t times[2] = {};
  uint8_t timeCount = 0;
  int textAt = 0;
  while (timeCount < 2 &&
         (textAt = output.indexOf("text=\"", textAt)) >= 0) {
    textAt += strlen("text=\"");
    int end = output.indexOf('"', textAt);
    if (end < 0) break;
    String value = output.substring(textAt, end);
    textAt = end + 1;
    uint32_t total = 0;
    uint32_t part = 0;
    uint8_t colons = 0;
    bool validTime = value.length() >= 3;
    for (size_t i = 0; validTime && i < value.length(); i++) {
      char c = value[i];
      if (isDigit(c)) {
        part = part * 10 + (uint32_t)(c - '0');
      } else if (c == ':' && colons < 2) {
        total = total * 60 + part;
        part = 0;
        colons++;
      } else {
        validTime = false;
      }
    }
    if (validTime && colons >= 1) times[timeCount++] = total * 60 + part;
  }

  if (!description.length() || timeCount < 2) {
    Serial.printf("ABC iview: overlay parse missed fields (bytes=%u, description=%s, times=%u)\n",
                  (unsigned)output.length(), description.length() ? "yes" : "no",
                  (unsigned)timeCount);
    Serial.println(output);
    metadata = {};
    return true;
  }
  description.replace("&amp;", "&");
  description.replace("&apos;", "'");
  description.replace("&#39;", "'");
  description.replace("&quot;", "\"");
  int classifiedAt = description.indexOf(", Classified ");
  if (classifiedAt >= 0) description.remove(classifiedAt);

  int splitAt = -1;
  for (int at = description.indexOf(", S"); at >= 0;
       at = description.indexOf(", S", at + 2)) {
    int digit = at + 3;
    if (digit < (int)description.length() && isDigit(description[digit]) &&
        description.indexOf(" Episode ", digit) >= 0) {
      splitAt = at;
      break;
    }
  }
  String title = splitAt >= 0 ? description.substring(0, splitAt)
                              : description;
  String subtitle = splitAt >= 0 ? description.substring(splitAt + 2)
                                 : String();
  title.trim();
  subtitle.trim();

  AbcIviewRichMetadata fresh = {};
  fresh.valid = title.length() > 0;
  fresh.playing = output.indexOf("content-desc=\"Pause\"") >= 0;
  fresh.positionSeconds = times[0];
  fresh.durationSeconds = times[1];
  copyField(title, fresh.title, sizeof(fresh.title));
  copyField(subtitle, fresh.subtitle, sizeof(fresh.subtitle));
  resolveAbcArtwork(title.c_str(), fresh);
  metadata = fresh;
  Serial.printf("ABC iview: %s - %s (%lu/%lus)\n", metadata.title,
                metadata.subtitle, (unsigned long)metadata.positionSeconds,
                (unsigned long)metadata.durationSeconds);
  return true;
}

bool AppleTvMetadataClient::capturePlexConnection(const IPAddress &googleTv) {
  static const char command[] =
    // Changing logcat's ring-buffer size clears its contents on Android TV.
    // Discovery must be read-only or it deletes the exact URL it needs.
    "shell:logcat -d -v raw | "
    "grep 'Image URL' | grep 'X-Plex-Token=' | tail -n 1";
  String line;
  if (!runAdbShell(googleTv, command, line, 1536)) return false;
  int urlStart = line.indexOf("http");
  if (urlStart < 0) return false;
  return learnPlexConnection(line.substring(urlStart).c_str());
}

bool AppleTvMetadataClient::learnPlexConnection(const char *artworkUrl) {
  if (!artworkUrl || !artworkUrl[0]) return false;
  String line(artworkUrl);
  int urlStart = line.indexOf("http");
  int photo = line.indexOf("/photo/", urlStart);
  int tokenStart = line.indexOf("X-Plex-Token=", urlStart);
  if (urlStart < 0 || photo <= urlStart || tokenStart < 0) return false;
  tokenStart += strlen("X-Plex-Token=");
  int tokenEnd = tokenStart;
  while (tokenEnd < (int)line.length()) {
    char c = line[tokenEnd];
    if (c == '&' || c == '\r' || c == '\n' || c == ' ' || c == '\t') break;
    tokenEnd++;
  }
  String server = line.substring(urlStart, photo);
  String token = line.substring(tokenStart, tokenEnd);
  if (!server.length() || !token.length() || server.length() >= sizeof(plexServer_) ||
      token.length() >= sizeof(plexToken_)) return false;

  String machine;
  int machineAt = line.indexOf("machineIdentifier=", urlStart);
  if (machineAt >= 0) {
    machineAt += strlen("machineIdentifier=");
    int machineEnd = line.indexOf('&', machineAt);
    if (machineEnd < 0) machineEnd = line.length();
    machine = line.substring(machineAt, machineEnd);
  }
  String ratingKey;
  int encodedUrlAt = line.indexOf("url=", urlStart);
  if (encodedUrlAt >= 0) {
    encodedUrlAt += 4;
    int encodedUrlEnd = line.indexOf('&', encodedUrlAt);
    if (encodedUrlEnd < 0) encodedUrlEnd = line.length();
    String mediaPath = urlDecode(line.substring(encodedUrlAt, encodedUrlEnd));
    int metadataAt = mediaPath.indexOf("/library/metadata/");
    if (metadataAt >= 0) {
      metadataAt += strlen("/library/metadata/");
      int ratingEnd = mediaPath.indexOf('/', metadataAt);
      if (ratingEnd < 0) ratingEnd = mediaPath.length();
      ratingKey = mediaPath.substring(metadataAt, ratingEnd);
    }
  }
  bool connectionChanged = server != plexServer_ || token != plexToken_;
  strlcpy(plexServer_, server.c_str(), sizeof(plexServer_));
  strlcpy(plexToken_, token.c_str(), sizeof(plexToken_));
  if (machine.length() && machine.length() < sizeof(plexMachineId_))
    strlcpy(plexMachineId_, machine.c_str(), sizeof(plexMachineId_));
  if (ratingKey.length() && ratingKey.length() < sizeof(plexCurrentRatingKey_))
    strlcpy(plexCurrentRatingKey_, ratingKey.c_str(),
            sizeof(plexCurrentRatingKey_));
  if (!connectionChanged) return true;
  plexSessionEndpointForbidden_ = false;
  Preferences stored;
  stored.begin("plexmeta", false);
  stored.putString("server", plexServer_);
  stored.putString("token", plexToken_);
  stored.end();
  Serial.printf("Plex: saved server connection %s\n", plexServer_);
  return true;
}

bool AppleTvMetadataClient::resolvePlexSession(const IPAddress &googleTv,
                                               PlexRichMetadata &metadata) {
  if (!plexServer_[0] || !plexToken_[0]) return false;
  String url = String(plexServer_) + "/status/sessions";
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(8000);
  NetworkClient plain;
  NetworkClientSecure secure;
  secure.setInsecure();
  bool begun = url.startsWith("https://") ? http.begin(secure, url)
                                          : http.begin(plain, url);
  if (!begun) return false;
  http.addHeader("X-Plex-Token", plexToken_);
  int status = http.GET();
  if (status != 200) {
    Serial.printf("Plex: sessions returned HTTP %d\n", status);
    if (status == 401 || status == 403) plexSessionEndpointForbidden_ = true;
    http.end();
    return false;
  }
  int length = http.getSize();
  if (length > 65536) {
    http.end();
    return false;
  }
  String body = http.getString();
  http.end();
  String addressMarker = String("address=\"") + googleTv.toString() + "\"";
  int videoAt = 0;
  while ((videoAt = body.indexOf("<Video ", videoAt)) >= 0) {
    int videoTagEnd = body.indexOf('>', videoAt);
    int videoEnd = body.indexOf("</Video>", videoTagEnd);
    if (videoTagEnd < 0 || videoEnd < 0) break;
    int playerAt = body.indexOf("<Player ", videoTagEnd);
    if (playerAt < 0 || playerAt > videoEnd) {
      videoAt = videoEnd + 8;
      continue;
    }
    int playerEnd = body.indexOf('>', playerAt);
    String playerTag = body.substring(playerAt, playerEnd + 1);
    if (playerTag.indexOf(addressMarker) < 0) {
      videoAt = videoEnd + 8;
      continue;
    }

    String videoTag = body.substring(videoAt, videoTagEnd + 1);
    String title = xmlAttribute(videoTag, "title");
    String ratingKey = xmlAttribute(videoTag, "ratingKey");
    if (!title.length() || !ratingKey.length()) return false;
    String type = xmlAttribute(videoTag, "type");
    String year = xmlAttribute(videoTag, "year");
    String show = xmlAttribute(videoTag, "grandparentTitle");
    String episode = xmlAttribute(videoTag, "parentTitle");
    String subtitle;
    if (type == "episode") {
      subtitle = show;
      if (episode.length()) subtitle += String(" - ") + episode;
    } else {
      subtitle = year;
      if (type.length()) {
        if (subtitle.length()) subtitle += " - ";
        type[0] = (char)toupper((unsigned char)type[0]);
        subtitle += type;
      }
    }
    String thumb = xmlAttribute(videoTag, "thumb");
    String duration = xmlAttribute(videoTag, "duration");
    String offset = xmlAttribute(videoTag, "viewOffset");
    String state = xmlAttribute(playerTag, "state");
    String cacheKey = String("plex://") + googleTv.toString() + "/" + ratingKey;
    String fetchUrl;
    if (thumb.length()) {
      fetchUrl = String(plexServer_) +
        "/photo/:/transcode?width=96&height=96&minSize=1&upscale=1&url=" +
        urlEncode(thumb) + "&X-Plex-Token=" + urlEncode(plexToken_);
    }
    metadata = {};
    metadata.valid = true;
    metadata.playing = state == "playing";
    metadata.playbackFromSession = true;
    copyField(ratingKey, metadata.ratingKey, sizeof(metadata.ratingKey));
    copyField(title, metadata.title, sizeof(metadata.title));
    copyField(subtitle, metadata.subtitle, sizeof(metadata.subtitle));
    copyField(cacheKey, metadata.artworkKey, sizeof(metadata.artworkKey));
    copyField(fetchUrl, metadata.artworkFetchUrl, sizeof(metadata.artworkFetchUrl));
    metadata.positionSeconds = (uint32_t)(offset.toInt() / 1000);
    metadata.durationSeconds = (uint32_t)(duration.toInt() / 1000);
    return true;
  }
  // The server answered correctly and has no session for this Chromecast.
  // That is a real stopped state, not a network failure. Return success with
  // an invalid/empty record so the caller can clear the previous title, clock
  // and poster immediately.
  metadata = {};
  return true;
}

bool AppleTvMetadataClient::resolvePlexItem(PlexRichMetadata &metadata) {
  if (!plexServer_[0] || !plexToken_[0] || !plexCurrentRatingKey_[0])
    return false;
  String url = String(plexServer_) + "/library/metadata/" +
               plexCurrentRatingKey_;
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(8000);
  NetworkClient plain;
  NetworkClientSecure secure;
  secure.setInsecure();
  bool begun = url.startsWith("https://") ? http.begin(secure, url)
                                          : http.begin(plain, url);
  if (!begun) return false;
  http.addHeader("X-Plex-Token", plexToken_);
  int status = http.GET();
  if (status != 200) {
    Serial.printf("Plex: item metadata returned HTTP %d\n", status);
    http.end();
    return false;
  }
  int length = http.getSize();
  if (length > 65536) {
    http.end();
    return false;
  }
  String body = http.getString();
  http.end();
  int videoAt = body.indexOf("<Video ");
  int videoEnd = body.indexOf('>', videoAt);
  if (videoAt < 0 || videoEnd < 0) return false;
  String videoTag = body.substring(videoAt, videoEnd + 1);
  String title = xmlAttribute(videoTag, "title");
  String ratingKey = xmlAttribute(videoTag, "ratingKey");
  if (!title.length() || !ratingKey.length()) return false;
  String type = xmlAttribute(videoTag, "type");
  String year = xmlAttribute(videoTag, "year");
  String show = xmlAttribute(videoTag, "grandparentTitle");
  String episode = xmlAttribute(videoTag, "parentTitle");
  String subtitle;
  if (type == "episode") {
    subtitle = show;
    if (episode.length()) subtitle += String(" - ") + episode;
  } else {
    subtitle = year;
    if (type.length()) {
      if (subtitle.length()) subtitle += " - ";
      type[0] = (char)toupper((unsigned char)type[0]);
      subtitle += type;
    }
  }
  String thumb = xmlAttribute(videoTag, "thumb");
  String duration = xmlAttribute(videoTag, "duration");
  String offset = xmlAttribute(videoTag, "viewOffset");
  String identity = plexMachineId_[0] ? String(plexMachineId_)
                                      : String(plexServer_);
  String cacheKey = String("plex://") + identity + "/" + ratingKey;
  String fetchUrl;
  if (thumb.length()) {
    fetchUrl = String(plexServer_) +
      "/photo/:/transcode?width=96&height=96&minSize=1&upscale=1&url=" +
      urlEncode(thumb) + "&X-Plex-Token=" + urlEncode(plexToken_);
  }
  metadata = {};
  metadata.valid = true;
  metadata.playbackFromSession = false;
  copyField(ratingKey, metadata.ratingKey, sizeof(metadata.ratingKey));
  copyField(title, metadata.title, sizeof(metadata.title));
  copyField(subtitle, metadata.subtitle, sizeof(metadata.subtitle));
  copyField(cacheKey, metadata.artworkKey, sizeof(metadata.artworkKey));
  copyField(fetchUrl, metadata.artworkFetchUrl,
            sizeof(metadata.artworkFetchUrl));
  metadata.positionSeconds = (uint32_t)(offset.toInt() / 1000);
  metadata.durationSeconds = (uint32_t)(duration.toInt() / 1000);
  return true;
}

bool AppleTvMetadataClient::pollPlex(const IPAddress &googleTv,
                                     PlexRichMetadata &metadata) {
  uint32_t now = millis();
  if (nextPlexPollMs_ && (int32_t)(now - nextPlexPollMs_) < 0) return false;
  nextPlexPollMs_ = now + PLEX_POLL_MS;
  if (!nextPlexCaptureMs_ || (int32_t)(now - nextPlexCaptureMs_) >= 0) {
    nextPlexCaptureMs_ = now + PLEX_CAPTURE_RETRY_MS;
    capturePlexConnection(googleTv);
  }
  PlexRichMetadata fresh = {};
  if (plexSessionEndpointForbidden_) {
    if (!resolvePlexItem(fresh)) return false;
  } else if (!resolvePlexSession(googleTv, fresh)) {
    // The account token can change. Refresh it from Plex's own current player
    // log, then retry once; the token is never sent to the remote or printed.
    if (capturePlexConnection(googleTv) && resolvePlexSession(googleTv, fresh)) {
      metadata = fresh;
      return true;
    }
    // Shared-library users can read the current item but Plex deliberately
    // denies them the server-owner sessions list. The Android session supplies
    // play/pause and position; this item lookup supplies the rich fields.
    if (!plexSessionEndpointForbidden_ || !resolvePlexItem(fresh)) return false;
  }
  if (!fresh.valid && metadata.valid) {
    // Plex briefly removes a session while starting a player, seeking, or
    // replacing one item with the next. Keep the last confirmed record through
    // one such gap; two consecutive successful empty responses mean playback
    // really stopped and should clear the widget.
    if (++plexEmptyPolls_ < PLEX_STOP_CONFIRMATIONS) return false;
  } else if (fresh.valid) {
    plexEmptyPolls_ = 0;
  }
  metadata = fresh;
  return true;
}

bool AppleTvMetadataClient::pollStremio(
    const IPAddress &googleTv, StremioArtworkMetadata &metadata) {
  uint32_t now = millis();
  if (nextStremioPollMs_ &&
      (int32_t)(now - nextStremioPollMs_) < 0) return false;
  nextStremioPollMs_ = now + STREMIO_POLL_MS;

  // Stremio's Android media session and Cast record contain title and timing,
  // but no image. Its local server logs the catalogue stream request with a
  // stable content ID such as series/tt14688458%3A3%3A1.json. Ask Android to
  // return only those short lines instead of transferring the full log buffer.
  static const char command[] =
    "shell:logcat -d -v raw -s StremioServer:I | "
    "grep '/local-addon/stream/' | tail -n 8";
  String output;
  if (!runAdbShell(googleTv, command, output, 2048)) return false;
  int marker = output.lastIndexOf("/local-addon/stream/");
  if (marker < 0) return false;
  marker += strlen("/local-addon/stream/");
  int typeEnd = output.indexOf('/', marker);
  int jsonEnd = output.indexOf(".json", typeEnd + 1);
  if (typeEnd < 0 || jsonEnd < 0) return false;
  String type = output.substring(marker, typeEnd);
  String contentId = output.substring(typeEnd + 1, jsonEnd);
  contentId.replace("%3A", ":");
  contentId.replace("%3a", ":");
  int separator = contentId.indexOf(':');
  String catalogueId = separator >= 0 ? contentId.substring(0, separator)
                                      : contentId;
  if (type != "series" && type != "movie") return false;
  if (!catalogueId.startsWith("tt") || catalogueId.length() < 4) return false;
  for (size_t i = 2; i < catalogueId.length(); i++) {
    if (!isDigit(catalogueId[i])) return false;
  }
  if (contentId == lastStremioContentId_ && metadata.valid) return false;

  // v2 invalidates images decoded before the JPEGDEC scale-option fix. Those
  // files contain only the top-left of the source and must not be reused.
  String artworkKey = String("stremio://v4/") + type + "/" + catalogueId;
  String artworkUrl = String("https://images.metahub.space/poster/small/") +
                      catalogueId + "/img";
  metadata = {};
  metadata.valid = true;
  copyField(contentId, metadata.contentId, sizeof(metadata.contentId));
  copyField(artworkKey, metadata.artworkKey, sizeof(metadata.artworkKey));
  copyField(artworkUrl, metadata.artworkUrl, sizeof(metadata.artworkUrl));
  copyField(contentId, lastStremioContentId_, sizeof(lastStremioContentId_));
  Serial.printf("Stremio: resolved poster for %s\n", catalogueId.c_str());
  return true;
}

bool AppleTvMetadataClient::ensurePrimeMediaHelper(
    const IPAddress &googleTv) {
  if (primeMediaHelperReady_) return true;

  String output;
  String check = String("shell:test \"$(wc -c < ") + PRIME_HELPER_PATH +
                 " 2>/dev/null)\" = \"" + String(PRIME_HELPER_BYTES) +
                 "\" && echo ready";
  if (runAdbShell(googleTv, check.c_str(), output, 32) &&
      output.indexOf("ready") >= 0) {
    primeMediaHelperReady_ = true;
    return true;
  }

  // Android's stock media-session commands intentionally print only a
  // description and the byte size of the extras Bundle. Prime keeps its GTI
  // in that hidden Bundle. Install a tiny read-only app_process helper through
  // the already-approved ADB shell so the dock can ask for only that public
  // catalogue identifier. /data/local/tmp survives ordinary TV reboots.
  String install;
  install.reserve(strlen(OPENREMOTE_PRIME_MEDIA_HELPER_V1_BASE64) + 192);
  install = "shell:printf '%s' '";
  install += OPENREMOTE_PRIME_MEDIA_HELPER_V1_BASE64;
  install += "' | base64 -d > ";
  install += PRIME_HELPER_PATH;
  install += " && chmod 644 ";
  install += PRIME_HELPER_PATH;
  install += " && test \"$(wc -c < ";
  install += PRIME_HELPER_PATH;
  install += ")\" = \"";
  install += String(PRIME_HELPER_BYTES);
  install += "\" && echo ready";
  output = "";
  if (!runAdbShell(googleTv, install.c_str(), output, 32) ||
      output.indexOf("ready") < 0) {
    Serial.println("Prime Video: could not cache the media-session helper");
    return false;
  }
  primeMediaHelperReady_ = true;
  Serial.println("Prime Video: media-session helper cached on Google TV");
  return true;
}

bool AppleTvMetadataClient::readPrimeContentId(
    const IPAddress &googleTv, char *contentId, size_t contentIdSize) {
  if (!contentId || contentIdSize < 50 || !ensurePrimeMediaHelper(googleTv))
    return false;
  String command = String("shell:CLASSPATH=") + PRIME_HELPER_PATH +
                   " app_process /system/bin OpenRemotePrimeMedia 2>/dev/null";
  String output;
  if (!runAdbShell(googleTv, command.c_str(), output, 160)) return false;
  int at = output.indexOf("id=amzn1.dv.gti.");
  if (at < 0) return false;
  at += 3;
  int end = output.indexOf('\n', at);
  if (end < 0) end = output.length();
  String id = output.substring(at, end);
  id.trim();
  if (!id.startsWith("amzn1.dv.gti.") || id.length() >= contentIdSize)
    return false;
  strlcpy(contentId, id.c_str(), contentIdSize);
  return true;
}

bool AppleTvMetadataClient::resolvePrimeCatalogue(
    const char *contentId, PrimeVideoRichMetadata &metadata) {
  if (!contentId || !contentId[0]) return false;
  NetworkClientSecure secure;
  secure.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(12000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  String url = String("https://www.primevideo.com/detail/") + contentId;
  if (!http.begin(secure, url)) return false;
  http.addHeader("Accept-Language", "en-AU,en;q=0.9");
  http.addHeader("User-Agent", "Mozilla/5.0 OpenRemote-Dock");
  int status = http.GET();
  if (status != 200) {
    Serial.printf("Prime Video: catalogue returned HTTP %d\n", status);
    http.end();
    return false;
  }

  String title, artwork, titleType;
  uint32_t duration = 0;
  uint16_t releaseYear = 0;
  String window;
  window.reserve(2560);
  String targetMarker = String("\"pageTitleId\":\"") + contentId + "\"";
  bool targetDetailReached = false;
  NetworkClient *stream = http.getStreamPtr();
  uint32_t deadline = millis() + 20000;
  while ((int32_t)(millis() - deadline) < 0) {
    int available = stream->available();
    if (!available) {
      if (!http.connected()) break;
      delay(1);
      continue;
    }
    uint8_t chunk[512];
    int got = stream->read(chunk, min(available, (int)sizeof(chunk)));
    if (got <= 0) continue;
    window.concat((const char *)chunk, (unsigned int)got);

    if (!title.length()) {
      int at = window.indexOf("<title>Prime Video: ");
      if (at >= 0) {
        at += strlen("<title>Prime Video: ");
        int end = window.indexOf("</title>", at);
        if (end > at) title = htmlDecode(window.substring(at, end));
      }
    }
    if (!artwork.length()) {
      int at = window.indexOf("<meta property=\"og:image\" content=\"");
      if (at >= 0) {
        at += strlen("<meta property=\"og:image\" content=\"");
        int end = window.indexOf('"', at);
        if (end > at) artwork = htmlDecode(window.substring(at, end));
      }
    }
    if (!targetDetailReached && window.indexOf(targetMarker) >= 0)
      targetDetailReached = true;
    if (targetDetailReached) {
      if (!duration) {
        int search = 0;
        while (true) {
          int at = window.indexOf("\"duration\":", search);
          if (at < 0) break;
          at += strlen("\"duration\":");
          int end = at;
          while (end < (int)window.length() && isDigit(window[end])) end++;
          if (end > at && end < (int)window.length()) {
            duration = (uint32_t)window.substring(at, end).toInt();
            break;
          }
          search = at;
        }
      }
      if (!releaseYear) {
        int search = 0;
        while (true) {
          int at = window.indexOf("\"releaseYear\":", search);
          if (at < 0) break;
          at += strlen("\"releaseYear\":");
          int end = at;
          while (end < (int)window.length() && isDigit(window[end])) end++;
          if (end > at && end < (int)window.length()) {
            releaseYear = (uint16_t)window.substring(at, end).toInt();
            break;
          }
          search = at;
        }
      }
      if (!titleType.length()) {
        int search = 0;
        while (true) {
          int at = window.indexOf("\"titleType\":\"", search);
          if (at < 0) break;
          at += strlen("\"titleType\":\"");
          int end = window.indexOf('"', at);
          if (end > at) {
            String candidate = window.substring(at, end);
            if (candidate == "movie" || candidate == "episode") {
              titleType = candidate;
              break;
            }
          }
          search = at;
        }
      }
    }
    if (title.length() && artwork.length() && duration && releaseYear &&
        titleType.length()) {
      break;
    }
    if (window.length() > 2048) window.remove(0, 1024);
  }
  http.end();
  if (!title.length()) return false;

  // Amazon's unmodified packshot can be several megabytes and portrait. The
  // SS rendition is a filled 192px square JPEG, normally around 10-20 KB, so
  // the existing half-scale decoder produces the remote's 96px cache image
  // without black bars.
  int jpg = artwork.lastIndexOf(".jpg");
  if (jpg > 8) artwork = artwork.substring(0, jpg) + "._SS192_.jpg";
  String subtitle;
  if (releaseYear) subtitle = String(releaseYear);
  if (titleType.length()) {
    titleType[0] = (char)toupper((unsigned char)titleType[0]);
    if (subtitle.length()) subtitle += " - ";
    subtitle += titleType;
  }
  String artworkKey;
  // v2 invalidates images decoded before the JPEGDEC scale-option fix. Those
  // files contain only the top-left of the 192px Amazon square rendition.
  if (artwork.length()) artworkKey = String("prime://v4/") + contentId;

  metadata = {};
  metadata.valid = true;
  copyField(contentId, metadata.contentId, sizeof(metadata.contentId));
  copyField(title, metadata.title, sizeof(metadata.title));
  copyField(subtitle, metadata.subtitle, sizeof(metadata.subtitle));
  copyField(artworkKey, metadata.artworkKey, sizeof(metadata.artworkKey));
  copyField(artwork, metadata.artworkUrl, sizeof(metadata.artworkUrl));
  metadata.durationSeconds = duration;
  return true;
}

bool AppleTvMetadataClient::pollPrimeVideo(
    const IPAddress &googleTv, PrimeVideoRichMetadata &metadata) {
  uint32_t now = millis();
  if (nextPrimePollMs_ && (int32_t)(now - nextPrimePollMs_) < 0) return false;
  nextPrimePollMs_ = now + PRIME_POLL_MS;
  char contentId[64] = "";
  if (!readPrimeContentId(googleTv, contentId, sizeof(contentId))) return false;
  if (strcmp(contentId, lastPrimeContentId_) == 0 && metadata.valid)
    return false;
  if (strcmp(contentId, lastPrimeContentId_) != 0) metadata = {};
  PrimeVideoRichMetadata fresh = {};
  if (!resolvePrimeCatalogue(contentId, fresh)) return false;
  metadata = fresh;
  strlcpy(lastPrimeContentId_, contentId, sizeof(lastPrimeContentId_));
  Serial.printf("Prime Video: %s - %s (%lus)\n", metadata.subtitle,
                metadata.title, (unsigned long)metadata.durationSeconds);
  return true;
}

bool AppleTvMetadataClient::readCurrentContentId(const IPAddress &googleTv,
                                                 char *contentId,
                                                 size_t contentIdSize,
                                                 char *kind,
                                                 size_t kindSize) {
  // Google TV advertises its stable authenticated ADB listener on 5555 while
  // Wireless debugging is enabled. Prefer it: unlike the random TLS-connect
  // port, it survives reboots and uses the same RSA identity Android already
  // approved. Newer devices that expose only TLS still use the mDNS path
  // below.
  NetworkClient legacy;
  if (adbConnectLegacy(legacy, googleTv)) {
    bool found = readLunaContentId(legacy, contentId, contentIdSize,
                                   kind, kindSize);
    legacy.stop();
    return found;
  }
  legacy.stop();

  // Android's Wireless debugging TLS service moves ports after a reboot, so
  // cache it only while it works and re-discover it after a failed connection.
  uint16_t tlsPort = 0;
  bool haveCachedPort = cachedTlsPort && cachedTlsHost == googleTv;
  if (haveCachedPort || discoverAdbTlsPort(googleTv, tlsPort)) {
    if (haveCachedPort) tlsPort = cachedTlsPort;
    NetworkClientSecure secure;
    if (adbConnectTls(secure, googleTv, tlsPort)) {
      cachedTlsHost = googleTv;
      cachedTlsPort = tlsPort;
      bool found = readLunaContentId(secure, contentId, contentIdSize,
                                     kind, kindSize);
      secure.stop();
      return found;
    }
    secure.stop();
    cachedTlsPort = 0;
    // A cached port changes after Android restarts. Re-discover once before
    // dropping to the legacy listener.
    if (haveCachedPort && discoverAdbTlsPort(googleTv, tlsPort)) {
      NetworkClientSecure retry;
      if (adbConnectTls(retry, googleTv, tlsPort)) {
        cachedTlsHost = googleTv;
        cachedTlsPort = tlsPort;
        bool found = readLunaContentId(retry, contentId, contentIdSize,
                                       kind, kindSize);
        retry.stop();
        return found;
      }
      retry.stop();
    }
  }

  return false;
}

bool AppleTvMetadataClient::resolveCatalogue(const char *contentId,
                                             const char *kind,
                                             AppleTvRichMetadata &metadata) {
  NetworkClientSecure secure;
  secure.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(10000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  String url = String("https://tv.apple.com/au/") + kind + "/x/" + contentId;
  if (!http.begin(secure, url)) return false;
  int status = http.GET();
  if (status != 200) {
    Serial.printf("Apple TV: catalogue returned HTTP %d\n", status);
    http.end();
    return false;
  }

  String title, subtitle, artwork, duration;
  String tag;
  tag.reserve(512);
  bool insideTag = false;
  NetworkClient *stream = http.getStreamPtr();
  uint32_t deadline = millis() + 12000;
  while ((int32_t)(millis() - deadline) < 0 && http.connected()) {
    int available = stream->available();
    if (!available) { delay(1); continue; }
    while (available-- > 0) {
      char c = (char)stream->read();
      if (c == '<') { insideTag = true; tag = "<"; continue; }
      if (!insideTag) continue;
      if (tag.length() < 1000) tag += c;
      if (c != '>') continue;
      insideTag = false;
      if (tag.indexOf("name=\"apple:title\"") >= 0) title = tagContent(tag);
      else if (tag.indexOf("name=\"apple:description\"") >= 0) subtitle = tagContent(tag);
      else if (tag.indexOf("property=\"og:image\"") >= 0) artwork = tagContent(tag);
      else if (tag.indexOf("property=\"og:video:duration\"") >= 0) duration = tagContent(tag);
      if (title.length() && subtitle.length() && artwork.length() && duration.length()) {
        deadline = 0;
        break;
      }
      tag = "";
    }
  }
  http.end();
  if (!title.length()) return false;
  compactArtworkUrl(artwork);
  metadata = {};
  metadata.valid = true;
  copyField(contentId, metadata.contentId, sizeof(metadata.contentId));
  copyField(title, metadata.title, sizeof(metadata.title));
  copyField(subtitle, metadata.subtitle, sizeof(metadata.subtitle));
  copyField(artwork, metadata.artworkUrl, sizeof(metadata.artworkUrl));
  metadata.durationSeconds = (uint32_t)duration.toInt();
  return true;
}

bool AppleTvMetadataClient::poll(const IPAddress &googleTv,
                                 AppleTvRichMetadata &metadata) {
  uint32_t now = millis();
  if (nextPollMs_ && (int32_t)(now - nextPollMs_) < 0) return false;
  nextPollMs_ = now + APPLE_POLL_MS;
  char contentId[64] = "";
  char kind[12] = "";
  if (!readCurrentContentId(googleTv, contentId, sizeof(contentId),
                            kind, sizeof(kind))) return false;
  if (strcmp(contentId, lastContentId_) == 0 && metadata.valid) return false;
  AppleTvRichMetadata fresh = {};
  if (!resolveCatalogue(contentId, kind, fresh)) return false;
  metadata = fresh;
  strlcpy(lastContentId_, contentId, sizeof(lastContentId_));
  Serial.printf("Apple TV: %s — %s (%lus)\n", metadata.subtitle,
                metadata.title, (unsigned long)metadata.durationSeconds);
  return true;
}

bool AppleTvMetadataClient::decodeArtwork(const char *url, uint16_t *rgb565,
                                          uint16_t width, uint16_t height) {
  if (!url || !url[0] || !rgb565 || !width || !height) return false;
  NetworkClientSecure secure;
  secure.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(10000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(secure, url)) return false;
  int status = http.GET();
  int length = http.getSize();
  if (status != 200 || length <= 0 || length > 65536) {
    Serial.printf("Media: artwork HTTP %d, length %d\n", status, length);
    http.end();
    return false;
  }
  uint8_t *jpegBytes = (uint8_t *)malloc((size_t)length);
  if (!jpegBytes) {
    Serial.printf("Media: no heap for %d-byte JPEG (free=%u largest=%u)\n",
                  length, (unsigned)ESP.getFreeHeap(),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    http.end();
    return false;
  }
  NetworkClient *stream = http.getStreamPtr();
  int got = 0;
  uint32_t deadline = millis() + 10000;
  uint32_t disconnectedAt = 0;
  while (got < length && (int32_t)(millis() - deadline) < 0) {
    int available = stream->available();
    if (available > 0) {
      int read = stream->read(jpegBytes + got,
                              min(available, length - got));
      if (read > 0) {
        got += read;
        disconnectedAt = 0;
      }
    } else if (!http.connected()) {
      // Plex can queue its short JPEG and TLS close together. Give the secure
      // client time to expose already-received plaintext after FIN, just as
      // the ADB reader above does for Android's final WRTE packet.
      if (!disconnectedAt) disconnectedAt = millis();
      if (millis() - disconnectedAt >= 500) break;
      delay(1);
    } else {
      disconnectedAt = 0;
      delay(1);
    }
  }
  http.end();
  if (got != length) {
    Serial.printf("Media: artwork download stopped at %d of %d bytes\n", got, length);
    free(jpegBytes);
    return false;
  }

  memset(rgb565, 0, (size_t)width * height * 2);
  // JPEGDEC carries about 17.5 KB of decoder workspace. A local instance
  // overflows Arduino's loopTask stack on the ESP32-C3, so keep that workspace
  // on the heap for the duration of the conversion.
  JPEGDEC *jpeg = new (std::nothrow) JPEGDEC;
  if (!jpeg) {
    Serial.printf("Media: no heap for JPEG decoder (free=%u largest=%u)\n",
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    free(jpegBytes);
    return false;
  }
  if (!jpeg->openRAM(jpegBytes, length, drawJpegBlock)) {
    Serial.printf("Media: JPEG decoder rejected %d-byte artwork\n", length);
    delete jpeg;
    free(jpegBytes);
    return false;
  }
  int imageWidth = jpeg->getWidth();
  int imageHeight = jpeg->getHeight();
  // JPEGDEC can decode at 1/2, 1/4 or 1/8 size. Choose the smallest rendition
  // that still covers the square target, then crop the excess centrally. This
  // lets Stremio's compact 300x450 series poster fit the C3 and fill the 96x96
  // remote frame without black bars, while leaving existing 96px Apple/Plex
  // art at full resolution.
  int scale = 0;
  while (scale < 3 &&
         (imageWidth >> (scale + 1)) >= width &&
         (imageHeight >> (scale + 1)) >= height) {
    scale++;
  }
  int decodedWidth = max(1, imageWidth >> scale);
  int decodedHeight = max(1, imageHeight >> scale);
  jpegTarget = rgb565;
  jpegTargetWidth = width;
  jpegTargetHeight = height;
  int x = ((int)width - decodedWidth) / 2;
  int y = ((int)height - decodedHeight) / 2;
  // The third JPEGDEC argument is an option bit field, not a shift count.
  // Passing 1 for a half-size decode actually requested an unrelated option,
  // so JPEGDEC returned the full 192px image and our 96px target clipped it to
  // the top-left corner. Keep the shift count for dimensions and translate it
  // to the library's explicit scale flag for the decode itself.
  static const int scaleOptions[] = {
    0, JPEG_SCALE_HALF, JPEG_SCALE_QUARTER, JPEG_SCALE_EIGHTH
  };
  bool decoded = jpeg->decode(x, y, scaleOptions[scale]) != 0;
  if (!decoded) Serial.println("Apple TV: JPEG decode failed");
  jpeg->close();
  delete jpeg;
  jpegTarget = nullptr;
  free(jpegBytes);
  return decoded;
}
