import { promisify } from 'util';
import { EventEmitter } from 'events';
import { ExtendedDevice } from './device';
import * as usb from './bindings';
import type { EventListeners } from './typed-events';

if (usb.INIT_ERROR) {
    console.warn('Failed to initialize libusb.');
}

Object.setPrototypeOf(usb, EventEmitter.prototype);

Object.getOwnPropertyNames(ExtendedDevice.prototype).forEach(name => {
    Object.defineProperty(usb.Device.prototype, name, Object.getOwnPropertyDescriptor(ExtendedDevice.prototype, name) || Object.create(null));
});

declare module './bindings' {
	interface Device extends ExtendedDevice {
        // Merge interfaces
    }

	interface DeviceEvents extends EventListeners<DeviceEvents> {
		attach: Device;
		detach: Device;
	}

	function addListener<K extends keyof DeviceEvents>(event: K, listener: (arg: DeviceEvents[K]) => void): void;
	function removeListener<K extends keyof DeviceEvents>(event: K, listener: (arg: DeviceEvents[K]) => void): void;
	function on<K extends keyof DeviceEvents>(event: K, listener: (arg: DeviceEvents[K]) => void): void;
	function off<K extends keyof DeviceEvents>(event: K, listener: (arg: DeviceEvents[K]) => void): void;
	function once<K extends keyof DeviceEvents>(event: K, listener: (arg: DeviceEvents[K]) => void): void;
	function listeners<K extends keyof DeviceEvents>(event: K): ((arg: DeviceEvents[K]) => void)[];
	function rawListeners<K extends keyof DeviceEvents>(event: K): ((arg: DeviceEvents[K]) => void)[];
	function removeAllListeners<K extends keyof DeviceEvents>(event?: K): void;
	function emit<K extends keyof DeviceEvents>(event: K, arg: DeviceEvents[K]): boolean;
	function listenerCount<K extends keyof DeviceEvents>(event: K): number;

	function findByIds(vid: number, pid: number): usb.Device | undefined;
	function findBySerialNumber(serialNumber: string): Promise<usb.Device | undefined>;
}

usb.on('newListener', event => {
    if (event !== 'attach' && event !== 'detach') {
        return;
    }
    const listenerCount = usb.listenerCount('attach') + usb.listenerCount('detach');
    if (listenerCount === 0) {
        usb._enableHotplugEvents();
    }
});

usb.on('removeListener', event => {
    if (event !== 'attach' && event !== 'detach') {
        return;
    }
    const listenerCount = usb.listenerCount('attach') + usb.listenerCount('detach');
    if (listenerCount === 0) {
        usb._disableHotplugEvents();
    }
});

/**
 * Convenience method to get the first device with the specified VID and PID, or `undefined` if no such device is present.
 * @param vid
 * @param pid
 */
const findByIds = (vid: number, pid: number): usb.Device | undefined => {
    const devices = usb.getDeviceList();
    return devices.find(item => item.deviceDescriptor.idVendor === vid && item.deviceDescriptor.idProduct === pid);
};

/**
 * Convenience method to get the device with the specified serial number, or `undefined` if no such device is present.
 * @param serialNumber
 */
const findBySerialNumber = async (serialNumber: string): Promise<usb.Device | undefined> => {
    const devices = usb.getDeviceList();

    for (const device of devices) {
        try {
            device.open();

            const getStringDescriptor = promisify(device.getStringDescriptor).bind(device);
            const buffer = await getStringDescriptor(device.deviceDescriptor.iSerialNumber);

            if (buffer && buffer.toString() === serialNumber) {
                return device;
            }
        } catch {
            // Ignore any errors, device may be a system device or inaccessible
        } finally {
            try {
                device.close();
            } catch {
                // Ignore any errors, device may be a system device or inaccessible
            }
        }
    }

    return undefined;
};

Object.assign(usb, {
    findByIds,
    findBySerialNumber
});

export = usb;
