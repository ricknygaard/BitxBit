  static final int UDPCMD_SYNCH = 0;
  static final int UDPCMD_STOP  = 1;
  static final int UDPCMD_START = 2;
  static final int UDPCMD_TEMPO = 3;
  static final int UDPCMD_NONE  = 255;

  static final int CLIPID_NONE          = -1;
  static final int CLIPID_FIRST_OF_SHOW = 20;
  
  static final int UDP_PAYLOAD_SIZE        = 6;
  static final int UDP_PAYLOAD_TIME_OFFSET = 0;
  static final int UDP_PAYLOAD_CMD_OFFSET  = 4;
  static final int UDP_PAYLOAD_ID_OFFSET   = 5;
