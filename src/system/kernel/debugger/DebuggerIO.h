/// DebuggerIO.h - interface for the abstract DebuggerIO class.
/// \author James Molloy

/**
 * Note that columns are numbered starting at 0 from the left, and rows are numbered
 * starting at 0 from the top.
 */
class DebuggerIO
{
public:

  /**
   * Enumeration of all possible foreground and background colours.
   */
  enum Colour
  {
    Black       =0,
    Blue        =1,
    Green       =2,
    Cyan        =3,
    Red         =4,
    Magenta     =5,
    Orange      =6,
    LightGrey   =7,
    DarkGrey    =8,
    LightBlue   =9,
    LightGreen  =10,
    LightCyan   =11,
    LightRed    =12,
    LightMagenta=13,
    Yellow      =14,
    White       =15,
  };

  /**
   * Default constructor and destructor.
   */
  DebuggerIO() {}
  ~DebuggerIO() {};

  /**
   * Forces the command line interface not to use the specified number of lines
   * from either the top or bottom of the screen, respectively. Can be used to
   * create status lines that aren't destroyed by screen scrolling.
   */
  void setCliUpperLimit(int nlines) {};
  void setCliLowerLimit(int nlines) {};

  /**
   * Enables or disables the command line interface, allowing full access to the display.
   */
  void enableCli() {};
  void disableCli() {};

  /**
   * Writes the given text out to the CLI, in the given colour and background colour.
   */
  void writeCli(char *str, Colour foreColour, Colour backColour) {};

  /**
   * Reads a command from the interface. Blocks until a character is pressed, and then
   * the current buffer is returned in *str, and the return value is true if the command
   * is complete (if enter has been pressed). *str will never exceed maxLen.
   */
  bool readCli(char *str, int maxLen) {};

  /**
   * Draw a line of characters in the given fore and back colours, in the 
   * horizontal or vertical direction. Note that if the CLI is enabled,
   * anything drawn across the CLI area can be wiped without warning.
   */
  void drawHorizonalLine(char c, int row, int colStart, int colEnd, Colour foreColour, Colour backColour) {};
  void drawVerticalLine(char c, int col, int rowStart, int rowEnd, Colour foreColour, Colour backColour) {};

  /**
   * Draws a string of text at the given location in the given colour.
   * note that wrapping is not performed, the string will be clipped.
   */
  void drawString(char *str, int row, int col, Colour foreColour, Colour backColour) {};

  /**
   * Returns the width and height respectively of the console.
   */
  int getWidth() {};
  int getHeight() {};

  /**
   * Allows disabling of refreshes, for example when deleting something then writing it back.
   */
  void enableRefreshes() {};
  void disableRefreshes() {};
  void forceRefresh() {};

};

