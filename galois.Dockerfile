FROM ubuntu:24.04 AS build

ENV DEBIAN_FRONTEND=noninteractive
ENV llvm_version=16.0.0
ENV lib_deps="cmake g++ gcc git zlib1g-dev libncurses-dev libtinfo6 build-essential libssl-dev libpcre2-dev zip libzstd-dev"
ENV build_deps="wget xz-utils git tcl software-properties-common"
RUN apt-get update
RUN apt-get install -y $build_deps $lib_deps sccache

COPY . /opt/svf
RUN cd /opt/svf && \
  bash -exo pipefail ./build.sh && \
  rm -rf /opt/svf/Release-build/svf && \
  ln -s $PWD/z3.obj/bin/libz3.so $PWD/z3.obj/bin/libz3.so.4

FROM ubuntu:24.04
COPY --from=build /opt/svf /opt/svf
RUN /opt/svf/galois-setup-svf.sh

