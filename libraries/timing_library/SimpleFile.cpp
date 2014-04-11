#include <SimpleFile.h>

SimpleFile::SimpleFile()
{
}

SimpleFile::~SimpleFile()
{
  m_file.close();
  m_root.close();
}

bool SimpleFile::Init(uint8_t csPin)
{
  // Performs the initialisation required by the sdfatlib library.
  //
  return
    m_card.init(SPI_HALF_SPEED, csPin)
    && m_volume.init(m_card)
    && m_root.openRoot(m_volume);
}

bool SimpleFile::Open(char *fName)
{
  // If a previous file is open, close it first.
  //
  if (m_file.isOpen())
  {
    m_file.close();
  }

  return m_file.open(m_root, fName, O_READ);
}

uint8_t SimpleFile::ReadByte()
{
  uint8_t val = 0;
  
  m_file.read(&val, 1);

  return val;
}

int SimpleFile::ReadBuf(void *buf, int numBytes)
{
  return m_file.read(buf, (uint16_t) numBytes);
}

void SimpleFile::Close()
{
  m_file.close();
}
