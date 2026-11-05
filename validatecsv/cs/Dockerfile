# C# CSV Validator Docker Image
FROM mcr.microsoft.com/dotnet/sdk:8.0-alpine AS build
WORKDIR /src

# Copy project file and restore dependencies
COPY validatecsv.csproj ./
RUN dotnet restore

# Copy source code and build
COPY validatecsv.cs ./
RUN dotnet publish -c Release -o /app/publish --no-restore

# Runtime image
FROM mcr.microsoft.com/dotnet/runtime:8.0-alpine
WORKDIR /data

# Copy the published application
COPY --from=build /app/publish /usr/local/bin/

# Set the entrypoint
ENTRYPOINT ["dotnet", "/usr/local/bin/validatecsv.dll"]
