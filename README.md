<p align="center"><a href="https://platformio.org" target="_blank"><img src="https://piolabs.com/assets/img/platformio-labs-logo-horizontal-white.png" width="300" alt="PlatformIO Labs Logo"></a></p>

## Luminate

Luminate is a PlatformIO-based dual-tone white LED driver that smoothly fades lighting from cool white to warm white as the day goes by.

- Uses PWM to control the LEDs brightness.
- Uses DLS to calculate the LEDs net brightness.
- Uses DTS to adjust the cooling FANs speed.
- Uses RTC to calculate the LEDs color ratio.
- Uses RTC to calculate the cooling FAN speed.

## Dependencies

Make sure these softwares are installed:

1. [Python](https://www.python.org/downloads/): Required for PlatformIO core to run.

2. [Visual Studio Code](https://code.visualstudio.com): Required for PlatformIO core to run.

3. [PlatformIO IDE](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide): Required for the project to compile.

4. [Node.js](https://nodejs.org/en/download): Required for the html to compile.

5. [HTMLMinifier](https://github.com/kangax/html-minifier): Required for the html to compile.

## Deployment

Follow these steps to setup the project:

1. VS Code > PlatformIO > Open Project > Luminate.

2. Menu > Terminal > New Terminal.

3. Execute `./scripts/build.sh -fs -fw`.

## Documentation

Follow these documents to stay informed:

1. This project is licensed under the terms of the [MIT License](LICENSE).

2. All notable changes to this project will be documented in the [CHANGELOG](CHANGELOG.md) file.

3. All known issues, bugs, and feature requests are tracked in the [Issues](https://github.com/iarghadip/luminate/issues) section.