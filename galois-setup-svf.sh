#!/usr/bin/env bash
set -euxo pipefail
export DEBIAN_FRONTEND=noninteractive
apt-get update
# It's unclear which of these libraries are actually required by SVF.
apt-get install -y zlib1g libncurses6 libtinfo6 libpcre2-dev libzstd1 libxml2 build-essential

