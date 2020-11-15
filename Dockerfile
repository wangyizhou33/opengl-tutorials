FROM nvidia/opengl:base

ARG DEBIAN_FRONTEND=noninteractive

RUN apt -y update && apt -y install \
freeglut3-dev clang-format build-essential cmake \
xorg-dev libglu1-mesa-dev

# Clean up APT when done.
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
