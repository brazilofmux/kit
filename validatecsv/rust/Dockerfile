# Rust CSV Validator Docker Image
# Built on Alpine, whose native target is musl on every architecture, so the
# binary is static without naming a target triple: the same Dockerfile builds
# natively on amd64 and arm64. (It used to name x86_64-unknown-linux-musl,
# which made the image amd64-only.)
FROM rust:alpine AS builder

WORKDIR /app

# musl-dev supplies the C runtime objects the linker needs
RUN apk add --no-cache musl-dev

# Copy source files
COPY . .

# Build the Rust binary (statically linked: musl is the native target)
RUN cargo build --release

# Use a minimal Alpine runtime
FROM alpine:latest

WORKDIR /data

# Copy the statically linked binary
COPY --from=builder /app/target/release/validatecsv /usr/local/bin/

# Set the entrypoint
ENTRYPOINT ["/usr/local/bin/validatecsv"]
