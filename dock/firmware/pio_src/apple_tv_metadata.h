#pragma once

#include <Arduino.h>
#include <IPAddress.h>

struct AppleTvRichMetadata {
  bool valid;
  char contentId[64];
  char title[128];
  char subtitle[64];
  char artworkUrl[200];
  uint32_t durationSeconds;
};

struct PlexRichMetadata {
  bool valid;
  bool playing;
  bool playbackFromSession;
  char ratingKey[24];
  char title[128];
  char subtitle[64];
  char artworkKey[200];
  char artworkFetchUrl[512];
  uint32_t positionSeconds;
  uint32_t durationSeconds;
};

struct StremioArtworkMetadata {
  bool valid;
  char contentId[64];
  char artworkKey[200];
  char artworkUrl[200];
};

struct PrimeVideoRichMetadata {
  bool valid;
  char contentId[64];
  char title[128];
  char subtitle[64];
  char artworkKey[200];
  char artworkUrl[200];
  uint32_t durationSeconds;
};

struct AbcIviewRichMetadata {
  bool valid;
  bool playing;
  char title[128];
  char subtitle[64];
  char artworkKey[200];
  char artworkUrl[200];
  uint32_t positionSeconds;
  uint32_t durationSeconds;
};

// Android's active media-session owner is the authority for which native TV
// app currently owns playback. Cast receiver state can lag behind an app
// switch and otherwise resurrect an old Apple TV record.
struct AndroidMediaSession {
  bool valid;
  bool active;
  bool playing;
  uint32_t positionSeconds;
  char packageName[64];
  char foregroundPackageName[64];
};

class AppleTvMetadataClient {
 public:
  void begin();
  bool poll(const IPAddress &googleTv, AppleTvRichMetadata &metadata);
  bool learnPlexConnection(const char *artworkUrl);
  bool pollPlex(const IPAddress &googleTv, PlexRichMetadata &metadata);
  bool pollStremio(const IPAddress &googleTv,
                   StremioArtworkMetadata &metadata);
  bool pollPrimeVideo(const IPAddress &googleTv,
                      PrimeVideoRichMetadata &metadata);
  bool pollAbcIview(const IPAddress &googleTv,
                    AbcIviewRichMetadata &metadata);
  bool pollAndroidMediaSession(const IPAddress &googleTv,
                               AndroidMediaSession &session,
                               bool force = false);
  bool testAdbConnection(const IPAddress &googleTv);
  void resetAdbSession();
  bool decodeArtwork(const char *url, uint16_t *rgb565, uint16_t width,
                     uint16_t height, bool preservePortrait = false,
                     uint16_t *outputWidth = nullptr,
                     uint16_t *outputHeight = nullptr);

 private:
  bool readCurrentContentId(const IPAddress &googleTv, char *contentId,
                            size_t contentIdSize, char *kind, size_t kindSize);
  bool resolveCatalogue(const char *contentId, const char *kind,
                        AppleTvRichMetadata &metadata);
  bool runAdbShell(const IPAddress &googleTv, const char *command,
                   String &output, size_t maximumBytes);
  bool capturePlexConnection(const IPAddress &googleTv);
  bool resolvePlexSession(const IPAddress &googleTv,
                          PlexRichMetadata &metadata);
  bool resolvePlexItem(PlexRichMetadata &metadata);
  bool ensurePrimeMediaHelper(const IPAddress &googleTv);
  bool readPrimeContentId(const IPAddress &googleTv, char *contentId,
                          size_t contentIdSize);
  bool resolvePrimeCatalogue(const char *contentId,
                             PrimeVideoRichMetadata &metadata);
  bool resolveAbcArtwork(const char *title,
                         AbcIviewRichMetadata &metadata);

  uint32_t nextPollMs_ = 0;
  uint32_t nextPlexPollMs_ = 0;
  uint32_t nextPlexCaptureMs_ = 0;
  uint8_t plexEmptyPolls_ = 0;
  uint32_t nextStremioPollMs_ = 0;
  uint32_t nextPrimePollMs_ = 0;
  uint32_t nextMediaSessionPollMs_ = 0;
  char lastContentId_[64] = "";
  char lastStremioContentId_[64] = "";
  char lastPrimeContentId_[64] = "";
  char lastAbcIviewSlug_[96] = "";
  bool primeMediaHelperReady_ = false;
  char plexServer_[96] = "";
  char plexToken_[96] = "";
  char plexMachineId_[48] = "";
  char plexCurrentRatingKey_[24] = "";
  bool plexSessionEndpointForbidden_ = false;
  AndroidMediaSession cachedMediaSession_ = {};
};

extern AppleTvMetadataClient appleTvMetadataClient;
