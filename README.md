# 240212 S88 naar ESP-NOW

## Doel

ESP32-prototype dat vier S88-modules inleest, de toestand op een OLED toont en de gegevens via ESP-NOW verzendt.

## Werking volgens `src/main.cpp`

- Leest 16 bits per S88-module in met `ReadS88()`.
- Toont de bitpatronen wanneer de bootknop wordt ingedrukt.
- Verzendt bij die knopactie een ESP-NOW-pakket naar het MAC-adres dat in de broncode is ingesteld.
- Het pakket bevat `ID`, `OFFSET`, `AANTAL` en vier `uint16_t`-woorden.
- De code initialiseert de datawoorden bij het opstarten ook met vaste testpatronen.

## Hardware en bouwen

- PlatformIO-target: `esp32doit-devkit-v1`
- S88-clock: GPIO 23
- S88-load: GPIO 18
- S88-data-ingang: GPIO 19
- Bootknop: GPIO 0 (naar GND)
- OLED I2C: SDA GPIO 22, SCL GPIO 21
- Seriële debug: 115200 baud

Bouwen vanuit deze projectmap:

```sh
pio run
```

## Protocolnotitie

Dit is een oudere packetindeling met ruwe `uint16_t`-woorden. Die verschilt van de nieuwere S88-host in `240518S88Host2ESPNow`, die de moduledata als MSB-first bytes verstuurt. Gebruik deze firmware daarom niet zonder protocolcontrole als zender voor `251111HostESPNowWEB`.
