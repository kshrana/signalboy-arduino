/*
  Training (Time-Sync)
*/

typedef enum {
  trainingNotStarted,
  trainingPending,
  trainingSucceeded
} trainingStatusCode_t;

struct TrainingStatus {
  trainingStatusCode_t statusCode;
  unsigned long adjustedReferenceTimestamp;
};

/// Discards (ongoing) Time-Sync/Training, if a certain timeout-duration has elapsed
/// since receiving the last Training-Msg (during an ongoing Training).
void setTrainingTimeoutIfNeeded();
void onReceivedReferenceTimestamp(unsigned long referenceTimestamp);
TrainingStatus trainingStatus();
