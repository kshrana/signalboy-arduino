// The following code is copied and modified from:
// https://github.com/PaulStoffregen/Time/blob/master/TimeLib.h

/*==============================================================================*/
/* Useful Constants */
#define MILISECS_PER_SEC 1000UL

typedef enum { 
  timeNotSet,
  timeNeedsSync,
  timeSet
} timeStatus_t;

/// in ms
unsigned long now();
void setTime(unsigned long t);

/* time sync functions	*/
timeStatus_t timeStatus();                     // indicates if time has been set and recently synchronized
void setSyncInterval(unsigned long interval);  // set the number of seconds between re-sync
