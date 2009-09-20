export PATH=/applications
export TERM=xterm
export RUBYLIB=/libraries/ruby
export PYTHONHOME=/

if [ "$TERM" != "dumb" ]; then
    eval "`dircolors -b`"
    alias ls='ls --color=auto'
fi

export PS1="[\d \T \@ \u on \h] \w \n > "

echo "Welcome to Pedigree, Matt!"
echo

