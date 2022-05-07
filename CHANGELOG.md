# Changelog

## [2.4.1] - 2022-05-07

### Fixed
- Fixed node engine ranges in package.json - ([Rob Moran](https://github.com/thegecko))

## [2.4.0] - 2022-05-02

### Fixed
- Made addon context aware - [`512`](https://github.com/node-usb/node-usb/pull/512) ([Rob Moran](https://github.com/thegecko) & [Alba Mendez](https://github.com/mildsunrise))
- Fixed deprecation of `libusb_set_option` - [`510`](https://github.com/node-usb/node-usb/pull/510) ([Rob Moran](https://github.com/thegecko))

### Changed
- **Breaking:** Updated to N-API version 6 (Node >=10.20.0 <11.x, >=12.17.0 <13.0, >=14.0.0) - [`509`](https://github.com/node-usb/node-usb/pull/509) ([Rob Moran](https://github.com/thegecko))

### Added
- Added `rebuild` command - [`511`](https://github.com/node-usb/node-usb/pull/511) ([Rob Moran](https://github.com/thegecko))

### Removed
- Removed `USE_POLL` build definition to simplify code - [`507`](https://github.com/node-usb/node-usb/pull/507) ([Rob Moran](https://github.com/thegecko))

## [2.3.1] - 2022-04-11

### Changed
- Removed device access mutex from WebUSB API - [`501`](https://github.com/node-usb/node-usb/pull/501) ([Rob Moran](https://github.com/thegecko))

## [2.3.0] - 2022-04-11

### Changed
- Changed libusb dependency to upstream v1.0.26 - [`505`](https://github.com/node-usb/node-usb/pull/505) ([Rob Moran](https://github.com/thegecko))
- Cleaned up Windows device polling method - [`496`](https://github.com/node-usb/node-usb/pull/496) ([Alba Mendez](https://github.com/mildsunrise))

## [2.2.0] - 2022-03-25

### Added
- Added `useUsbDkBackend()` method for switching libusb backend on Windows - [`489`](https://github.com/node-usb/node-usb/pull/489) ([Rob Moran](https://github.com/thegecko))

### Changed
- Changed libusb dependency to upstream v1.0.25 - [`490`](https://github.com/node-usb/node-usb/pull/490) ([Rob Moran](https://github.com/thegecko))

### Fixed
- Shortcut getStringDescriptor() to return undefined when passed index 0 - [`487`](https://github.com/node-usb/node-usb/pull/487) ([Rob Moran](https://github.com/thegecko))
- Fixed race issue with garbage collected devices - [`492`](https://github.com/node-usb/node-usb/pull/492) ([Alba Mendez](https://github.com/mildsunrise))

## [2.1.3] - 2022-03-18

### Fixed
- Fixed update to @types/w3c-web-usb - [`485`](https://github.com/node-usb/node-usb/pull/485) ([Rob Moran](https://github.com/thegecko))

## [2.1.2] - 2022-02-09

### Fixed
- Fixed error being thrown when searching for interface endpoint - [`479`](https://github.com/node-usb/node-usb/pull/479) ([Rob Moran](https://github.com/thegecko))

## [2.1.1] - 2022-02-07

### Fixed
- Fixed prebuildify tags which lead to missing binaries - [`477`](https://github.com/node-usb/node-usb/pull/477) ([Rob Moran](https://github.com/thegecko))

## [2.1.0] - 2022-01-29

### Added
- Added `getWebUsb()` convenience method - [`467`](https://github.com/node-usb/node-usb/pull/467) ([Rob Moran](https://github.com/thegecko))
- Added initialize() error - [`470`](https://github.com/node-usb/node-usb/pull/470) ([Gerrit Niezen](https://github.com/gniezen))

### Fixed
- Fixed lack of universal binary for MacOS darwin arm64 - [`473`](https://github.com/node-usb/node-usb/pull/473) ([Rob Moran](https://github.com/thegecko))

## [2.0.3] - 2021-12-13

### Added
- Added definition for `NAPI_VERSION 4` and added N-API v4 target to prebuildify - ([Rob Moran](https://github.com/thegecko))

## [2.0.2] - 2021-12-12

### Fixed
- Fixed error with trying to close WebUSB devices after they had failed to open - ([Rob Moran](https://github.com/thegecko))

## [2.0.1] - 2021-12-11

### Added
- Added new WebUSB API - [`443`](https://github.com/node-usb/node-usb/pull/443) ([Rob Moran](https://github.com/thegecko))
- Switched to TypeScript
- Switched to yarn instead of npm
- Added docs using Typedoc

## [1.9.2] - 2021-12-05

### Fixed
- Fixed exit delay and hang by moving queue start/stop to device open/close  - [`460`](https://github.com/node-usb/node-usb/pull/460) ([MikeColeGuru](https://github.com/MikeColeGuru))

## [1.9.1] - 2021-11-14

### Fixed
- Reverted fix for exit delay fixing a segfault issue it introduced - [`456`](https://github.com/node-usb/node-usb/pull/456) ([Rob Moran](https://github.com/thegecko))

## [1.9.0] - 2021-11-08

### Changed
- Changed libusb dependency to upstream v1.0.23 - [`453`](https://github.com/node-usb/node-usb/pull/453) ([Rob Moran](https://github.com/thegecko))

## [1.8.1] - 2021-11-08

### Added
- Added functionality to ref/unref the hotplug events - [`455`](https://github.com/node-usb/node-usb/pull/455) ([Guilherme Francescon](https://github.com/gfcittolin))
- Added CHANGELOG - [`454`](https://github.com/node-usb/node-usb/pull/454) ([Rob Moran](https://github.com/thegecko))

### Fixed
- Fixed delay when exiting program - [`455`](https://github.com/node-usb/node-usb/pull/455) ([Rob Moran](https://github.com/thegecko))

## [1.8.0] - 2021-10-14

### Added
- Added prebuildify and GitHub action for native binaries - [`450`](https://github.com/node-usb/node-usb/pull/450) ([Rob Moran](https://github.com/thegecko))

### Fixed
- Fixed crash after wake on Windows when using Electron - [`451`](https://github.com/node-usb/node-usb/pull/451) ([Rob Moran](https://github.com/thegecko))
- Fixed invalid initial refs - [`445`](https://github.com/node-usb/node-usb/pull/445) ([Alba Mendez](https://github.com/mildsunrise))

### Changed
- Changed GitHub organisation to [node-usb](https://github.com/node-usb) - [`447`](https://github.com/node-usb/node-usb/pull/447) ([Rob Moran](https://github.com/thegecko))

## [1.7.2] - 2021-08-30

### Fixed
- Fixed crash when exiting on MacOS with Electron 9 - [`440`](https://github.com/node-usb/node-usb/pull/440) ([Daniel Main](https://github.com/danielmain))

### Changed
- **Breaking:** Changed minimum Node.js version to `10` - [`428`](https://github.com/node-usb/node-usb/pull/428) ([Rob Moran](https://github.com/thegecko))

## [1.7.1] - 2021-05-08

### Fixed
- Fixed compiler warnings - [`419`](https://github.com/node-usb/node-usb/pull/419) ([Joel Purra](https://github.com/joelpurra))

## [1.7.0] - 2021-04-10

### Changed
- Changed native support to use Node Addon API - [`399`](https://github.com/node-usb/node-usb/pull/399) ([Georg Vienna](https://github.com/geovie))

## [1.6.5] - 2021-02-14

### Changed
- Changed prebuild for Electron 12 beta to nightly - [`410`](https://github.com/node-usb/node-usb/pull/410) ([Piotr Rogowski](https://github.com/karniv00l))

### Removed
- Removed `portNumbers` test on arm64 - [`408`](https://github.com/node-usb/node-usb/pull/408) ([Rob Moran](https://github.com/thegecko))

## [1.6.4] - 2021-01-30

### Added
- Added prebuild for Electron 10 and 12 beta - [`407`](https://github.com/node-usb/node-usb/pull/407) ([Rob Moran](https://github.com/thegecko))
- Added prebuild for Electron 9 - [`362`](https://github.com/node-usb/node-usb/pull/362) (Luke Whyte)

### Changed
- Changed to GitHub Actions for prebuild workflow - [`404`](https://github.com/node-usb/node-usb/pull/404) ([Rob Moran](https://github.com/thegecko))
- Changed Node.js 13 prebuild target to Node.js 14 - [`374`](https://github.com/node-usb/node-usb/pull/374) ([Micah Zoltu](https://github.com/MicahZoltu))
