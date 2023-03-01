
#ifndef Logger_hpp
#define Logger_hpp

class Logger {
public:
  Logger(arduino::HardwareSerial *serial);

  void writeWhileAvailable();
  void flush();

  bool print(const char str[]);
  bool print(const String &s);
  bool print(char c);
  bool print(unsigned char b);
  bool print(int n);
  bool print(unsigned int n);
  bool print(long n);
  bool print(unsigned long n);

  bool println(const char str[]);
  bool println(const String &s);
  bool println(int n);
  bool println(unsigned int n);
  bool println(long n);
  bool println(unsigned long n);

  void printTimestamp();

private:
  /// The destination serial interface.
  arduino::HardwareSerial *serialPtr;

  // string to buffer output
  arduino::String buffer;

  bool print(const char str[], bool shouldAppendEndl);
  bool print(const String &s, bool shouldAppendEndl);
  bool print(char c, bool shouldAppendEndl);
  bool print(unsigned char b, bool shouldAppendEndl);
  bool print(int n, bool shouldAppendEndl);
  bool print(unsigned int n, bool shouldAppendEndl);
  bool print(long n, bool shouldAppendEndl);
  bool print(unsigned long n, bool shouldAppendEndl);

  /// Returns `true`, when Serial is available.
  bool ensureSerial();
  void handleBufferOverflow();
};

#endif /* Logger_hpp */
