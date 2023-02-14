#include <Arduino.h>
#include "training.h"
#include "constants.h"
#include "logger.h"

unsigned int receivedTrainingMsgCounter = 0;
// Reference-timestamps received during (time-sync) training
unsigned long receivedReferenceTimestamps[TRAINING_MSGS_COUNT];
// Local timestamps of the received messages during (time-sync) training
unsigned long receivedTrainingMsgTimestamps[TRAINING_MSGS_COUNT];

unsigned long adjustedReferenceTimestamp;
/// `true`, if a training has been finished successfuly and while no
/// new training has begun.
bool isSuccess = false;

void setTrainingTimeoutIfNeeded() {
  if (receivedTrainingMsgCounter > 0) {
    int index = receivedTrainingMsgCounter - 1;
    if (millis() - receivedTrainingMsgTimestamps[index] > TRAINING_INTERVAL) {
      printTimestamp();
      Serial.print("Training did timeout! Were some Training-Messages (BLE-Packets) lost? (received: ");
      Serial.print(receivedTrainingMsgCounter);
      Serial.print("/");
      Serial.print(TRAINING_MSGS_COUNT);
      Serial.println(")");

      // Handle timeout: Reset counter
      receivedTrainingMsgCounter = 0;
      isSuccess = false;
    }
  }
}

void onReceivedReferenceTimestamp(unsigned long receivedTime, unsigned long referenceTimestamp) {
  // Only a single Training-Message (containing the Reference-Timestamp)
  // is allowed for each Connection-Event.
  if (receivedTrainingMsgCounter > 0) {
    unsigned long lastLocalTimestamp = receivedTrainingMsgTimestamps[receivedTrainingMsgCounter - 1];
    if (receivedTime - lastLocalTimestamp < TRAINING_INTERVAL) {
      Serial.print("WARNING: Already received Training-Message for this Training-Event");
      Serial.print("(lastLocalTimestamp=" + String(lastLocalTimestamp) + ").");
      Serial.print(" Will discard the received Reference-Timestamp");
      Serial.print(" (receivedTime=" + String(receivedTime) + " value=" + String(referenceTimestamp) + ")");
      Serial.println(".");

      return;
    }
  }

  receivedReferenceTimestamps[receivedTrainingMsgCounter] = referenceTimestamp;
  receivedTrainingMsgTimestamps[receivedTrainingMsgCounter] = receivedTime;
  receivedTrainingMsgCounter++;
  isSuccess = false;

  if (receivedTrainingMsgCounter >= TRAINING_MSGS_COUNT) {
    // Training complete.
    int networkDelayTotal = 0;
    for (int i = 0; i < TRAINING_MSGS_COUNT - 1; i++) {
      // unsigned long elapsed = receivedTrainingMsgTimestamps[i + 1] - receivedTrainingMsgTimestamps[i];
      // unsigned long referenceDelay = receivedReferenceTimestamps[i + 1] - receivedReferenceTimestamps[i];
      // 
      // unsigned long networkDelay = elapsed - referenceDelay;
      // Serial.print(i); Serial.print(" -> "); Serial.print(i + 1);
      // Serial.print("\telapsed: ");
      // Serial.print(elapsed);
      // Serial.print("\treferenceDelay: ");
      // Serial.print(referenceDelay);
      // Serial.print("\tnetworkDelay: ");
      // Serial.println(networkDelay);

      // We will measure latency in the number of BLE's Connection-Events between reception
      // of the Training-Messages (which include the Reference-Timestamps).
      //
      // Background: The delta between the local timestamps (therefore `elapsed`) is a multiple of
      // the BLE's Connection-Interval.
      unsigned long elapsed = receivedTrainingMsgTimestamps[i + 1] - receivedTrainingMsgTimestamps[i];
      unsigned long referenceDelay = receivedReferenceTimestamps[i + 1] - receivedReferenceTimestamps[i];
      int networkDelay = elapsed / CONNECTION_INTERVAL; // number of Connection-Events

      Serial.print(i); Serial.print(" -> "); Serial.print(i + 1);
      Serial.print("\telapsed: ");
      Serial.print(elapsed);
      Serial.print(" (ms)\treferenceDelay: ");
      Serial.print(referenceDelay);
      Serial.print(" (ms)\tnetworkDelay: ");
      Serial.print(networkDelay);
      Serial.println(" (number of Connection-Events)");

      networkDelayTotal += networkDelay;
    }

    // For starters we will simply opt for the cumulative average (this may be improved...)
    int networkDelay = networkDelayTotal / (float)(TRAINING_MSGS_COUNT - 1); // number of Connection-Events
    Serial.print("networkDelay (cumulative avg): ");
    Serial.print(networkDelay);
    Serial.println(" (number of Connection-Events)");

    // Note: We'll only add half the delay (1/2 * networkDelay) for the last connection-interval.
    //
    // This is an approximation as we don't know when exactly the Reference-Timestamp was created
    // in the interval of the Connection-Interval of its transmission (when working on Application Level).
    if (networkDelay > 0) {
      adjustedReferenceTimestamp = referenceTimestamp
        + ((networkDelay - 1) * CONNECTION_INTERVAL)
        + (CONNECTION_INTERVAL / 2);
    } else {
      // This case should not be possible. But hey, for the sake of completeness:
      adjustedReferenceTimestamp = referenceTimestamp;
    }

    isSuccess = true;
    receivedTrainingMsgCounter = 0; // reset
  }
}

TrainingStatus trainingStatus() {
  trainingStatusCode_t statusCode;
  if (receivedTrainingMsgCounter > 0) {
    statusCode = trainingPending;
  } 
  else if (isSuccess) {
    statusCode = trainingSucceeded;
  }
  else {
    statusCode = trainingNotStarted;
  }

  struct TrainingStatus status = {
    statusCode,
    adjustedReferenceTimestamp
  };
  return status;
}
