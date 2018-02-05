#!/bin/bash

if [ "$1" = "PLAY" ]; then
    echo "  $2" > /home/mateusz/work/bass-player/song_info
elif [ "$1" = "END" ]; then
    echo "" > /home/mateusz/work/bass-player/song_info
else
    echo "Not equal"
fi
