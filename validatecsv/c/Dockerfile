# C CSV Validator Docker Image (Ragel-generated)
FROM alpine:latest AS build
WORKDIR /src

# Install build dependencies (Ragel + toolchain)
RUN apk add --no-cache build-base ragel

# Copy Ragel spec
COPY validatecsv.rl ./

# Generate parser and compile
RUN ragel -G2 -o validatecsv.c validatecsv.rl \
 && cc -O3 -std=c11 -o validatecsv validatecsv.c

# Runtime image
FROM alpine:latest
WORKDIR /data

# Copy binary
COPY --from=build /src/validatecsv /usr/local/bin/

ENTRYPOINT ["/usr/local/bin/validatecsv"]
