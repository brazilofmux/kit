# Multi-stage Alpine build for QR Tape

# Stage 1: Build
FROM alpine:3.19 AS builder

RUN apk add --no-cache \
    g++ \
    make \
    zlib-dev \
    libqrencode-dev \
    zbar-dev

WORKDIR /build

# Copy source
COPY librs/ librs/
COPY src/qrtape/ src/qrtape/

# Build librs (clean first to ensure fresh build)
RUN cd librs && rm -f *.o *.a && make

# Build qrtape tools (clean first to ensure fresh build)
RUN cd src/qrtape && rm -f *.o qrtapemake qrtaperead && make

# Stage 2: Runtime
FROM alpine:3.19

RUN apk add --no-cache \
    libstdc++ \
    zlib \
    libqrencode \
    zbar \
    poppler-utils \
    imagemagick \
    ghostscript \
    bash \
    fontconfig \
    ttf-dejavu

WORKDIR /app

# Copy binaries
COPY --from=builder /build/src/qrtape/qrtapemake /app/
COPY --from=builder /build/src/qrtape/qrtaperead /app/

# Copy scripts
COPY src/qrtape/layout_tape.sh /app/
COPY src/qrtape/read_pdf.sh /app/

# Make scripts executable
RUN chmod +x /app/*.sh

# Create working directory for user data
WORKDIR /data

# Set PATH to include /app
ENV PATH="/app:${PATH}"

# Default command
CMD ["bash"]
