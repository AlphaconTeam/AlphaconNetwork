#!/bin/sh

TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
SRCDIR=${SRCDIR:-$TOPDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

ALPHACOND=${ALPHACOND:-$SRCDIR/alphacond}
ALPHACONCLI=${ALPHACONCLI:-$SRCDIR/alphacon-cli}
ALPHACONTX=${ALPHACONTX:-$SRCDIR/alphacon-tx}
ALPHACONQT=${ALPHACONQT:-$SRCDIR/qt/alphacon-qt}

[ ! -x $ALPHACOND ] && echo "$ALPHACOND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
ALPVER=($($ALPHACONCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for alphacond if --version-string is not set,
# but has different outcomes for alphacon-qt and alphacon-cli.
echo "[COPYRIGHT]" > footer.h2m
$ALPHACOND --version | sed -n '1!p' >> footer.h2m

for cmd in $ALPHACOND $ALPHACONCLI $ALPHACONTX $ALPHACONQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${ALPVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${ALPVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
