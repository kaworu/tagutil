#!/bin/sh

case "`uname`" in
    *BSD)   command="fetch -o -" ;;
    *)      command="wget  -O -" ;;
esac

$command "http://musicbrainz.org/ws/1/track/?type=xml&query=\"$1\"&limit=${2:-"10"}"
