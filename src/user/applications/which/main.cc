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

#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include <map>
using std::map;

#include <string>
using std::string;

#include <sstream>
using std::stringstream;

#include <algorithm>
using std::copy;

#include <vector>
using std::vector;

#include <iostream>
using std::cout;
using std::cin;
using std::cerr;
using std::endl;

#include <regex>
using std::regex;


int process_opts(int argc, char *argv[]);
void usage();
void version();

enum tty_opts
{
    opt_skip_dot,
    opt_show_dot,
    opt_skip_tilde,
    opt_show_tilde,
    opt_tty_only,
};


class opts_t {
public:
    opts_t() :
        all(0), read_aliases(0), read_functions(0), show_dot(0), skip_dot(0),
        show_tilde(0), skip_tilde(0) {};
    int all;
    int read_aliases;
    int read_functions;
    int show_dot;
    int skip_dot;
    int show_tilde;
    int skip_tilde;
} static opts;

static vector<string> aliases, functions;
static string home, cwd;


int process_opts(int argc, char *argv[])
{
    int c, long_option, tty_only = 0;
    struct option long_options[] =
    {
        {"all", no_argument, &opts.all, 1},
        {"read-alias", no_argument, &opts.read_aliases, 1},
        {"skip-alias", no_argument, &opts.read_aliases, 0},
        {"read-functions", no_argument, &opts.read_functions, 1},
        {"skip-functions", no_argument, &opts.read_functions, 0},
        {"skip-dot",  no_argument, &long_option, opt_skip_dot},
        {"show-dot",  no_argument, &long_option, opt_show_dot},
        {"skip-tilde",  no_argument, &long_option, opt_skip_tilde},
        {"show-tilde",  no_argument, &long_option, opt_show_tilde},
        {"tty-only",  no_argument, &long_option, opt_tty_only},
        {"version",  no_argument, 0, 'v'},
        {"help",  no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while (1) {
        c = getopt_long(argc, argv, "aivVh", long_options, NULL);

        if (c == -1)
            break;

        switch (c) {
            case 0:
                switch (long_option) {
                    case opt_skip_dot:
                        opts.skip_dot = !tty_only;
                        break;
                    case opt_show_dot:
                        opts.show_dot = !tty_only;
                        break;
                    case opt_skip_tilde:
                        opts.skip_tilde = !tty_only;
                        break;
                    case opt_show_tilde:
                        opts.show_tilde = !tty_only;
                        break;
                    case opt_tty_only:
                        tty_only = !isatty(1);
                        break;
                }
                break;
            case 'a':
                opts.all = 1;
                break;
            case 'i':
                opts.read_aliases = 1;
                break;
            case 'v':
            case 'V':
                version();
                return 1;
            case 'h':
                usage();
                return 1;
        }
    }

    return 0;
}


void usage()
{
    cout << "Usage: which [options] [--] COMMAND [...]" << endl;
    cout << "Write the full path of COMMAND(s) to standard output." << endl;
    cout << "" << endl;
    cout << "  --version, -[vV] Print version and exit successfully." << endl;
    cout << "  --help,          Print this help and exit successfully." << endl;
    cout << "  --skip-dot       Skip directories in PATH that start with a dot." << endl;
    cout << "  --skip-tilde     Skip directories in PATH that start with a tilde." << endl;
    cout << "  --show-dot       Don't expand a dot to current directory in output." << endl;
    cout << "  --show-tilde     Output a tilde for HOME directory for non-root." << endl;
    cout << "  --tty-only       Stop processing options on the right if not on tty." << endl;
    cout << "  --all, -a        Print all matches in PATH, not just the first" << endl;
    cout << "  --read-alias, -i Read list of aliases from stdin." << endl;
    cout << "  --skip-alias     Ignore option --read-alias; don't read stdin." << endl;
    cout << "  --read-functions Read shell functions from stdin." << endl;
    cout << "  --skip-functions Ignore option --read-functions; don't read stdin." << endl;
    cout << "" << endl;
    cout << "Recommended use is to write the output of (alias; declare -f) to standard" << endl;
    cout << "input, so that which can show aliases and shell functions. See which(1) fo" << endl;
    cout << "examples." << endl;
    cout << "" << endl;
    cout << "If the options --read-alias and/or --read-functions are specified then the" << endl;
    cout << "output can be a full alias or function definition, optionally followed by" << endl;
    cout << "the full path of each command used inside of those." << endl;

}

void version()
{
    cout << "which v1.0, Copyright (C) 2014 Nathan Hoad" << endl;
}


void split(const std::string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

int find_paths(string filename)
{
    char *path;
    vector<string> dirs;
    int failed;

    for (auto &alias : aliases) {
        if (alias.find(filename) == 0) {
            cout << alias << endl;
            if (opts.all == 0)
                return 0;
        }
    }

    for (auto &function : functions) {
        if (function.find(filename) == 0) {
            cout << function << endl;
            if (opts.all == 0)
                return 0;
        }
    }

    path = getenv("PATH");
    failed = 1;
    split(path ? path : "", ':', dirs);

    for (auto &dir: dirs) {
        // clean up paths so we don't render double slashes
        while(dir.rbegin() != dir.rend() && *dir.rbegin() == '/')
            dir.pop_back();

        string fullpath = dir + "/" + filename;
        string printed_path;

        struct stat sb;
        if ((stat(fullpath.c_str(), &sb) == 0) && sb.st_mode & S_IXUSR) {
            if (opts.show_dot && dir[0] == '.' && fullpath.compare(0, cwd.size(), cwd) == 0) {
                printed_path = string("./") + filename;
            } else {
                if (opts.show_tilde && !home.empty() && fullpath.compare(0, home.size(), home) == 0) {
                    fullpath = fullpath.substr(home.size(), fullpath.size()-home.size());
                    printed_path = string("~") + fullpath;
                } else {
                    printed_path = fullpath;
                }
            }

            if (printed_path.size()) {
                if (opts.skip_tilde && printed_path[0] == '~' && geteuid() != 0) {
                    continue;
                } else if (opts.skip_dot && printed_path[0] == '.') {
                    continue;
                }
                cout << printed_path << endl;
                failed = 0;
                if (opts.all == 0)
                    break;
            }
        }
    }

    return failed;
}

void read_aliases_and_functions()
{
    if (!opts.read_functions && !opts.read_aliases)
        return;

    if (isatty(0)) {
        cerr << "which: warning: stdin is a tty" << endl;
    }

    string line, function;

    while (std::getline(cin, line, '\n')) {
        if (opts.read_aliases && regex_match(line, regex("^.+=.+?$"))) {
            aliases.push_back(line);
        } else if (opts.read_functions) {
            function = line + "\n";
            while (std::getline(cin, line, '\n') && line != "}") {
                function += line + "\n";
            }

            if (line == "}") {
                function += "}\n";
                functions.push_back(function);
                function = "";
            }
        } else {
            cerr << "which: line doesn't appear to be an alias and functions are disabled " << line << endl;
            exit(EXIT_FAILURE);
        }
    }
}

void setup_from_env() {
    char *_home;
    char _cwd[PATH_MAX];

    _home = getenv("HOME");
    home = _home ? _home : "";

    memset(_cwd, 0, PATH_MAX);
    if (getcwd(_cwd, PATH_MAX) == NULL) {
        cerr << "which: unable to retrieve current working directory " << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }

    cwd = _cwd;
}


int main(int argc, char *argv[])
{
    int failed;

    if (process_opts(argc, argv) != 0) {
        return 1;
    }

    setup_from_env();
    read_aliases_and_functions();

    failed = EXIT_SUCCESS;

    for (int i = optind; i < argc; i++) {
        if (find_paths(argv[i]) != 0) {
            cout << argv[i] << " not found" << endl;
            failed = EXIT_FAILURE;
        }
    }

    return failed;
}
