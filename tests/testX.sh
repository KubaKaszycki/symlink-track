#!/bin/sh
set -ex

cleanup () {
  rm -rf tmp.$$
}

cleanup

unset LC_ALL
unset LC_MESSAGES

LC_ALL=C
LC_MESSAGES=C

${SYMLINK_TRACK} ./X0 > tmp.$$

cmp tmp.$$ - <<_EOF_
./X0 -> ./X1 -> ./X2 -> ./X3 -> ./X4 -> ./X5 -> ./X6 -> ./X7 -> ./X8 -> ./X9 -> ./X10 (regular file)
_EOF_

cleanup
exit 0
