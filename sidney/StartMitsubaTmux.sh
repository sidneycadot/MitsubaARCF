#! /bin/sh

exec tmux new-session -s mitsuba "source $HOME/Repositories/github/MitsubaARCF/sidney/mitsuba-current/setpath.sh ; while true ; do mtssrv ; sleep 20 ; done"
