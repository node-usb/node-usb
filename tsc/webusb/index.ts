import * as usb from '../usb';
import { WebUSBDevice } from './webusb-device';
import { TypedEventTarget } from './typed-event-target';

/**
 * USB Options
 */
export interface USBOptions {
    /**
     * A `device found` callback function to allow the user to select a device
     */
    devicesFound?: (devices: Array<USBDevice>) => Promise<USBDevice | void>;

    /**
     * Optional timeout (in milliseconds) to use for the device control transfers
     */
    deviceTimeout?: number;
}

/**
 * @hidden
 */
interface Device {
    vendorId?: number;
    productId?: number;
    serialNumber?: string;
}

/**
 * @hidden
 */
export type USBEvents = {
    /**
     * USBDevice connected event
     */
    connect: USBConnectionEvent;
    /**
     * USBDevice disconnected event
     */
    disconnect: USBConnectionEvent;
};

export class WebUSB extends TypedEventTarget<USBEvents> implements USB {

    private allowedDevices: Array<Device> = [];

    constructor(private options: USBOptions = {}) {
        super();
    }

    public set onconnect(listener: (event: USBConnectionEvent) => void) {
        this.addEventListener('connect', listener);
    }

    public set ondisconnect(listener: (event: USBConnectionEvent) => void) {
        this.addEventListener('disconnect', listener);
    }

    /**
     * Requests a single Web USB device
     * @param options The options to use when scanning
     * @returns Promise containing the selected device
     */
    public async requestDevice(options?: USBDeviceRequestOptions): Promise<USBDevice> {
        // Must have options
        if (!options) {
            throw new TypeError('requestDevice error: 1 argument required, but only 0 present');
        }

        // Options must be an object
        if (options.constructor !== {}.constructor) {
            throw new TypeError('requestDevice error: parameter 1 (options) is not an object');
        }

        // Must have a filter
        if (!options.filters) {
            throw new TypeError('requestDevice error: required member filters is undefined');
        }

        // Filter must be an array
        if (options.filters.constructor !== [].constructor) {
            throw new TypeError('requestDevice error: the provided value cannot be converted to a sequence');
        }

        // Check filters
        options.filters.forEach(filter => {
            // Protocol & Subclass
            if (filter.protocolCode && !filter.subclassCode) {
                throw new TypeError('requestDevice error: subclass code is required');
            }

            // Subclass & Class
            if (filter.subclassCode && !filter.classCode) {
                throw new TypeError('requestDevice error: class code is required');
            }
        });

        let devices = await this.loadDevices(options.filters);
        devices = devices.filter(device => this.filterDevice(options, device));

        if (devices.length === 0) {
            throw new Error('requestDevice error: no devices found');
        }

        try {
            // If no devicesFound function, select the first device found
            const device = this.options.devicesFound ? await this.options.devicesFound(devices) : devices[0];

            if (!device) {
                throw new Error('selected device not found');
            }

            if (!this.replaceAllowedDevice(device)) {
                this.allowedDevices.push({
                    vendorId: device.vendorId,
                    productId: device.productId,
                    serialNumber: device.serialNumber
                });
            }

            return device;
        } catch (error) {
            throw new Error(`requestDevice error: ${error}`);
        }
    }

    /**
     * Gets all allowed Web USB devices which are connected
     * @returns Promise containing an array of devices
     */
    public async getDevices(): Promise<USBDevice[]> {
        // Create pre-filters
        const preFilters = this.allowedDevices.map(device => ({
            vendorId: device.vendorId || undefined,
            productId: device.productId || undefined,
            serialNumber: device.serialNumber || undefined
        }));

        // Refresh devices and filter for allowed ones
        let devices = await this.loadDevices(preFilters);
        devices = devices.filter(device => {
            for (const i in this.allowedDevices) {
                if (this.isSameDevice(device, this.allowedDevices[i])) {
                    return true;
                }
            }

            return false;
        });

        return devices;
    }

    private async loadDevices(preFilters?: Array<Device>): Promise<USBDevice[]> {
        let devices = usb.getDeviceList();

        if (preFilters) {
            // Pre-filter devices
            devices = this.preFilterDevices(devices, preFilters);
        }

        const webDevices: USBDevice[] = [];

        for (const device of devices) {
            if (this.options.deviceTimeout) {
                device.timeout = this.options.deviceTimeout;
            }

            const webDevice = await WebUSBDevice.createInstance(device);
            webDevices.push(webDevice);
        }

        return webDevices;
    }

    private preFilterDevices(devices: Array<usb.Device>, preFilters: Array<Device>): Array<usb.Device> {
        // Just pre-filter on vid/pid
        return devices.filter(device => preFilters.some(filter => {
            // Vendor
            if (filter.vendorId && filter.vendorId !== device.deviceDescriptor.idVendor) return false;

            // Product
            if (filter.productId && filter.productId !== device.deviceDescriptor.idProduct) return false;

            // Ignore serial number for node-usb as it requires device connection
            return true;
        }));
    }

    private isSameDevice(device1: Device, device2: Device): boolean {
        return (device1.productId === device2.productId
            && device1.vendorId === device2.vendorId
            && device1.serialNumber === device2.serialNumber);
    }

    private replaceAllowedDevice(device: USBDevice): boolean {
        for (const i in this.allowedDevices) {
            if (this.isSameDevice(device, this.allowedDevices[i])) {
                this.allowedDevices[i] = {
                    vendorId: device.vendorId,
                    productId: device.productId,
                    serialNumber: device.serialNumber
                };
                return true;
            }
        }

        return false;
    }

    private filterDevice(options: USBDeviceRequestOptions, device: USBDevice): boolean {
        return options.filters.some(filter => {
            // Vendor
            if (filter.vendorId && filter.vendorId !== device.vendorId) return false;

            // Product
            if (filter.productId && filter.productId !== device.productId) return false;

            // Class
            if (filter.classCode) {

                if (!device.configuration) {
                    return false;
                }

                // Interface Descriptors
                const match = device.configuration.interfaces.some(iface => {
                    // Class
                    if (filter.classCode && filter.classCode !== iface.alternate.interfaceClass) return false;

                    // Subclass
                    if (filter.subclassCode && filter.subclassCode !== iface.alternate.interfaceSubclass) return false;

                    // Protocol
                    if (filter.protocolCode && filter.protocolCode !== iface.alternate.interfaceProtocol) return false;

                    return true;
                });

                if (match) {
                    return true;
                }
            }

            // Class
            if (filter.classCode && filter.classCode !== device.deviceClass) return false;

            // Subclass
            if (filter.subclassCode && filter.subclassCode !== device.deviceSubclass) return false;

            // Protocol
            if (filter.protocolCode && filter.protocolCode !== device.deviceProtocol) return false;

            // Serial
            if (filter.serialNumber && filter.serialNumber !== device.serialNumber) return false;

            return true;
        });
    }
}
