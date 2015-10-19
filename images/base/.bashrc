case "$TERM" in
    xterm) export TERM=xterm-256color;;
esac

if [ -x /applications/dircolors ]; then
    alias ls='ls --color=auto'
    alias grep='grep --color=auto'
fi

# Make sure we can type UTF-8 characters (eg, altgr + >)
export LANG=en_US.UTF-8

alias ll='ls -alF'

export PS1="\[\e[34;1m\][\t]\[\e[0m\] \w \[\e[34;1m\]$\[\e[0m\] "

shopt -q login_shell && echo "Welcome to the Pedigree Operating System.

Run 'tour' for a quick run-through of Pedigree's unique features."
