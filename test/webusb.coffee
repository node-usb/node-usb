assert = require('assert')
util = require('util')
webusb = require('../').webusb
WebUSB = require('../').WebUSB

if typeof gc is 'function'
    # running with --expose-gc, do a sweep between tests so valgrind blames the right one
    afterEach -> gc()

describe 'WebUSB Module', ->
    it 'should describe basic constants', ->
        assert.notEqual(webusb, undefined, "webusb must be undefined")

describe 'allowedDevices', ->
    it 'should not list any devices by default', ->
        l = await webusb.getDevices()
        assert.equal(l.length, 0)

    it 'should list allowed devices', ->
        customWebusb = new WebUSB({ allowedDevices: [{ vendorId: 0x59e3 }] })
        l = await customWebusb.getDevices()
        assert.equal(l.length, 1)
        assert.notEqual(l[0], undefined)

describe 'requestDevice', ->
    it 'should return a device', ->
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
        assert.notEqual(device, undefined)

describe 'getDevices', ->
    it 'should return one device', ->
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
        l = await webusb.getDevices()
        assert.equal(l.length, 1)
        assert.notEqual(l[0], undefined)
        assert.deepEqual(l[0], device)

describe 'WebUSB Hotplug', ->
    it 'should detect disconnect', (done) ->
        fn = (e) ->
            assert.equal(e.device.serialNumber, "TEST_DEVICE")
            webusb.removeEventListener 'disconnect', fn
            done()
        webusb.addEventListener 'disconnect', fn
        console.log('\n--- DISCONNECT DEVICE ---\n')

    it 'should detect connect', (done) ->
        fn = (e) ->
            assert.equal(e.device.serialNumber, "TEST_DEVICE")
            webusb.removeEventListener 'connect', fn
            done()
        webusb.addEventListener 'connect', fn
        console.log('\n--- CONNECT DEVICE ---\n')

describe 'Device properties', ->
    device = null
    before ->
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });

    it 'should have usb version properties', ->
        assert.equal(device.usbVersionMajor, 1)
        assert.equal(device.usbVersionMinor, 1)
        assert.equal(device.usbVersionSubminor, 0)

    it 'should have device version properties', ->
        assert.equal(device.deviceVersionMajor, 1)
        assert.equal(device.deviceVersionMinor, 1)
        assert.equal(device.deviceVersionSubminor, 0)

    it 'should have class properties', ->
        assert.equal(device.deviceClass, 0)
        assert.equal(device.deviceSubclass, 0)

    it 'should have protocol property', ->
        assert.equal(device.deviceProtocol, 0)

    it 'should have vid/pid properties', ->
        assert.equal(device.vendorId, 0x59e3)
        assert.equal(device.productId, 0x0a23)

    it 'should have a single configuration', ->
        assert.equal(device.configurations.length, 1)
        assert.equal(device.configurations[0].configurationValue, 1)

    it 'should have a configuration property', ->
        assert.notEqual(device.configuration, undefined)

    it 'should have a single interface', ->
        assert.equal(device.configuration.interfaces.length, 1)
        assert.equal(device.configuration.interfaces[0].interfaceNumber, 0)

    it 'should have a single alternate', ->
        assert.equal(device.configuration.interfaces[0].alternates.length, 1)
        assert.equal(device.configuration.interfaces[0].alternates[0].alternateSetting, 0)

describe 'String descriptors', ->
    device = null
    before ->
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });

    it 'gets serialNumber string', ->
        assert.equal(device.serialNumber, "TEST_DEVICE")

    it 'gets manufacturerName string', ->
        assert.equal(device.manufacturerName, "Nonolith Labs")

    it 'gets productName string', ->
        assert.equal(device.productName, "STM32F103 Test Device")

describe 'Device access', ->
    device = null
    before ->
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });

    it 'is not open', ->
        assert.equal(device.opened, false)

    it 'is opens and closes', ->
        assert.equal(device.opened, false)
        await device.open()
        assert.equal(device.opened, true)
        await device.close()
        assert.equal(device.opened, false)

describe 'Configurations', ->
    device = null
    before ->
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
        await device.open()

    it 'selects existing configuration', ->
        assert.doesNotReject(device.selectConfiguration(1))

    it 'fails to select missing configuration', ->
        assert.rejects(device.selectConfiguration(99))

    after ->
        device.close()

describe 'Interfaces', ->
    device = null
    before ->
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
        await device.open()

    it 'claims existing interface', ->
        assert.doesNotReject(device.claimInterface(0))

    it 'fails to claim missing interface', ->
        assert.rejects(device.claimInterface(99))

    it 'releases existing interface', ->
        assert.doesNotReject(device.releaseInterface(0))

    it 'fails to release missing interface', ->
        assert.rejects(device.releaseInterface(99))

    after ->
        device.close()

describe 'Alternates', ->
    device = null
    before ->
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
        await device.open()
        await device.claimInterface(0)

    it 'selects existing alternate', ->
        assert.doesNotReject(device.selectAlternateInterface(0, 0))

    after ->
        device.close()

describe 'Transfers', ->
    device = null
    b = Uint8Array.from([0x30...0x40]).buffer

    before ->
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
        await device.open()
        await device.claimInterface(0)

    it 'should control transfer OUT', ->
        transferResult = await device.controlTransferOut({
            requestType: 'device',
            recipient: 'vendor';
            request: 0x81,
            value: 0,
            index: 0
        }, b)

        assert.equal(transferResult.status, 'ok')
        assert.equal(transferResult.bytesWritten, b.byteLength)

    it 'should control transfer IN', ->
        transferResult = await device.controlTransferIn({
            requestType: 'device',
            recipient: 'vendor';
            request: 0x81,
            value: 0,
            index: 0
        }, 128)

        assert.equal(transferResult.status, 'ok')
        assert.equal(transferResult.data.buffer.toString(), b.toString())

    it 'should transfer OUT', ->
        transferResult = await device.transferOut(2, b)
        assert.equal(transferResult.status, 'ok')
        assert.equal(transferResult.bytesWritten, b.byteLength)

    it 'should transfer IN', ->
        transferResult = await device.transferIn(1, 64)
        assert.equal(transferResult.status, 'ok')
        assert.equal(transferResult.data.byteLength, 64)

    after ->
        device.close()
