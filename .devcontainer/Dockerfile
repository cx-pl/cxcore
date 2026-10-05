FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        git \
        gdb \
        python3 \
        python3-venv \
    && python3 -m venv /opt/cmake \
    && /opt/cmake/bin/pip install --no-cache-dir cmake==4.4.4 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

ENV PATH="/opt/cmake/bin:${PATH}"
