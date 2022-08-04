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

unsigned long now();  // return the current time as seconds since Jan 1 1970
void setTime(unsigned long t);
void adjustTime(long adjustment);

/* time sync functions	*/
timeStatus_t timeStatus();                     // indicates if time has been set and recently synchronized
void setSyncInterval(unsigned long interval);  // set the number of seconds between re-sync
