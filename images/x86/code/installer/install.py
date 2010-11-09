import curses
import curses.wrapper

from Installer import Installer

# Change this depending on your installer behaviour
filesdir = "root:/install-files"

# Will get set by "selectDest()"
installdir = "pedigree:/test"

def main(stdscr):
	global filesdir, installdir
	
	return 0
	
	inst = Installer(stdscr, "Pedigree", filesdir, installdir)
	inst.setupCurses()
	
	inst.introPage()
	inst.selectDest()
	inst.installFiles()
	inst.postInstall()
	inst.done()

print "Installer is starting up..."

try:
	stdscr = curses.initscr()
	exit()
	curses.endwin()
	
	curses.noecho()
	
	# main(stdscr)
except:
	pass

curses.nocbreak()
stdscr.keypad(0)
curses.echo()
curses.endwin()
	
# curses.wrapper(main)
