/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <string>
#include <iostream>

static void waitForEnter()
{
    std::string ign;
    std::cout << std::endl << "Press ENTER to continue...";
    std::getline(std::cin, ign);

    std::cout << "\e[2J";
}

static void waitForInput(const std::string &desired, const std::string &info)
{
    std::string ign;
    while (ign != desired)
    {
        std::cout << std::endl;
        std::cout << info << std::endl;
        std::cout << "Enter '" << desired << "' (then ENTER) to continue...";
        std::getline(std::cin, ign);
    }

    std::cout << "\e[2J";
}

static void runProgram(const char *cmd, char * const args[])
{
    pid_t f = fork();
    if (!f)
    {
        if (execv(cmd, args) != 0)
        {
            std::cerr << "Failed to run command '" << cmd << "'." << std::endl;
        }
        exit(1);
    }
    else
    {
        waitpid(f, 0, 0);
    }
}

int main(int argc, char *argv[])
{
    const std::string separator("»");
    const std::string separatorPrompt("To enter the '»' separator, use RIGHTALT-.");

    // Undo any silliness.
    chdir("root»/");

    std::cout << "Welcome to Pedigree!" << std::endl << std::endl;
    std::cout << "This tour is designed to help you understand how Pedigree " \
        "differs from other UNIX-like systems. It's interactive, so you can " \
        "practice along the way." << std::endl << std::endl;

    waitForEnter();

    std::cout << "First, let's run `ls` for you:" << std::endl;

    char * const args[] = {"--color=auto", "/", 0};
    runProgram("/applications/ls", args);

    std::cout << std::endl;

    std::cout << "As you can see, the typical /bin, /lib, /var (and so on) are "
        "not present. Instead, you find /applications, /libraries, /system, "
        "/config, and so on. This is designed to be intuitive though it can "
        "break programs with direct path assumptions!" << std::endl;

    std::cout << std::endl;

    std::cout << "After the tour completes, you can navigate around the "
        "filesystem to to get a closer look at what each directory contains."
        << std::endl;

    waitForEnter();

    std::cout << "Another significant difference in Pedigree is the path "
        "structure. In Pedigree, paths follow the format "
        "[mount]»/path/to/file." << std::endl;

    std::cout << std::endl;

    std::cout << "We've chdir'd to root»/ if you were elsewhere. The root "
        "mount always exists; Pedigree will not start without it. Your "
        "applications and configuration exist under root»/." << std::endl;

    std::cout << std::endl;

    std::cout << "Paths that being with a '/' will always operate in your "
        "current mount. Because the current working directory is root»/, we "
        "can simply run `/applications/ls` to run `root»/applications/ls`."
        << std::endl;

    std::cout << std::endl;

    std::cout << "Before we dig into what other mounts may exist, it's "
        "important to know how to type these paths. You can type the '»' "
        "character in Pedigree by using 'RIGHTALT-.' - try it now." << std::endl;

    waitForInput(separator, separatorPrompt);

    std::cout << "Now that you know how to type the paths, here are a "
        "selection of standard Pedigree mounts:" << std::endl;

    std::cout << std::endl;

    std::cout << "* dev» provides device access (ala /dev)." << std::endl;
    std::cout << "* raw» provides access to raw disks and partitions." << std::endl;
    std::cout << "* scratch» is an entirely in-memory filesystem." << std::endl;
    std::cout << "* runtime» is an in-memory filesystem for runfiles (ala "
        "/run)." << std::endl;
    std::cout << "    Files here can only be modified by their owning process."
        << std::endl;
    std::cout << "* unix» provides a location for named UNIX sockets." << std::endl;

    waitForEnter();

    std::cout << "Note that there is a significant caveat with respect to the "
        "$PATH variable with this scheme. If your $PATH does not contain "
        "absolute paths, you may find that switching working directory to a "
        "different mount point can cause you to be unable to run any commands."
        << std::endl;

    std::cout << std::endl;

    std::cout << "This image has been configured such that the default PATH "
        "does this correctly. There may still be weirdness, and if you notice "
        "things are not quite working correctly, you can always run "
        "`cd root»/` to return to the root mount." << std::endl;

    waitForEnter();

    std::cout << "If something goes wrong, you may find yourself in the "
        "Pedigree kernel debugger. This can also be accessed on-demand by "
        "pressing F12 at any time." << std::endl;

    std::cout << std::endl;

    std::cout << "In the debugger, you can read the kernel log, view "
        "backtraces, and do various other inspections to identify what went "
        "wrong." << std::endl;

    std::cout << std::endl;

    std::cout << "You can use the `help` command to see what is available in "
        "the debugger. If you run into an issue that triggers the debugger, "
        "please try and add a serial port log if you report it to us. Thanks!"
        << std::endl;

    waitForEnter();

    std::cout << "With that, you are now better-equipped to handle Pedigree! "
        "Join us in #pedigree on Freenode IRC, and raise any issues you find "
        "at https://pedigree-project.org." << std::endl;

    std::cout << std::endl;

    std::cout << "And thank you for trying out Pedigree!" << std::endl;

    waitForEnter();

    return 0;
}
