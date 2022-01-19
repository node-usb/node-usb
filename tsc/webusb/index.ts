import * as usb from '../usb';
import { EventEmitter } from 'events';
import { WebUSBDevice } from './webusb-device';

/**
 * USB Options
 */
export interface USBOptions {
    /**
     * Optional `device found` callback function to allow the user to select a device
     */
    devicesFound?: (devices: USBDevice[]) => Promise<USBDevice | void>;

    /**
     * Optional array of preconfigured allowed devices
     */
    allowedDevices?: USBDeviceFilter[];

    /**
     * Optional flag to automatically allow all devices
     */
    allowAllDevices?: boolean;

    /**
     * Optional timeout (in milliseconds) to use for the device control transfers
     */
    deviceTimeout?: number;
}

/**
 * Convenience method to get the WebUSB interface available
 */
export const getWebUsb = (): USB => {
    if (navigator && navigator.usb) {
        return navigator.usb;
    }

    return new WebUSB();
};

export class WebUSB implements USB {

    protected emitter = new EventEmitter();
    protected knownDevices: Map<string, USBDevice> = new Map();
    protected allowedDevices: USBDeviceFilter[];

    constructor(private options: USBOptions = {}) {
        this.allowedDevices = options.allowedDevices || [];

        const deviceConnectCallback = async (device: usb.Device) => {
            const webDevice = await WebUSBDevice.createInstance(device);

            // When connected, emit an event if it is an allowed device
            if (webDevice && this.isAllowedDevice(webDevice)) {
                const deviceId = this.getDeviceId(device);
                if (deviceId) {
                    this.knownDevices.set(deviceId, webDevice);
                }

                const event = {
                    type: 'connect',
                    device: webDevice
                };

                this.emitter.emit('connect', event);
            }
        };

        const deviceDisconnectCallback = async (device: usb.Device) => {
            const deviceId = this.getDeviceId(device);

            // When disconnected, emit an event if the device was a known allowed device
            if (deviceId !== undefined && this.knownDevices.has(deviceId)) {
                const webDevice = this.knownDevices.get(deviceId);

                if (webDevice && this.isAllowedDevice(webDevice)) {
                    const event = {
                        type: 'disconnect',
                        device: webDevice
                    };

                    this.emitter.emit('disconnect', event);
                }
            }
        };

        this.emitter.on('newListener', event => {
            const listenerCount = this.emitter.listenerCount(event);

            if (listenerCount !== 0) {
                return;
            }

            if (event === 'connect') {
                usb.addListener('attach', deviceConnectCallback);
            } else if (event === 'disconnect') {
                usb.addListener('detach', deviceDisconnectCallback);
            }
        });

        this.emitter.on('removeListener', event => {
            const listenerCount = this.emitter.listenerCount(event);

            if (listenerCount !== 0) {
                return;
            }

            if (event === 'connect') {
                usb.removeListener('attach', deviceConnectCallback);
            } else if (event === 'disconnect') {
                usb.removeListener('detach', deviceDisconnectCallback);
            }
        });
    }

    private _onconnect: ((ev: USBConnectionEvent) => void) | undefined;
    public set onconnect(fn: (ev: USBConnectionEvent) => void) {
        if (this._onconnect) {
            this.removeEventListener('connect', this._onconnect);
            this._onconnect = undefined;
        }

        if (fn) {
            this._onconnect = fn;
            this.addEventListener('connect', this._onconnect);
        }
    }

    private _ondisconnect: ((ev: USBConnectionEvent) => void) | undefined;
    public set ondisconnect(fn: (ev: USBConnectionEvent) => void) {
        if (this._ondisconnect) {
            this.removeEventListener('disconnect', this._ondisconnect);
            this._ondisconnect = undefined;
        }

        if (fn) {
            this._ondisconnect = fn;
            this.addEventListener('disconnect', this._ondisconnect);
        }
    }

    public addEventListener(type: 'connect' | 'disconnect', listener: (this: this, ev: USBConnectionEvent) => void): void;
    public addEventListener(type: 'connect' | 'disconnect', listener: EventListener): void;
    public addEventListener(type: string, listener: (ev: USBConnectionEvent) => void): void {
        this.emitter.addListener(type, listener);
    }

    public removeEventListener(type: 'connect' | 'disconnect', callback: (this: this, ev: USBConnectionEvent) => void): void;
    public removeEventListener(type: 'connect' | 'disconnect', callback: EventListener): void;
    public removeEventListener(type: string, callback: (this: this, ev: USBConnectionEvent) => void): void {
        this.emitter.removeListener(type, callback);
    }

    public dispatchEvent(_event: Event): boolean {
        // Don't dispatch from here
        return false;
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

            if (!this.isAllowedDevice(device)) {
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
        let preFilters: USBDeviceFilter[] | undefined;

        if (!this.options.allowAllDevices) {
            // Create pre-filters
            preFilters = this.allowedDevices.map(device => ({
                vendorId: device.vendorId || undefined,
                productId: device.productId || undefined,
                serialNumber: device.serialNumber || undefined
            }));
        }

        // Refresh devices and filter for allowed ones
        const devices = await this.loadDevices(preFilters);
        return devices.filter(device => this.isAllowedDevice(device));
    }

    private async loadDevices(preFilters?: USBDeviceFilter[]): Promise<USBDevice[]> {
        let devices = usb.getDeviceList();

        // Pre-filter devices
        devices = this.preFilterDevices(devices, preFilters);

        const webDevices: USBDevice[] = [];

        for (const device of devices) {
            if (this.options.deviceTimeout) {
                device.timeout = this.options.deviceTimeout;
            }

            const webDevice = await WebUSBDevice.createInstance(device);
            if (webDevice) {
                webDevices.push(webDevice);

                const deviceId = this.getDeviceId(device);
                if (deviceId) {
                    this.knownDevices.set(deviceId, webDevice);
                }
            }
        }

        return webDevices;
    }

    private preFilterDevices(devices: usb.Device[], preFilters?: USBDeviceFilter[]): usb.Device[] {
        if (!preFilters || !preFilters.length) {
            return devices;
        }

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

    private filterDevice(options: USBDeviceRequestOptions, device: USBDevice): boolean {
        if (!options.filters || !options.filters.length) {
            return true;
        }

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

    private getDeviceId(device: usb.Device): string | undefined {
        if (device.busNumber === undefined || device.deviceAddress === undefined) {
            return undefined;
        }

        return `${device.busNumber}.${device.deviceAddress}`;
    }

    private isAllowedDevice(device: USBDeviceFilter): boolean {
        if (this.options.allowAllDevices) {
            return true;
        }

        const isSameDevice = (device1: USBDeviceFilter, device2: USBDeviceFilter): boolean => {
            return (device1.productId === device2.productId
                && device1.vendorId === device2.vendorId
                && device1.serialNumber === device2.serialNumber);
        };

        for (const i in this.allowedDevices) {
            if (isSameDevice(device, this.allowedDevices[i])) {
                return true;
            }
        }

        return false;
    }
}
