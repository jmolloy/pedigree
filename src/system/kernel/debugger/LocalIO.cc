#include <LocalIO.h>
#include <utility.h>

LocalIO::LocalIO() :
  upperCliLimit(0),
  lowerCliLimit(0)
{
  // Clear the framebuffer.
  for (int i = 0; i < CONSOLE_WIDTH*CONSOLE_HEIGHT; i++)
  {
    m_pFramebuffer[i] = MAKE_SHORT(' ', DebuggerIO::Black, DebuggerIO::Black); // Leave the colour byte to 00 - black.
  }
}

LocalIO::~LocalIO()
{
}

void LocalIO::setCliUpperLimit(int nlines)
{
  if (nlines < CONSOLE_HEIGHT)
    upperCliLimit = nlines;
}

void LocalIO::setCliLowerLimit(int nlines)
{
  if (nlines < CONSOLE_HEIGHT)
    lowerCliLimit = nlines;
}

void LocalIO::enableCli()
{
  forceRefresh();
}

void LocalIO::disableCli()
{
}

void LocalIO::writeCli(char *str, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour)
{
}

bool LocalIO::readCli(char *str, int maxLen)
{
}

void LocalIO::drawHorizontalLine(char c, int row, int colStart, int colEnd, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour)
{

}

void LocalIO::drawVerticalLine(char c, int col, int rowStart, int rowEnd, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour)
{
}

void LocalIO::drawString(char *str, int row, int col, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour)
{
}

void LocalIO::enableRefreshes()
{
}

void LocalIO::disableRefreshes()
{
}

void LocalIO::forceRefresh()
{
  unsigned short *vidmem = (unsigned short *)0xB8000;

  memcpy((unsigned char *)vidmem, (unsigned char *)m_pFramebuffer, CONSOLE_WIDTH*CONSOLE_HEIGHT*2);
}

