/*
  Training (Time-Sync)
*/

#ifndef training_h
#define training_h

typedef enum {
  trainingNotStarted,
  trainingPending,
  trainingSucceeded,
  trainingFailed
} trainingStatusCode_t;

struct TrainingStatus {
  trainingStatusCode_t statusCode;
  unsigned long adjustedReferenceTimestamp;
};

/// Discards (ongoing) Time-Sync/Training, if a certain timeout-duration has elapsed
/// since receiving the last Training-Msg.
void setTrainingTimeoutIfNeeded(unsigned long now);
void onReceivedReferenceTimestamp(unsigned long receivedTime, unsigned long referenceTimestamp);
TrainingStatus trainingStatus(void);

#endif /* training_h */
