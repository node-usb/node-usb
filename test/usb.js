const assert = require('assert');
const util = require('util');
const usb = require('../').usb;
const getDeviceList = require('../').getDeviceList;
const findByIds = require('../').findByIds;
const findBySerialNumber = require('../').findBySerialNumber;
const Worker = require('worker_threads').Worker;

if (typeof gc === 'function') {
    // Running with --expose-gc, do a sweep between tests so valgrind blames the right one.
    afterEach(() => gc());
}

describe('USB Module', () => {
    it('should describe basic constants', () => {
        assert.notEqual(usb, undefined, 'usb must not be undefined');
        assert.ok(usb.LIBUSB_CLASS_PER_INTERFACE !== undefined, 'Constants must be described');
        assert.ok(usb.LIBUSB_ENDPOINT_IN === 128);
    });

    it('should handle abuse without crashing', () => {
        assert.throws(() => new usb.Device());
        assert.throws(() => usb.Device());
        assert.throws(() => usb.Device.prototype.open.call({}));
    });
});

describe('setDebugLevel', () => {
    it('should throw when passed invalid args', () => {
        assert.throws(() => usb.setDebugLevel(), TypeError);
        assert.throws(() => usb.setDebugLevel(-1), TypeError);
        assert.throws(() => usb.setDebugLevel(5), TypeError);
    });

    it('should succeed with good args', () => {
        assert.doesNotThrow(() => usb.setDebugLevel(0));
    });
});

describe('getDeviceList', () => {
    it('should return at least one device', () => {
        const devices = getDeviceList();
        assert.ok(devices.length > 0);
    });
});

describe('findByIds', () => {
    it('should return an array with length > 0', () => {
        const device = findByIds(0x59e3, 0x0a23);
        assert.ok(device, 'Demo device is not attached');
    });
});

describe('findBySerialNumber', () => {
    it('should return a single device', () => {
        const device = findBySerialNumber('TEST_DEVICE');
        assert.ok(device, 'Demo device is not attached');
    });
});

describe('Hotplug', () => {
    it('should detect detach', done => {
        usb.once('detach', device => {
            assert.equal(device.deviceDescriptor.idVendor, 0x59e3);
            assert.equal(device.deviceDescriptor.idProduct, 0x0a23);
            done();
        });
        console.log('\n--- DISCONNECT DEVICE ---\n');
    });

    it('should detect attach', done => {
        usb.once('attach', device => {
            assert.equal(device.deviceDescriptor.idVendor, 0x59e3);
            assert.equal(device.deviceDescriptor.idProduct, 0x0a23);
            done();
        });
        console.log('\n--- CONNECT DEVICE ---\n');
    });
});

describe('Device', () => {
    let device;
    const buffer = Buffer.from(Array.from({ length: 0x10 }, (_, index) => 0x30 + index));

    before(() => {
        device = findByIds(0x59e3, 0x0a23);
    });

    it('should have sane properties', () => {
        assert.ok(device.busNumber > 0, 'busNumber must be larger than 0');
        assert.ok(device.deviceAddress > 0, 'deviceAddress must be larger than 0');
        if (process.platform !== 'darwin' || process.arch !== 'arm64') {
            assert.ok(util.isArray(device.portNumbers), 'portNumbers must be an array');
        }
    });

    it('should have a deviceDescriptor property', () => {
        assert.ok(device.deviceDescriptor !== undefined);
    });

    it('should have a configDescriptor property', () => {
        assert.ok(device.configDescriptor !== undefined);
    });

    it('should open', () => {
        device.open();
    });

    it('gets string descriptors', done => {
        device.getStringDescriptor(device.deviceDescriptor.iManufacturer, (error, string) => {
            assert.ok(error === undefined, error);
            assert.equal(string, 'Nonolith Labs');
            done();
        });
    });

    it('supports null string descriptors', done => {
        device.getStringDescriptor(device.configDescriptor.iConfiguration, (error, string) => {
            assert.ok(error === undefined, error);
            assert.equal(string, undefined);
            done();
        });
    });

    describe('control transfer', () => {
        it('should OUT transfer when the IN bit is not set', done => {
            device.controlTransfer(0x40, 0x81, 0, 0, buffer, error => {
                assert.ok(error === undefined, error);
                done();
            });
        });

        it('should fail when bmRequestType doesn\'t match buffer / length', () => {
            assert.throws(() => device.controlTransfer(0x40, 0x81, 0, 0, 64));
        });

        it('should IN transfer when the IN bit is set', done => {
            device.controlTransfer(0xc0, 0x81, 0, 0, 128, (error, data) => {
                assert.ok(error === undefined, error);
                assert.equal(data.toString(), buffer.toString());
                done();
            });
        });

        it('should signal errors', done => {
            device.controlTransfer(0xc0, 0xff, 0, 0, 64, error => {
                assert.equal(error.errno, usb.LIBUSB_TRANSFER_STALL);
                done();
            });
        });
    });

    describe('Interface', () => {
        let iface;

        before(() => {
            iface = device.interfaces[0];
            iface.claim();
        });

        it('should have one interface', () => {
            assert.notEqual(iface, undefined);
        });

        it('should be the same as the interfaceNo 0', () => {
            assert.strictEqual(iface, device.interface(0));
        });

        if (process.platform === 'linux') {
            it('shouldn\'t have a kernel driver', () => {
                assert.equal(iface.isKernelDriverActive(), false);
            });

            it('should fail to detach the kernel driver', () => {
                assert.throws(() => iface.detachKernelDriver());
            });

            it('should fail to attach the kernel driver', () => {
                assert.throws(() => iface.attachKernelDriver());
            });
        }

        describe('IN endpoint', () => {
            let inEndpoint;

            before(() => {
                inEndpoint = iface.endpoints[0];
            });

            it('should be able to get the endpoint', () => {
                assert.ok(inEndpoint != null);
            });

            it('should be able to get the endpoint by address', () => {
                assert.equal(inEndpoint, iface.endpoint(0x81));
            });

            it('should have the IN direction flag', () => {
                assert.equal(inEndpoint.direction, 'in');
            });

            it('should have a descriptor', () => {
                assert.equal(inEndpoint.descriptor.bEndpointAddress, 0x81);
                assert.equal(inEndpoint.descriptor.wMaxPacketSize, 64);
            });

            it('should fail to write', () => {
                assert.throws(() => inEndpoint.transfer(buffer));
            });

            it('should support read', done => {
                inEndpoint.transfer(64, (error, data) => {
                    assert.ok(error === undefined, error);
                    assert.ok(data.length === 64);
                    done();
                });
            });

            it('times out', done => {
                iface.endpoints[4].timeout = 20;
                iface.endpoints[4].transfer(64, error => {
                    assert.equal(error.errno, usb.LIBUSB_TRANSFER_TIMED_OUT);
                    done();
                });
            });

            it('polls the device using events', done => {
                let packets = 0;

                inEndpoint.startPoll(8, 64);
                inEndpoint.on('data', data => {
                    assert.equal(data.length, 64);
                    packets++;

                    if (packets === 100) {
                        inEndpoint.stopPoll();
                    }
                });

                inEndpoint.once('error', error => {
                    throw error;
                });

                inEndpoint.once('end', () => {
                    done();
                });
            });

            it('polls the device using a callback', done => {
                let packets = 0;

                inEndpoint.startPoll(8, 64, (error, _buffer, _actualLength, cancelled) => {
                    assert.equal(cancelled, true);
                    assert.ok(error === undefined, error);
                    assert.equal(packets, 100);
                    done();
                });

                inEndpoint.on('data', data => {
                    assert.equal(data.length, 64);
                    packets++;

                    if (packets === 100) {
                        inEndpoint.stopPoll();
                    }
                });

                inEndpoint.once('error', error => {
                    throw error;
                });

                inEndpoint.once('end', () => {
                    assert.equal(packets, 100);
                });
            });
        });

        describe('OUT endpoint', () => {
            let outEndpoint;

            before(() => {
                outEndpoint = iface.endpoints[1];
            });

            it('should be able to get the endpoint', () => {
                assert.ok(outEndpoint != null);
            });

            it('should be able to get the endpoint by address', () => {
                assert.equal(outEndpoint, iface.endpoint(0x02));
            });

            it('should have the OUT direction flag', () => {
                assert.equal(outEndpoint.direction, 'out');
            });

            it('should support write', done => {
                outEndpoint.transfer([1, 2, 3, 4], error => {
                    assert.ok(error === undefined, error);
                    done();
                });
            });

            it('times out', done => {
                iface.endpoints[5].timeout = 20;
                iface.endpoints[5].transfer([1, 2, 3, 4], error => {
                    assert.equal(error.errno, usb.LIBUSB_TRANSFER_TIMED_OUT);
                    done();
                });
            });
        });

        after(callback => {
            iface.release(callback);
        });
    });

    after(() => {
        device.close();
    });
});

if (process.platform !== 'win32') {
    describe('Context Aware', () => {
        it('should handle opening the same device from different contexts', async () => {
            await Promise.all(Array.from({ length: 5 }, () => new Promise((resolve, reject) => {
                const worker = new Worker('./test/worker.cjs');

                worker.on('message', serial => {
                    try {
                        assert.equal(serial, 'TEST_DEVICE');
                    } catch (error) {
                        reject(error);
                    }
                });

                worker.on('error', reject);
                worker.on('exit', code => {
                    try {
                        assert.equal(code, 0);
                        resolve();
                    } catch (error) {
                        reject(error);
                    }
                });
            })));
        });
    });
}
