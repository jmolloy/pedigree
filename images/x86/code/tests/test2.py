import curses
print "Imported"

w1 = curses.initscr()
print "initscr"

print "Type:", type(w1)

print curses.longname()
print "Baudrate:", curses.baudrate()

print "Has colors:", curses.has_colors()

curses.curs_set(1)

w2=curses.newwin(0,0)
print "New window (fullscreen)"

w2.addch(ord('A'))
w2.addch('B')

w2.addstr("some text")
print "some text"

w2.addstr(" more text")
print "more text"

w2.addstr(" even more text")
print "even more text"

w2.addstr(" even more text")
print "even more text"

w2.addstr(" even more text")
print "even more text"

w2.addstr(" yay border")
w2.border()

w2.refresh()

w2.getch()

curses.endwin()

exit()

w2.addstr(5,10, "text over here")
print "text over here (4,10)"

w2.move(0,0)
print "move to 0,0"

w2.insertln()
print "insert line"

w2.move(0,3)
print "move to 0,3"

for i in range(16):
	w2.move(6+i,5)
	for j in range(16):
		w2.addch(j + i*16)

print "add special characters"

w2.refresh()
print "refresh"

w2.getch()
print "getch gotten"

print "Cursor at: ", w2.getyx()

w2.move(24,78)
w2.addch(64)
w2.addch(65)
w2.addch(66)
w2.refresh()

w2.border()
print "border"

w2.refresh()

w2.getch()
print "getch gotten"

w2.insch(0,0,ord('a'))
w2.insch(0,0,ord('b'))
w2.insch(0,0,ord('c'))
print "insching"
w2.refresh()

w2.getch()

w3 = w2
w4 = w2

curses.endwin()
print "endwin"

print "Done."
