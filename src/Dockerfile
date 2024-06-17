FROM ubuntu:20.04

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    git \
    cmake \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswresample-dev \
    libswscale-dev \
    libavfilter-dev \
    libx264-dev \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get install -y ffmpeg

WORKDIR /app
COPY . .
CMD [ "bash" ]

