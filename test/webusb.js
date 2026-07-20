const assert = require('assert');
const webusb = require('../').webusb;
const WebUSB = require('../').WebUSB;

if (typeof gc === 'function') {
    // Running with --expose-gc, do a sweep between tests so valgrind blames the right one.
    afterEach(() => gc());
}

describe('WebUSB Module', () => {
    it('should describe basic constants', () => {
        assert.notEqual(webusb, undefined, 'webusb must not be undefined');
    });
});

describe('allowedDevices', () => {
    it('should not list any devices by default', async () => {
        const devices = await webusb.getDevices();
        assert.equal(devices.length, 0);
    });

    it('should list allowed devices', async () => {
        const customWebusb = new WebUSB({ allowedDevices: [{ vendorId: 0x59e3 }] });
        const devices = await customWebusb.getDevices();
        assert.equal(devices.length, 1);
        assert.notEqual(devices[0], undefined);
    });
});

describe('requestDevice', () => {
    it('should return a device', async () => {
        const device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
        assert.notEqual(device, undefined);
    });
});

describe('getDevices', () => {
    it('should return one device', async () => {
        const device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
        const devices = await webusb.getDevices();
        assert.equal(devices.length, 1);
        assert.notEqual(devices[0], undefined);
        assert.deepEqual(devices[0], device);
    });
});

describe('WebUSB Hotplug', () => {
    it('should detect disconnect', done => {
        const fn = event => {
            assert.equal(event.device.serialNumber, 'TEST_DEVICE');
            webusb.removeEventListener('disconnect', fn);
            done();
        };
        webusb.addEventListener('disconnect', fn);
        console.log('\n--- DISCONNECT DEVICE ---\n');
    });

    it('should detect connect', done => {
        const fn = event => {
            assert.equal(event.device.serialNumber, 'TEST_DEVICE');
            webusb.removeEventListener('connect', fn);
            done();
        };
        webusb.addEventListener('connect', fn);
        console.log('\n--- CONNECT DEVICE ---\n');
    });
});

describe('Device properties', () => {
    let device;

    before(async () => {
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
    });

    it('should have usb version properties', () => {
        assert.equal(device.usbVersionMajor, 1);
        assert.equal(device.usbVersionMinor, 1);
        assert.equal(device.usbVersionSubminor, 0);
    });

    it('should have device version properties', () => {
        assert.equal(device.deviceVersionMajor, 1);
        assert.equal(device.deviceVersionMinor, 1);
        assert.equal(device.deviceVersionSubminor, 0);
    });

    it('should have class properties', () => {
        assert.equal(device.deviceClass, 0);
        assert.equal(device.deviceSubclass, 0);
    });

    it('should have protocol property', () => {
        assert.equal(device.deviceProtocol, 0);
    });

    it('should have vid/pid properties', () => {
        assert.equal(device.vendorId, 0x59e3);
        assert.equal(device.productId, 0x0a23);
    });

    it('should have a single configuration', () => {
        assert.equal(device.configurations.length, 1);
        assert.equal(device.configurations[0].configurationValue, 1);
    });

    it('should have a configuration property', () => {
        assert.notEqual(device.configuration, undefined);
    });

    it('should have a single interface', () => {
        assert.equal(device.configuration.interfaces.length, 1);
        assert.equal(device.configuration.interfaces[0].interfaceNumber, 0);
    });

    it('should have a single alternate', () => {
        assert.equal(device.configuration.interfaces[0].alternates.length, 1);
        assert.equal(device.configuration.interfaces[0].alternates[0].alternateSetting, 0);
    });
});

describe('String descriptors', () => {
    let device;

    before(async () => {
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
    });

    it('gets serialNumber string', () => {
        assert.equal(device.serialNumber, 'TEST_DEVICE');
    });

    it('gets manufacturerName string', () => {
        assert.equal(device.manufacturerName, 'Nonolith Labs');
    });

    it('gets productName string', () => {
        assert.equal(device.productName, 'STM32F103 Test Device');
    });
});

describe('Device access', () => {
    let device;

    before(async () => {
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
    });

    it('is not open', () => {
        assert.equal(device.opened, false);
    });

    it('is opens and closes', async () => {
        assert.equal(device.opened, false);
        await device.open();
        assert.equal(device.opened, true);
        await device.close();
        assert.equal(device.opened, false);
    });
});

describe('Configurations', () => {
    let device;

    before(async () => {
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
        await device.open();
    });

    it('selects existing configuration', async () => {
        await assert.doesNotReject(device.selectConfiguration(1));
    });

    it('fails to select missing configuration', async () => {
        await assert.rejects(device.selectConfiguration(99));
    });

    after(async () => {
        await device.close();
    });
});

describe('Interfaces', () => {
    let device;

    before(async () => {
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
        await device.open();
    });

    it('claims existing interface', async () => {
        await assert.doesNotReject(device.claimInterface(0));
    });

    it('fails to claim missing interface', async () => {
        await assert.rejects(device.claimInterface(99));
    });

    it('releases existing interface', async () => {
        await assert.doesNotReject(device.releaseInterface(0));
    });

    it('fails to release missing interface', async () => {
        await assert.rejects(device.releaseInterface(99));
    });

    after(async () => {
        await device.close();
    });
});

describe('Alternates', () => {
    let device;

    before(async () => {
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
        await device.open();
        await device.claimInterface(0);
    });

    it('selects existing alternate', async () => {
        await assert.doesNotReject(device.selectAlternateInterface(0, 0));
    });

    after(async () => {
        await device.close();
    });
});

describe('Throwing Transfers', () => {
    let device;

    before(async () => {
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
    });

    it('should fail transfer unless opened', async () => {
        await assert.rejects(device.transferIn(1, 64));
    });
});

describe('Transfers', () => {
    let device;
    const buffer1 = Uint8Array.from(Array.from({ length: 0x10 }, (_, index) => 0x30 + index)).buffer;
    const buffer2 = Uint8Array.from(Array.from({ length: 0x10 }, (_, index) => 0x32 + index)).buffer;

    before(async () => {
        device = await webusb.requestDevice({ filters: [{ vendorId: 0x59e3 }] });
        await device.open();
        await device.claimInterface(0);
    });

    it('should control transfer OUT', async () => {
        const transferResult = await device.controlTransferOut({
            requestType: 'vendor',
            recipient: 'device',
            request: 0x81,
            value: 0,
            index: 0
        }, buffer1);

        assert.equal(transferResult.status, 'ok');
        assert.equal(transferResult.bytesWritten, buffer1.byteLength);
    });

    it('should control transfer IN', async () => {
        const transferResult = await device.controlTransferIn({
            requestType: 'vendor',
            recipient: 'device',
            request: 0x81,
            value: 0,
            index: 0
        }, 128);

        assert.equal(transferResult.status, 'ok');
        assert.equal(transferResult.data.byteLength, buffer1.byteLength);
        const resultBuffer = Buffer.from(transferResult.data.buffer, transferResult.data.byteOffset, transferResult.data.byteLength);
        const expectedBuffer = Buffer.from(buffer1, 0, buffer1.byteLength);
        assert(resultBuffer.equals(expectedBuffer));
    });

    it('should transfer OUT', async () => {
        const transferResult = await device.transferOut(4, buffer2);

        assert.equal(transferResult.status, 'ok');
        assert.equal(transferResult.bytesWritten, buffer2.byteLength);
    });

    it('should transfer IN', async () => {
        const transferResult = await device.transferIn(3, buffer2.byteLength);

        assert.equal(transferResult.status, 'ok');
        assert.equal(transferResult.data.byteLength, buffer2.byteLength);

        const resultBuffer = Buffer.from(transferResult.data.buffer, transferResult.data.byteOffset, transferResult.data.byteLength);
        const expectedBuffer = Buffer.from(buffer2, 0, buffer2.byteLength);
        assert(resultBuffer.equals(expectedBuffer));
    });

    after(async () => {
        await device.close();
    });
});
