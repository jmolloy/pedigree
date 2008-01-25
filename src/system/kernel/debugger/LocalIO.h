/// LocalIO.h - interface for a concretion of the DebuggerIO class.
/// \author James Molloy

#include <DebuggerIO.h>

#define CONSOLE_WIDTH  80
#define CONSOLE_HEIGHT 25

#define MAKE_SHORT(c,f,b) ( (b << 12) | ((f&0x0F) << 8) | (c&0xFF) )

/**
 * Provides an implementation of DebuggerIO, using the monitor and
 * keyboard.
 */
class LocalIO : public DebuggerIO
{
public:
  /**
   * Default constructor and destructor.
   */
  LocalIO();
  ~LocalIO();

  /**
   * Forces the command line interface not to use the specified number of lines
   * from either the top or bottom of the screen, respectively. Can be used to 
   * create status lines that aren't destroyed by screen scrolling.
   */
  void setCliUpperLimit(int nlines);
  void setCliLowerLimit(int nlines);

  /**
   * Enables or disables the command line interface, allowing full access to the display.
   * disableCli blanks the screen, enableCli blanks it and puts a prompt up.
   */
  void enableCli();
  void disableCli();

  /**
   * Called when a command has completed, and a new prompt has to be shown.
   */
  void showPrompt();

  /**
   * Writes the given text out to the CLI, in the given colour and background colour.
   */
  void writeCli(char *str, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour);

  /**
   * Reads a command from the interface. Blocks until a character is pressed, and then
   * the current buffer is returned in *str, and the return value is true if the command
   * is complete (if enter has been pressed). *str will never exceed maxLen.
   */
  bool readCli(char *str, int maxLen);

  /**
   * Draw a line of characters in the given fore and back colours, in the 
   * horizontal or vertical direction. Note that if the CLI is enabled,
   * anything drawn across the CLI area can be wiped without warning.
   */
  void drawHorizontalLine(char c, int row, int colStart, int colEnd, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour);
  void drawVerticalLine(char c, int col, int rowStart, int rowEnd, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour);

  /**
   * Draws a string of text at the given location in the given colour.
   * note that wrapping is not performed, the string will be clipped.
   */
  void drawString(char *str, int row, int col, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour);

  /**
   * Returns the width and height respectively of the console.
   */
  int getWidth()
  {
    return CONSOLE_WIDTH;
  }
  int getHeight()
  {
    return CONSOLE_HEIGHT;
  }

  /**
   * Allows disabling of refreshes, for example when deleting something then writing it back.
   */
  void enableRefreshes();
  void disableRefreshes();
  void forceRefresh();

private:
  
  /**
   * Framebuffer.
   */
  unsigned short m_pFramebuffer[CONSOLE_WIDTH*CONSOLE_HEIGHT];

  /**
   * Current upper and lower CLI limits.
   */
  int upperCliLimit; /// How many lines from the top of the screen the top of our CLI area is.
  int lowerCliLimit; /// How many lines from the bottom of the screen the bottom of our CLI area is.
};

