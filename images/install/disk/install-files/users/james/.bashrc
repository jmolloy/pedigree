export PATH=/applications
export TERM=xterm
export RUBYLIB=/libraries/ruby

if [ "$TERM" != "dumb" ]; then
    eval "`dircolors -b`"
    alias ls='ls --color=auto'
fi

export PS1="\[\e[34m\][\t]\[\e[0m\] \[\e[34;1m\]\u\[\e[0m\]\[\e[36m\]@\[\e[36;1m\]\h\[\e[0m\] \w\n$ "

echo "Welcome back, James!"
