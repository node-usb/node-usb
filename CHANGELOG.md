# Changelog

## [2.14.0] - 2024-09-15

### Fixed
- Fixed fatal exceptions by using `ThrowAsJavaScriptException` instead of `Napi::Error::Fatal` - [`817`](https://github.com/node-usb/node-usb/pull/817) ([cleoo](https://github.com/cleoo))

## [2.13.0] - 2024-05-26

### Changed
- Updated libusb to v1.0.27 - [`773`](https://github.com/node-usb/node-usb/pull/773) ([Rob Moran](https://github.com/thegecko))

### Added
- Added nullchecks and errorchecks in descriptor parsing - [`764`](https://github.com/node-usb/node-usb/pull/764) ([Seth Foster](https://github.com/sfoster1))

## [2.12.1] - 2024-03-09

### Changed
- Rolled back prebuildify to 5.0.1 to avoid naming issue - [`744`](https://github.com/node-usb/node-usb/pull/744) ([Rob Moran](https://github.com/thegecko))

## [2.12.0] - 2024-03-02

### Added
- Added prebuilt binary for Windows Arm64 - [`735`](https://github.com/node-usb/node-usb/pull/735) ([Rob Moran](https://github.com/thegecko))
- Added exception handling - [`738`](https://github.com/node-usb/node-usb/pull/738) ([Rob Moran](https://github.com/thegecko))
- Added exception when trying to transfer and device is closed - [`715`](https://github.com/node-usb/node-usb/pull/715) ([Rob Moran](https://github.com/thegecko))

### Changed
- Precreate async transfer functions to increase speed - [`725`](https://github.com/node-usb/node-usb/pull/725) ([Cosmin Tanislav](https://github.com/Demon000))
- Precreate all other async functions to increase speed - [`730`](https://github.com/node-usb/node-usb/pull/730) ([Rob Moran](https://github.com/thegecko))

### Fixed
- Explicitly set configuration in WebUSB for vendor-specific devices on macos - [`739`](https://github.com/node-usb/node-usb/pull/739) ([Jouni Airaksinen](https://github.com/jounii))
- Handle exception when loading library on WSL - [`726`](https://github.com/node-usb/node-usb/pull/726) ([Julian Waller](https://github.com/Julusian))
- Comment typo fix - [`723`](https://github.com/node-usb/node-usb/pull/723) ([koji](https://github.com/koji))

## [2.11.0] - 2023-10-02

### Fixed
- Allow use of close on a closed device - [`676`](https://github.com/node-usb/node-usb/pull/676) ([Julien Vanier](https://github.com/monkbroc))

## [2.10.0] - 2023-08-22

### Fixed
- Fixed using callbacks in startPolling - [`631`](https://github.com/node-usb/node-usb/pull/631) ([Tim Strazzere](https://github.com/strazzere))

## [2.9.0] - 2023-03-16

### Changed
- Updated node-addon-api to v6.0 - [`581`](https://github.com/node-usb/node-usb/pull/581) ([Maarten Bent](https://github.com/MaartenBent))

## [2.8.3] - 2023-03-09

### Fixed
- Fixed Windows X86 crash with USB detection - [`578`](https://github.com/node-usb/node-usb/pull/578) ([Julian Waller](https://github.com/Julusian))
- Fixed Windows USB detection race condition with delay - [`577`](https://github.com/node-usb/node-usb/pull/577) ([Rob Moran](https://github.com/thegecko))

## [2.8.2] - 2023-03-06

### Added
- Added new `usb.pollHotplug` setting to allow users to force hotplug detection using polling - [`576`](https://github.com/node-usb/node-usb/pull/576) ([Rob Moran](https://github.com/thegecko))

## [2.8.1] - 2023-02-26

### Changed
- Reworked WebUSB options.allowedDevices to allow pre-authorisation using any filter - [`574`](https://github.com/node-usb/node-usb/pull/574) ([Rob Moran](https://github.com/thegecko))
- Updated requestDevice errors to use correct names - [`575`](https://github.com/node-usb/node-usb/pull/575) ([Rob Moran](https://github.com/thegecko))

## [2.8.0] - 2023-02-11

### Changed
- Minor tweaks to avoid some race conditions - [`569`](https://github.com/node-usb/node-usb/pull/569) ([Rob Moran](https://github.com/thegecko))

## [2.7.0] - 2023-01-25

### Fixed
- Return same WebUSB device on subsequent calls - [`567`](https://github.com/node-usb/node-usb/pull/567) ([Nisarg Jhaveri](https://github.com/nisargjhaveri))

## [2.6.0] - 2022-12-10

### Changed
- Updated build dependencies and now using Python 3.x for builds (shouldn't affect package) - [`563`](https://github.com/node-usb/node-usb/pull/563) ([Rob Moran](https://github.com/thegecko))

## [2.5.3] - 2022-12-06

### Changed
- Allow multiple polling to fail - [`556`](https://github.com/node-usb/node-usb/pull/556) ([Tim Strazzere](https://github.com/strazzere))
- Ignore creation issues during attach and list - [`550`](https://github.com/node-usb/node-usb/pull/550) ([Rob Moran](https://github.com/thegecko))

## [2.5.2] - 2022-09-24

### Fixed
- Allow errors during WebUSBDevice creation to be thown - [`549`](https://github.com/node-usb/node-usb/pull/549) ([Rob Moran](https://github.com/thegecko))
- Ensure cmNotifyFilter is kept alive for the duration of the hotplug detetion on Windows - [`541`](https://github.com/node-usb/node-usb/pull/541) ([Julian Waller](https://github.com/Julusian))

### Changed
- Export all types at the top level of the package - [`544`](https://github.com/node-usb/node-usb/pull/544) ([Rob Moran](https://github.com/thegecko))
- Trimmed TypeScript source from release package - [`536`](https://github.com/node-usb/node-usb/pull/536) ([Rob Moran](https://github.com/thegecko))

## [2.5.1] - 2022-08-29

### Fixed
- USB device plug/unplug detection on Windows when devce already attached - [`531`](https://github.com/node-usb/node-usb/pull/531) ([Rob Moran](https://github.com/thegecko))
- Removed dependency on yarn in package.json scripts - [`535`](https://github.com/node-usb/node-usb/pull/535) ([Rob Moran](https://github.com/thegecko))

## [2.5.0] - 2022-07-30

### Fixed
- Native USB device plug/unplug detection on Windows - [`524`](https://github.com/node-usb/node-usb/pull/524) ([Julian Waller](https://github.com/Julusian))

## [2.4.3] - 2022-06-21

### Fixed
- Fixed poll transfer tracking - [`522`](https://github.com/node-usb/node-usb/pull/522) ([Rob Moran](https://github.com/thegecko))

## [2.4.2] - 2022-05-27

### Fixed
- Fixed multiple events with device detection on Windows - [`512`](https://github.com/node-usb/node-usb/pull/512) ([Alba Mendez](https://github.com/mildsunrise))

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
