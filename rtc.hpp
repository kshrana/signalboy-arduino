
void printSqwMode();
// INT0 interrupt callback (IRS)
void pps_tick(void);

void setupRtc();

unsigned long millisRtc(bool skipSuspendInterrupts);
