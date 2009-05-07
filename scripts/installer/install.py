import curses
import curses.wrapper

from Installer import Installer

# Change this depending on your installer behaviour
filesdir = "./files"
installdir = "./test"

def main(stdscr):
	global filesdir, installdir
	
	inst = Installer(stdscr, "Pedigree", filesdir, installdir)
	inst.setupCurses()
	
	inst.introPage()
	inst.selectDest()
	inst.installFiles()
	inst.postInstall()
	inst.done()

curses.wrapper(main)
