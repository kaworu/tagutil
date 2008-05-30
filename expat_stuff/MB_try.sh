#!/bin/sh

wget -O - "http://musicbrainz.org/ws/1/track/?type=xml&query=\"$1\" AND artist:\"$2\"&limit=${3:-10}"
