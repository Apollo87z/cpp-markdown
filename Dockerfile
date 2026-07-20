FROM ubuntu:24.04 AS build

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    librdkafka-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build -DCMAKE_CXX_STANDARD=20 -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target conversion-service --parallel


FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    libboost-regex-dev \
    librdkafka1 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /src/build/conversion-service .

EXPOSE 8080
CMD ["./conversion-service"]