# Changelog

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
