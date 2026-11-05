# C++ CSV Validator Docker Image (Ragel-generated)
FROM alpine:latest AS build
WORKDIR /src

# Install build dependencies (Ragel + toolchain)
RUN apk add --no-cache g++ ragel

# Copy Ragel spec
COPY validatecsv.rl ./

# Generate parser and compile with optimizations and threading support
RUN ragel -G2 -o validatecsv.cpp validatecsv.rl \
 && g++ -O3 -std=c++17 -pthread -o validatecsv validatecsv.cpp

# Runtime image
FROM alpine:latest
WORKDIR /data

# Install runtime dependencies
RUN apk add --no-cache libstdc++ libgcc

# Copy the compiled binary
COPY --from=build /src/validatecsv /usr/local/bin/

# Set the entrypoint
ENTRYPOINT ["/usr/local/bin/validatecsv"]
