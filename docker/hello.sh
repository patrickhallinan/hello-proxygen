#!/bin/bash

set -x

/hello-proxygen/build/hello/hello &
/hello-proxygen/build/test/test &

exec "$@"
