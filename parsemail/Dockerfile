#See https://aka.ms/containerfastmode to understand how Visual Studio uses this Dockerfile to build your images for faster debugging.

FROM --platform=$TARGETPLATFORM mcr.microsoft.com/dotnet/runtime:8.0-alpine AS base
WORKDIR /app

FROM --platform=$BUILDPLATFORM mcr.microsoft.com/dotnet/sdk:8.0-alpine AS build
ARG TARGETARCH
RUN if [ "$TARGETARCH" = "amd64" ]; then echo x64 > /tmp/dotnet_arch; else echo "$TARGETARCH" > /tmp/dotnet_arch; fi
WORKDIR /src
COPY ["parsemail/parsemail.csproj", "parsemail/"]
RUN dotnet restore -a $(cat /tmp/dotnet_arch) "parsemail/parsemail.csproj"
COPY . .
WORKDIR "/src/parsemail"
RUN dotnet publish -a $(cat /tmp/dotnet_arch) "parsemail.csproj" -c Release -o /app/publish

FROM base AS final
WORKDIR /app
COPY --from=build /app/publish .
ENTRYPOINT ["dotnet", "parsemail.dll"]
