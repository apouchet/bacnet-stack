# BACnet Client Docker Image
# Build and run the BACnet client in a container

FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    libconfig-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy source
WORKDIR /build
COPY . .

# Build library and client
RUN make LEGACY=true -C apps lib
RUN cd client && make

# Runtime image
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libconfig9 \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN useradd -r -s /bin/false bacnet

# Copy binary
COPY --from=builder /build/client/bin/bacnet-client /usr/local/bin/

# Set permissions
RUN chmod +x /usr/local/bin/bacnet-client

# Switch to non-root user
USER bacnet

# Default environment
ENV BACNET_IP_PORT=47808

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD bacnet-client --version || exit 1

# Default command
ENTRYPOINT ["bacnet-client"]
CMD ["--help"]
