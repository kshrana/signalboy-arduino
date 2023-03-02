#include <Arduino.h>
#include "training.h"
#include "constants.h"
#include "Globals.hpp"
#include "Logger.hpp"

getExternalTime getTimePtr = nullptr;  // pointer to external time provider function

/// - value == -1 initially (no training attempt started, yet)
/// - value > 0 during Training
/// - value == 0 after at least one Training finished and while no new Training started
int receivedTrainingMsgCounter = -1;

// Reference-timestamps received during (time-sync) training
unsigned long receivedReferenceTimestamps[TRAINING_MSGS_COUNT];
// Local timestamps of the received messages during (time-sync) training
unsigned long receivedTrainingMsgTimestamps[TRAINING_MSGS_COUNT];

unsigned long adjustedReferenceTimestamp;
/// `true`, if the last training was finished successfully.
bool isSuccess = false;

/// Takes the network delay specified in ms and returns the network delay
/// as number of Connection-Events.
int calculateNetworkDelay(unsigned long elapsed) {
  /// Delay in number of Connection-Events
  int networkDelay = elapsed / CONNECTION_INTERVAL;
  // Discussion: While the Training Messages are ideally expected to arrive with
  // a fixed delay of n-times Connection Events, that received time may not be
  // exactly accurate on application level. Thus the network delay is just an
  // approximation.
  if (elapsed % CONNECTION_INTERVAL >= CONNECTION_INTERVAL / 2) {
    networkDelay++;
  }

  return networkDelay;
}

void setTimeProvider(getExternalTime getTimeFunction) {
  getTimePtr = getTimeFunction;
}

bool ensureTimeProvider(void) {
  if (!getTimePtr) {
    Log.printTimestamp();
    Log.println("setTrainingTimeoutIfNeeded() -> FATAL: getTimePtr is null!");
    return false;
  }

  return true;
}

void setTrainingTimeoutIfNeeded(void) {
  if (!ensureTimeProvider()) return;
  unsigned long now = getTimePtr();

  if (receivedTrainingMsgCounter > 0) {
    int index = receivedTrainingMsgCounter - 1;
    int networkDelay = calculateNetworkDelay(now - receivedTrainingMsgTimestamps[index]);

    if (networkDelay > 1) {
      Log.print(now);
      Log.print(" ms -> ");
      Log.print("Training did timeout! Were some Training-Messages (BLE-Packets) lost? (received: ");
      Log.print(receivedTrainingMsgCounter);
      Log.print("/");
      Log.print(TRAINING_MSGS_COUNT);
      Log.println(")");

      // Handle timeout: Reset counter
      receivedTrainingMsgCounter = 0;
      isSuccess = false;
    }
  }
}

void onReceivedReferenceTimestamp(unsigned long receivedTime, unsigned long referenceTimestamp) {
  if (!ensureTimeProvider()) return;

  if (receivedTrainingMsgCounter == -1) {
    // reset
    receivedTrainingMsgCounter = 0;
  }

  receivedTrainingMsgTimestamps[receivedTrainingMsgCounter] = receivedTime;
  receivedReferenceTimestamps[receivedTrainingMsgCounter] = referenceTimestamp;
  receivedTrainingMsgCounter++;
  isSuccess = false;

  if (receivedTrainingMsgCounter >= TRAINING_MSGS_COUNT) {
    // Training complete.
    unsigned int networkDelayTotal = 0;
    /// `true`, when some training event fails to satisfy the Training requirements.
    bool isTrainingValid = true;

    for (int i = 1; i < TRAINING_MSGS_COUNT; i++) {
      // We will measure latency in the number of BLE's Connection-Events between reception
      // of the Training-Messages (which include the Reference-Timestamps).
      //
      // Background: The delta between the local timestamps (therefore `elapsed`) is a multiple of
      // the BLE's Connection-Interval.
      unsigned long elapsed = receivedTrainingMsgTimestamps[i] - receivedTrainingMsgTimestamps[i - 1];
      unsigned long referenceDelay = receivedReferenceTimestamps[i] - receivedReferenceTimestamps[i - 1];

      /// Delay in number of Connection-Events
      int networkDelay = calculateNetworkDelay(elapsed);

      Log.print(String(i - 1) + " -> " + String(i));
      Log.print(String("\t") + "referenceDelay: " + String(referenceDelay) + " (ms)");
      Log.print(String("\t") + "elapsed: " + String(elapsed) + " (ms)");
      Log.println("\t-> networkDelay: " + String(networkDelay) + " (approx. number of Connection-Events)");

      networkDelayTotal += networkDelay;

      // ensure Training Requirement: elapsed
      // Only a single Training-Message (containing the Reference-Timestamp)
      // is allowed for each Connection-Event.
      int minimumElapsed = CONNECTION_INTERVAL / 2;
      if (elapsed < minimumElapsed) {
        Log.print("EXCEPTION: ^^ Minimum delay not satisfied");
        Log.print(" (elapsed=" + String(elapsed) + ")!");
        Log.println(" Will invalidate this Training attempt.");

        isTrainingValid = false;
      }

      // ensure Training Requirement: referenceDelay
      int maximumReferenceDelay = CONNECTION_INTERVAL + (CONNECTION_INTERVAL / 2);
      if (referenceDelay >= maximumReferenceDelay) {
        Log.print("EXCEPTION: ^^ Maximum Reference Delay exceeded");
        Log.print(" (referenceDelay=" + String(referenceDelay) + ")!");
        Log.println(" Will invalidate this Training attempt.");

        isTrainingValid = false;
      }
    }

    // For starters we will simply opt for the cumulative average (this may be improved...)
    int networkDelay = networkDelayTotal / (float)(TRAINING_MSGS_COUNT - 1); // number of Connection-Events
    Log.print("networkDelay (cumulative avg): ");
    Log.print(networkDelay);
    Log.println(" (number of Connection-Events)");

    if (isTrainingValid) {
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

      // Correct delay since receival
      unsigned long now = getTimePtr();
      adjustedReferenceTimestamp += now - receivedTime;
    }

    isSuccess = isTrainingValid;
    // reset
    receivedTrainingMsgCounter = 0;
  }
}

TrainingStatus trainingStatus(void) {
  trainingStatusCode_t statusCode;
  if (receivedTrainingMsgCounter == -1) {
    statusCode = trainingNotStarted;
  }
  else if (receivedTrainingMsgCounter == 0) {
    statusCode = isSuccess ? trainingSucceeded : trainingFailed;
  }
  else {
    statusCode = trainingPending;
  }

  struct TrainingStatus status = {
    statusCode,
    adjustedReferenceTimestamp
  };
  return status;
}
