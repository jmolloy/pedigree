export PATH=/applications

case "$TERM" in
    xterm) export TERM=xterm-256color;;
esac

if [ -x /applications/dircolors ]; then
    alias ls='ls --color=auto'
    alias grep='grep --color=auto'
fi

alias ll='ls -alF'

export PS1="\[\e[34;1m\][\t]\[\e[0m\] \w \[\e[34;1m\]$\[\e[0m\] "

echo "Welcome to the Pedigree operating system.

Please don't feed the code monkeys - they bite."
