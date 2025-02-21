#!/bin/bash

set -x

/usr/sbin/sshd -D &

exec "$@"
