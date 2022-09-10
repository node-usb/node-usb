import { promisify } from 'util';
import { WebUSB } from './webusb';
import * as usb from './usb';

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
    const opened = (device: usb.Device): boolean => !!device.interfaces;

    for (const device of devices) {
        try {
            if (!opened(device)) {
                device.open();
            }

            const getStringDescriptor = promisify(device.getStringDescriptor).bind(device);
            const buffer = await getStringDescriptor(device.deviceDescriptor.iSerialNumber);

            if (buffer && buffer.toString() === serialNumber) {
                return device;
            }
        } catch {
            // Ignore any errors, device may be a system device or inaccessible
        } finally {
            try {
                if (opened(device)) {
                    device.close();
                }
            } catch {
                // Ignore any errors, device may be a system device or inaccessible
            }
        }
    }

    return undefined;
};

const webusb = new WebUSB();

export {
    // Core usb object for quick access
    usb,

    // Convenience methods
    findByIds,
    findBySerialNumber,

    // Default WebUSB object (mimics navigator.usb)
    webusb
};

// Usb types
export { Device, Transfer, DeviceEvents, getDeviceList, useUsbDkBackend, LibUSBException } from './usb';
export * from './usb/capability';
export * from './usb/descriptors';
export * from './usb/endpoint';
export * from './usb/interface';

// WebUSB types
export * from './webusb';
export * from './webusb/webusb-device';
