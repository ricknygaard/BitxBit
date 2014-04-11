#ifndef __SIMPLE_FILE_INCLUDED__
#define __SIMPLE_FILE_INCLUDED__

#include <Arduino.h>
#include <utility/Sd2Card.h>
#include <utility/SdFat.h>
#include <utility/SdFatUtil.h>

class SimpleFile
{
public:
  SimpleFile();
  ~SimpleFile();

public:
  bool    Init(uint8_t csPin);
  bool    Open(char *fName);
  uint8_t ReadByte();
  int     ReadBuf(void *buf, int numBytes);
  void    Close();

private: // Private data
  Sd2Card  m_card;
  SdVolume m_volume;
  SdFile   m_root;
  SdFile   m_file;
};

#endif
