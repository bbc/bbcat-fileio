#!/bin/sh
TITLE="$1"
OUTPUTFILE="$2"
TEMPFILE="_temp"
shift ; shift
echo "Factories found:"
grep -h -o -E "SELF_REGISTERING.*\([a-zA-Z0-9_]+," $@ | sed -E "s/^.+\((.+),/\1/" | sort
echo "//Auto-generated - DO NOT EDIT!" >"$TEMPFILE"
echo "//Sources: $@" >>"$TEMPFILE"
echo "BBC_AUDIOTOOLBOX_START" >>"$TEMPFILE"
grep -h -o -E "SELF_REGISTERING.*\([a-zA-Z0-9_]+," $@ | sed -E "s/^.+\((.+),/extern const RegisteredObjectFactory *factory_\1;/" | sort >>"$TEMPFILE"
echo "const RegisteredObjectFactory *factories_$TITLE[]=" >>"$TEMPFILE"
echo "{" >>"$TEMPFILE"
grep -h -o -E "SELF_REGISTERING.*\([a-zA-Z0-9_]+," $@ | sed -E "s/^.+\((.+),/factory_\1,/" | sort >>"$TEMPFILE"
echo "};" >>"$TEMPFILE"
echo "BBC_AUDIOTOOLBOX_END" >>"$TEMPFILE"
test ! -f "$OUTPUTFILE" -o "$TEMPFILE" -nt "$OUTPUTFILE" && cp "$TEMPFILE" "$OUTPUTFILE"
