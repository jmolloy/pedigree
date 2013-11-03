#!/usr/bin/env python
#
# Copyright (c) 2011 Matthew Iselin
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

# Provides tools for management of users in a target Pedigree image. Can be used
# to create users for servers/daemons, or to create a user for the builder to
# use when logging in to Pedigree.

import os, sqlite3, getpass

scriptdir = os.path.dirname(os.path.realpath(__file__))
imagedir = os.path.realpath(scriptdir + "/../images/local")
usersdir = os.path.join(imagedir, "users")
configdb = os.path.realpath(scriptdir + "/../build/config.db") # TODO: build dir is not always 'build'

# Try to connect to the database
try:
    conn = sqlite3.connect(configdb)
except:
    print "Configuration database is not available. Please run 'scons build/config.db'."
    exit(2)

# Check for an existing users directory
if not os.path.exists(usersdir):
    os.mkdir(usersdir)

def ignore():
    pass

def getnextuid():
    q = conn.execute("select uid from users order by uid desc limit 1")
    u = q.fetchone()
    return u[0] + 1

def safe_input():
    try:
        line = raw_input("> ")
    except KeyboardInterrupt:
        print # a newline
        conn.close()
        exit(1)
    return line

def main_menu(funcmap):
    print "Select an option below:"
    print "1. Add a new user"
    print "2. Change the password of an existing user"
    print "3. Change attributes of an existing user"
    print "4. Delete user"
    print "5. Exit"
    
    option = 0
    while True:
        line = safe_input()
        
        try:
            option = int(line)
        except ValueError:
            print "Please enter a number."
            continue
        
        if not option in funcmap.keys():
            print "Please enter a valid option."
            continue
        
        break
    
    funcmap[option]()

userattrs = [["fullname", "New User", False], ["groupname", "users", False], ["homedir", "/users/user", False], ["shell", "/applications/bash", False], ["password", "", True]]

def adduser():
    print "Please type the username for the new user."
    username = safe_input()
    
    # Make sure the user does not exist.
    q = conn.execute("select uid from users where username=?", [username])
    user = q.fetchone()
    if not user is None:
        print "User '%s' already exists." % (username,)
        return
    
    uid = getnextuid()
    newattrs = [uid, username]
    
    # Get all attributes
    for attr in userattrs:
        if attr[2]:
            while True:
                # Secure.
                a = getpass.getpass("%s: " % (attr[0],))
                b = getpass.getpass("Confirm %s: " % (attr[0],))
                
                if not a == b:
                    print "Passwords do not match."
                else:
                    newattrs += [a]
                    break
        else:
            data = raw_input("%s [default=%s]: " % (attr[0], attr[1]))
            if data == "":
                data = attr[1]
            newattrs += [data]
    
    # Insert.
    conn.execute("insert into users (uid, username, fullname, groupname, homedir, shell, password) values (?, ?, ?, ?, ?, ?, ?)", newattrs)
    conn.commit()
    
    # Home directory.
    homedir = newattrs[4][1:]
    os.mkdir(os.path.join(imagedir, homedir))
    
    print "Created user '%s'" % (username,)

def validuser(username):
    # Check for a valid user in the database.
    q = conn.execute("select uid, password from users where username=?", [username])
    user = q.fetchone()
    if user is None:
        print "The user '%s' does not exist." % (username,)
        return False
    return True

def changepassword():
    print "Please type the username of the user for which you want to change password."
    username = safe_input()
    
    # Check for a valid user in the database.
    if not validuser(username):
        return

    # Now grab the user object.
    q = conn.execute("select uid from users where username=?", [username])
    user = q.fetchone()
    
    # Confirm the password
    print "Changing password for '%s'..." % (username,)
    curr = getpass.getpass("Current password: ")
    if curr == user[1]:
        new = getpass.getpass("New password: ")
        
        # Commit to the DB
        conn.execute("update users set password=? where uid=?", [new, user[0]])
        conn.commit()
    else:
        print "Incorrect password."
    
    print "Changed password for user '%s'" % (username,)

def changeattr():
    print "Please type the username of the user for which you want to change attributes."
    username = safe_input()
    
    # Check for a valid user in the database.
    if not validuser(username):
        return
    
    newattrs = []

    def dict_factory(cursor, row):
        d = {}
        for idx, col in enumerate(cursor.description):
            d[col[0]] = row[idx]
        return d
    old_factory = conn.row_factory
    conn.row_factory = dict_factory
    
    q = conn.execute("select uid, fullname, groupname, homedir, shell from users where username=?", [username])
    user = q.fetchone()
    
    # Get all attributes
    n = 0
    for attr in userattrs:
        if not attr[2]:
            data = raw_input("%s [current=%s]: " % (attr[0], user[attr[0]]))
            if data == "":
                data = user[attr[0]]
            newattrs += [data]
        
        n += 1
    
    newattrs += [user["uid"]]
    
    # Update.
    conn.execute("update users set fullname=?, groupname=?, homedir=?, shell=? where uid=?", newattrs)
    conn.commit()
    
    # Create home directory in case it changed.
    homedir = newattrs[2]
    if True or (not homedir == user["homedir"]):
        oldhome = os.path.join(imagedir, user["homedir"][1:])
        newhome = os.path.join(imagedir, homedir[1:])
        if not os.path.exists(oldhome):
            os.mkdir(newhome)
        else:
            os.rename(oldhome, newhome)
    
    conn.row_factory = old_factory

def deleteuser():
    print "Please type the username for the user to delete."
    username = safe_input()
    
    # Check for a valid user in the database.
    if not validuser(username):
        return

    # Now grab the user object.
    q = conn.execute("select uid from users where username=?", [username])
    user = q.fetchone()
    
    # Delete the user.
    conn.execute("delete from users where uid=?", [user[0]])
    conn.commit()
    
    print "Deleted user '%s'" % (username,)

options = {1 : adduser, 2 : changepassword, 3 : changeattr, 4 : deleteuser, 5 : ignore}

main_menu(options)

conn.close()


