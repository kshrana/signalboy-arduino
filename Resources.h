class Resources {
public:
  // "1"
  arduino::String hwRevision;
  // "1"
  arduino::String swRevision;

  // "(HW:1|SW:1.0.0)"
  arduino::String versionDisplay;
  // "↑↓ Browse menu"
  arduino::String browseMenuAction;
  // "***Close menu***"
  arduino::String closeMenuAction;
  // "SELECT to exec."
  arduino::String executeAction;
  // ">Reject conn."
  arduino::String rejectConnection;
  // "Error: "
  arduino::String errorCodeLabelPrefix;

  static Resources *shared;

  Resources();

  /* --- States: --- */
  // "Starting..."
  arduino::String getStateLabel_init();
  // "Await. conn..."
  arduino::String getStateLabel_awaitingConnection();
  // "Conn.(synced=%d)"
  arduino::String getStateLabel_connected(bool isSynced);
};