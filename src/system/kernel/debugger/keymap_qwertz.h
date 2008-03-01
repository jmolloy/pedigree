#ifndef KEYMAP_QWERTZ_H
#define KEYMAP_QWERTZ_H

#define CAPSLOCK  0x3a
#define LSHIFT    0x2a
#define RSHIFT    0x36
#define CTRL      0x1d
#define F1        0x3b
#define F2        0x3c
#define F3        0x3d
#define F4        0x3e

char keymap_lower[256];
char keymap_upper[256];

void initKeymap()
{
  keymap_lower[0x00] = 0x00;
  keymap_lower[0x01] = 0x1b; // Escape
  keymap_lower[0x02] = '1';
  keymap_lower[0x03] = '2';
  keymap_lower[0x04] = '3';
  keymap_lower[0x05] = '4';
  keymap_lower[0x06] = '5';
  keymap_lower[0x07] = '6';
  keymap_lower[0x08] = '7';
  keymap_lower[0x09] = '8';
  keymap_lower[0x0a] = '9';
  keymap_lower[0x0b] = '0';
  keymap_lower[0x0c] = '-'; // TODO: ß
  keymap_lower[0x0d] = 0x00; // TODO ´
  keymap_lower[0x0e] = 0x08; // Backspace
  keymap_lower[0x0f] = 0x09; // Tab
  keymap_lower[0x10] = 'q';
  keymap_lower[0x11] = 'w';
  keymap_lower[0x12] = 'e';
  keymap_lower[0x13] = 'r';
  keymap_lower[0x14] = 't';
  keymap_lower[0x15] = 'z';
  keymap_lower[0x16] = 'u';
  keymap_lower[0x17] = 'i';
  keymap_lower[0x18] = 'o';
  keymap_lower[0x19] = 'p';
  keymap_lower[0x1a] = '['; // TODO ü
  keymap_lower[0x1b] = '+';
  keymap_lower[0x1c] = '\n'; // Enter
  keymap_lower[0x1d] = 0x00; // LCtrl
  keymap_lower[0x1e] = 'a';
  keymap_lower[0x1f] = 's';
  keymap_lower[0x20] = 'd';
  keymap_lower[0x21] = 'f';
  keymap_lower[0x22] = 'g';
  keymap_lower[0x23] = 'h';
  keymap_lower[0x24] = 'j';
  keymap_lower[0x25] = 'k';
  keymap_lower[0x26] = 'l';
  keymap_lower[0x27] = ';'; // TODO ö
  keymap_lower[0x28] = '\''; // TODO ä
  keymap_lower[0x29] = '^';
  keymap_lower[0x2a] = 0x00; // LShift
  keymap_lower[0x2b] = '#';
  keymap_lower[0x2c] = 'y';
  keymap_lower[0x2d] = 'x';
  keymap_lower[0x2e] = 'c';
  keymap_lower[0x2f] = 'v';
  keymap_lower[0x30] = 'b';
  keymap_lower[0x31] = 'n';
  keymap_lower[0x32] = 'm';
  keymap_lower[0x33] = ',';
  keymap_lower[0x34] = '.';
  keymap_lower[0x35] = '-';
  keymap_lower[0x36] = 0x00; // RShift
  keymap_lower[0x37] = '*';  // Keypad-*
  keymap_lower[0x38] = 0x00; // LAlt
  keymap_lower[0x39] = ' ';
  
  keymap_upper[0x02] = '!';
  keymap_upper[0x03] = '"';
  keymap_upper[0x04] = 0x00;
  keymap_upper[0x05] = '$';
  keymap_upper[0x06] = '%';
  keymap_upper[0x07] = '&';
  keymap_upper[0x08] = '/';
  keymap_upper[0x09] = '(';
  keymap_upper[0x0a] = ')';
  keymap_upper[0x0b] = '=';
  keymap_upper[0x0c] = '?';
  keymap_upper[0x0d] = '`';
  keymap_upper[0x10] = 'Q';
  keymap_upper[0x11] = 'W';
  keymap_upper[0x12] = 'E';
  keymap_upper[0x13] = 'R';
  keymap_upper[0x14] = 'T';
  keymap_upper[0x15] = 'Z';
  keymap_upper[0x16] = 'U';
  keymap_upper[0x17] = 'I';
  keymap_upper[0x18] = 'O';
  keymap_upper[0x19] = 'P';
  keymap_upper[0x1a] = 0x00; // TODO Ü
  keymap_upper[0x1b] = '*';
  keymap_upper[0x1e] = 'A';
  keymap_upper[0x1f] = 'S';
  keymap_upper[0x20] = 'D';
  keymap_upper[0x21] = 'F';
  keymap_upper[0x22] = 'G';
  keymap_upper[0x23] = 'H';
  keymap_upper[0x24] = 'J';
  keymap_upper[0x25] = 'K';
  keymap_upper[0x26] = 'L';
  keymap_upper[0x27] = 0x00; // TODO Ö
  keymap_upper[0x28] = 0x00; // TODO Ä
  keymap_upper[0x29] = 0x00; // TODO °
  keymap_upper[0x2b] = '\'';
  keymap_upper[0x2c] = 'Y';
  keymap_upper[0x2d] = 'X';
  keymap_upper[0x2e] = 'C';
  keymap_upper[0x2f] = 'V';
  keymap_upper[0x30] = 'B';
  keymap_upper[0x31] = 'N';
  keymap_upper[0x32] = 'M';
  keymap_upper[0x33] = ';';
  keymap_upper[0x34] = ':';
  keymap_upper[0x35] = '_';
  
}

#endif

