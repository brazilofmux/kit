FROM --platform=$BUILDPLATFORM mcr.microsoft.com/dotnet/sdk:8.0-alpine AS build
ARG TARGETARCH
RUN if [ "$TARGETARCH" = "amd64" ]; then echo x64 > /tmp/dotnet_arch; else echo "$TARGETARCH" > /tmp/dotnet_arch; fi

WORKDIR /app

# Copy everything
COPY . .

# Restore and publish for target architecture
RUN dotnet restore -a $(cat /tmp/dotnet_arch)
RUN dotnet publish -a $(cat /tmp/dotnet_arch) -c Release -o out

# Runtime stage
FROM --platform=$TARGETPLATFORM mcr.microsoft.com/dotnet/runtime:8.0-alpine

WORKDIR /app

# Copy the built application
COPY --from=build /app/out .

# Create a directory for data files
RUN mkdir -p /data

# Set the entry point
ENTRYPOINT ["dotnet", "PrescriptionTracker.dll"]
