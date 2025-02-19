#!/bin/bash

set -x

echo nacho

/usr/sbin/sshd -D &

exec "$@"
