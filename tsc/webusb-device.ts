import { promisify } from 'util';
import { Endpoint, InEndpoint, OutEndpoint } from './endpoint';
import {
    Device,
    LIBUSB_ENDPOINT_IN,
    LIBUSB_ENDPOINT_OUT,
    LIBUSB_RECIPIENT_DEVICE,
    LIBUSB_RECIPIENT_INTERFACE,
    LIBUSB_RECIPIENT_ENDPOINT,
    LIBUSB_RECIPIENT_OTHER,
    LIBUSB_REQUEST_TYPE_STANDARD,
    LIBUSB_REQUEST_TYPE_CLASS,
    LIBUSB_REQUEST_TYPE_VENDOR,
    LIBUSB_TRANSFER_STALL,
    LIBUSB_TRANSFER_OVERFLOW,
    LIBUSB_TRANSFER_TYPE_BULK,
    LIBUSB_TRANSFER_TYPE_INTERRUPT
} from './usb';

const LIBUSB_TRANSFER_TYPE_MASK = 0x03;
const ENDPOINT_NUMBER_MASK = 0x7f;
const CLEAR_FEATURE = 0x01;
const ENDPOINT_HALT = 0x00;

/**
 * A mutex implementation that can be used to lock access between concurrent async functions
 */
class Mutex {
    private locked: boolean;

    /**
     * Create a new Mutex
     */
    constructor() {
        this.locked = false;
    }

    /**
     *  Yield the current execution context, effectively moving it to the back of the promise queue
     */
    private async sleep(): Promise<void> {
        return new Promise(resolve => setTimeout(resolve, 1));
    }

    /**
     * Wait until the Mutex is available and claim it
     */
    public async lock(): Promise<void> {
        while (this.locked) {
            await this.sleep();
        }

        this.locked = true;
    }

    /**
     * Unlock the Mutex
     */
    public unlock(): void {
        this.locked = false;
    }
}

/**
 * Wrapper to make a node-usb device look like a webusb device
 */
export class WebUSBDevice implements USBDevice {
    public static async createInstance(device: Device): Promise<WebUSBDevice> {
        const instance = new WebUSBDevice(device);
        await instance.initialise();
        return instance;
    }

    public readonly usbVersionMajor: number;
    public readonly usbVersionMinor: number;
    public readonly usbVersionSubminor: number;
    public readonly deviceClass: number;
    public readonly deviceSubclass: number;
    public readonly deviceProtocol: number;
    public readonly vendorId: number;
    public readonly productId: number;
    public readonly deviceVersionMajor: number;
    public readonly deviceVersionMinor: number;
    public readonly deviceVersionSubminor: number;

    public manufacturerName?: string | undefined;
    public productName?: string | undefined;
    public serialNumber?: string | undefined;
    public configurations: USBConfiguration[] = [];

    private deviceMutex = new Mutex();

    private constructor(private device: Device) {
        const usbVersion = this.decodeVersion(device.deviceDescriptor.bcdUSB);
        this.usbVersionMajor = usbVersion.major;
        this.usbVersionMinor = usbVersion.minor;
        this.usbVersionSubminor = usbVersion.sub;

        this.deviceClass = device.deviceDescriptor.bDeviceClass;
        this.deviceSubclass = device.deviceDescriptor.bDeviceSubClass;
        this.deviceProtocol = device.deviceDescriptor.bDeviceProtocol;
        this.vendorId = device.deviceDescriptor.idVendor;
        this.productId = device.deviceDescriptor.idProduct;

        const deviceVersion = this.decodeVersion(device.deviceDescriptor.bcdDevice);
        this.deviceVersionMajor = deviceVersion.major;
        this.deviceVersionMinor = deviceVersion.minor;
        this.deviceVersionSubminor = deviceVersion.sub;
    }

    public get configuration(): USBConfiguration | undefined {
        if (!this.device.configDescriptor) {
            return undefined;
        }
        const currentConfiguration = this.device.configDescriptor.bConfigurationValue;
        return this.configurations.find(configuration => configuration.configurationValue === currentConfiguration);
    }

    public get opened(): boolean {
        return (!!this.device.interfaces);
    }

    public async open(): Promise<void> {
        try {
            await this.deviceMutex.lock();

            if (this.opened) {
                return;
            }

            this.device.open();
        } catch (error) {
            throw new Error(`open error: ${error}`);
        } finally {
            this.deviceMutex.unlock();
        }
    }

    public async close(): Promise<void> {
        try {
            await this.deviceMutex.lock();

            if (!this.opened) {
                return;
            }

            try {
                if (this.configuration) {
                    for (const iface of this.configuration?.interfaces) {
                        await this._releaseInterface(iface.interfaceNumber);
                        // Re-create the USBInterface to set the claimed attribute
                        this.configuration.interfaces[this.configuration.interfaces.indexOf(iface)] = {
                            interfaceNumber: iface.interfaceNumber,
                            alternate : iface.alternate,
                            alternates : iface.alternates,
                            claimed : false
                        };
                    }
                }
            } catch (_error) {
                // Ignore
            }

            this.device.close();
        } catch (error) {
            throw new Error(`close error: ${error}`);
        } finally {
            this.deviceMutex.unlock();
        }
    }

    public async selectConfiguration(configurationValue: number): Promise<void> {
        try {
            await this.deviceMutex.lock();

            if (!this.opened || !this.device.configDescriptor) {
                throw new Error('selectConfiguration error: invalid state');
            }

            if (this.device.configDescriptor.bConfigurationValue === configurationValue) {
                return;
            }

            const config =  this.configurations.find(configuration => configuration.configurationValue === configurationValue);
            if (!config) {
                throw new Error('selectConfiguration error: configuration not found');
            }

            try {
                const setConfiguration = promisify(this.device.setConfiguration).bind(this.device);
                await setConfiguration(configurationValue);
            } catch (error) {
                throw new Error(`selectConfiguration error: ${error}`);
            }
        } finally {
            this.deviceMutex.unlock();
        }
    }

    public async claimInterface(interfaceNumber: number): Promise<void> {
        try {
            await this.deviceMutex.lock();

            if (!this.opened) {
                throw new Error('claimInterface error: invalid state');
            }

            if (!this.configuration) {
                throw new Error('claimInterface error: interface not found');
            }

            const iface = this.configuration.interfaces.find(usbInterface => usbInterface.interfaceNumber === interfaceNumber);
            if (!iface) {
                throw new Error('claimInterface error: interface not found');
            }

            if (iface.claimed) {
                return;
            }

            try {
                this.device.interface(interfaceNumber).claim();

                // Re-create the USBInterface to set the claimed attribute
                this.configuration.interfaces[this.configuration.interfaces.indexOf(iface)] = {
                    interfaceNumber,
                    alternate : iface.alternate,
                    alternates : iface.alternates,
                    claimed : true
                };
            } catch (error) {
                throw new Error(`claimInterface error: ${error}`);
            }
        } finally {
            this.deviceMutex.unlock();
        }
    }

    public async releaseInterface(interfaceNumber: number): Promise<void> {
        try {
            await this.deviceMutex.lock();
            await this._releaseInterface(interfaceNumber);

            if (this.configuration) {
                const iface = this.configuration.interfaces.find(usbInterface => usbInterface.interfaceNumber === interfaceNumber);
                if (iface) {
                    // Re-create the USBInterface to set the claimed attribute
                    this.configuration.interfaces[this.configuration.interfaces.indexOf(iface)] = {
                        interfaceNumber,
                        alternate : iface.alternate,
                        alternates : iface.alternates,
                        claimed : false
                    };
                }
            }

        } finally {
            this.deviceMutex.unlock();
        }
    }

    public async selectAlternateInterface(interfaceNumber: number, alternateSetting: number): Promise<void> {
        try {
            await this.deviceMutex.lock();

            if (!this.opened) {
                throw new Error('selectAlternateInterface error: invalid state');
            }

            if (!this.configuration) {
                throw new Error('selectAlternateInterface error: interface not found');
            }

            const iface = this.configuration.interfaces.find(usbInterface => usbInterface.interfaceNumber === interfaceNumber);
            if (!iface) {
                throw new Error('selectAlternateInterface error: interface not found');
            }

            if (!iface.claimed) {
                throw new Error('selectAlternateInterface error: invalid state');
            }

            try {
                const iface = this.device.interface(interfaceNumber);
                const setAltSetting = promisify(iface.setAltSetting).bind(iface);
                await setAltSetting(alternateSetting);
            } catch (error) {
                throw new Error(`selectAlternateInterface error: ${error}`);
            }
        } finally {
            this.deviceMutex.unlock();
        }
    }

    public async controlTransferIn(setup: USBControlTransferParameters, length: number): Promise<USBInTransferResult> {
        try {
            await this.deviceMutex.lock();
            const type = this.controlTransferParamsToType(setup, LIBUSB_ENDPOINT_IN);
            const controlTransfer = promisify(this.device.controlTransfer).bind(this.device);
            const result = await controlTransfer(type, setup.request, setup.value, setup.index, length);

            return {
                data: result ? new DataView(new Uint8Array(result).buffer) : undefined,
                status: 'ok'
            };
        } catch (error) {
            if (error.errno === LIBUSB_TRANSFER_STALL) {
                return {
                    status: 'stall'
                };
            }

            if (error.errno === LIBUSB_TRANSFER_OVERFLOW) {
                return {
                    status: 'babble'
                };
            }

            throw new Error(`controlTransferIn error: ${error}`);
        } finally {
            this.deviceMutex.unlock();
        }
    }

    public async controlTransferOut(setup: USBControlTransferParameters, data?: ArrayBuffer): Promise<USBOutTransferResult> {
        try {
            await this.deviceMutex.lock();
            const type = this.controlTransferParamsToType(setup, LIBUSB_ENDPOINT_OUT);
            const controlTransfer = promisify(this.device.controlTransfer).bind(this.device);
            const buffer = data ? Buffer.from(data) : new Buffer(0);
            await controlTransfer(type, setup.request, setup.value, setup.index, buffer);

            return {
                bytesWritten: buffer.byteLength, // Hack, should be bytes actually written
                status: 'ok'
            };
        } catch (error) {
            if (error.errno === LIBUSB_TRANSFER_STALL) {
                return {
                    bytesWritten: 0,
                    status: 'stall'
                };
            }

            throw new Error(`controlTransferOut error: ${error}`);
        } finally {
            this.deviceMutex.unlock();
        }
    }

    public async clearHalt(direction: USBDirection, endpointNumber: number): Promise<void> {
        try {
            await this.deviceMutex.lock();
            const wIndex = endpointNumber | (direction === 'in' ? LIBUSB_ENDPOINT_IN : LIBUSB_ENDPOINT_OUT);
            const controlTransfer = promisify(this.device.controlTransfer).bind(this.device);
            await controlTransfer(LIBUSB_RECIPIENT_ENDPOINT, CLEAR_FEATURE, ENDPOINT_HALT, wIndex, 0);
        } catch (error) {
            throw new Error(`clearHalt error: ${error}`);
        } finally {
            this.deviceMutex.unlock();
        }
    }

    public async transferIn(endpointNumber: number, length: number): Promise<USBInTransferResult> {
        try {
            await this.deviceMutex.lock();
            const endpoint = this.getEndpoint(endpointNumber | LIBUSB_ENDPOINT_IN) as InEndpoint;
            const transfer = promisify(endpoint.transfer).bind(endpoint);
            const result = await transfer(length);

            return {
                data: result ? new DataView(new Uint8Array(result).buffer) : undefined,
                status: 'ok'
            };
        } catch (error) {
            if (error.errno === LIBUSB_TRANSFER_STALL) {
                return {
                    status: 'stall'
                };
            }

            if (error.errno === LIBUSB_TRANSFER_OVERFLOW) {
                return {
                    status: 'babble'
                };
            }

            throw new Error(`transferIn error: ${error}`);
        } finally {
            this.deviceMutex.unlock();
        }
    }

    public async transferOut(endpointNumber: number, data: ArrayBuffer): Promise<USBOutTransferResult> {
        try {
            await this.deviceMutex.lock();
            const endpoint = this.getEndpoint(endpointNumber | LIBUSB_ENDPOINT_OUT) as OutEndpoint;
            const transfer = promisify(endpoint.transfer).bind(endpoint);
            const buffer = Buffer.from(data);
            await transfer(buffer);

            return {
                bytesWritten: buffer.byteLength, // Hack, should be bytes actually written
                status: 'ok'
            };
        } catch (error) {
            if (error.errno === LIBUSB_TRANSFER_STALL) {
                return {
                    bytesWritten: 0,
                    status: 'stall'
                };
            }

            throw new Error(`transferOut error: ${error}`);
        } finally {
            this.deviceMutex.unlock();
        }
    }

    public async isochronousTransferIn(_endpointNumber: number, _packetLengths: number[]): Promise<USBIsochronousInTransferResult> {
        throw new Error('isochronousTransferIn error: method not implemented');
    }

    public async isochronousTransferOut(_endpointNumber: number, _data: BufferSource, _packetLengths: number[]): Promise<USBIsochronousOutTransferResult> {
        throw new Error('isochronousTransferOut error: method not implemented');
    }

    public async reset(): Promise<void> {
        try {
            await this.deviceMutex.lock();
            const reset = promisify(this.device.reset).bind(this.device);
            await reset();
        } catch (error) {
            throw new Error(`reset error: ${error}`);
        } finally {
            this.deviceMutex.unlock();
        }
    }

    private async initialise(): Promise<void> {
        try {
            await this.deviceMutex.lock();
            this.device.open();
            this.manufacturerName = await this.getStringDescriptor(this.device.deviceDescriptor.iManufacturer);
            this.productName = await this.getStringDescriptor(this.device.deviceDescriptor.iProduct);
            this.serialNumber = await this.getStringDescriptor(this.device.deviceDescriptor.iSerialNumber);
            this.configurations = await this.getConfigurations();
        } finally {
            this.device.close();
            this.deviceMutex.unlock();
        }
    }

    private decodeVersion(version: number): { [key: string]: number } {
        const hex = `0000${version.toString(16)}`.slice(-4);
        return {
            major: parseInt(hex.substr(0, 2), undefined),
            minor: parseInt(hex.substr(2, 1), undefined),
            sub: parseInt(hex.substr(3, 1), undefined),
        };
    }

    private async getStringDescriptor(index: number): Promise<string> {
        try {
            const getStringDescriptor = promisify(this.device.getStringDescriptor).bind(this.device);
            const buffer = await getStringDescriptor(index);
            return buffer ? buffer.toString() : '';
        } catch (error) {
            return '';
        }
    }

    private async getConfigurations(): Promise<USBConfiguration[]> {
        const configs: USBConfiguration[] = [];

        for (const config of this.device.allConfigDescriptors) {
            const interfaces: USBInterface[] = [];

            for (const iface of config.interfaces) {
                const alternates: USBAlternateInterface[] = [];

                for (const alternate of iface) {
                    const endpoints: USBEndpoint[] = [];

                    for (const endpoint of alternate.endpoints) {
                        endpoints.push({
                            endpointNumber: endpoint.bEndpointAddress & ENDPOINT_NUMBER_MASK,
                            direction: endpoint.bEndpointAddress & LIBUSB_ENDPOINT_IN ? 'in' : 'out',
                            type: (endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) === LIBUSB_TRANSFER_TYPE_BULK ? 'bulk'
                                : (endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) === LIBUSB_TRANSFER_TYPE_INTERRUPT ? 'interrupt'
                                    : 'isochronous',
                            packetSize: endpoint.wMaxPacketSize
                        });
                    }

                    alternates.push({
                        alternateSetting: alternate.bAlternateSetting,
                        interfaceClass: alternate.bInterfaceClass,
                        interfaceSubclass: alternate.bInterfaceSubClass,
                        interfaceProtocol: alternate.bInterfaceProtocol,
                        interfaceName: await this.getStringDescriptor(alternate.iInterface),
                        endpoints
                    });
                }

                const interfaceNumber = iface[0].bInterfaceNumber;
                const alternate = alternates.find(alt => alt.alternateSetting === this.device.interface(interfaceNumber).altSetting);

                if (alternate) {
                    interfaces.push({
                        interfaceNumber,
                        alternate,
                        alternates,
                        claimed: false
                    });
                }
            }

            configs.push({
                configurationValue: config.bConfigurationValue,
                configurationName: await this.getStringDescriptor(config.iConfiguration),
                interfaces
            });
        }

        return configs;
    }

    private getEndpoint(address: number): Endpoint | undefined {
        if (!this.device.interfaces) {
            return undefined;
        }

        for (const iface of this.device.interfaces) {
            const endpoint = iface.endpoint(address);
            if (endpoint) {
                return endpoint;
            }
        }

        return undefined;
    }

    private controlTransferParamsToType(setup: USBControlTransferParameters, direction: number): number {
        const recipient = setup.recipient === 'device' ? LIBUSB_RECIPIENT_DEVICE
            : setup.recipient === 'interface' ? LIBUSB_RECIPIENT_INTERFACE
                : setup.recipient === 'endpoint' ? LIBUSB_RECIPIENT_ENDPOINT
                    : LIBUSB_RECIPIENT_OTHER;

        const requestType = setup.requestType === 'standard' ? LIBUSB_REQUEST_TYPE_STANDARD
            : setup.requestType === 'class' ? LIBUSB_REQUEST_TYPE_CLASS
                : LIBUSB_REQUEST_TYPE_VENDOR;

        return recipient | requestType | direction;
    }

    private async _releaseInterface(interfaceNumber: number): Promise<void> {
        if (!this.opened) {
            throw new Error('releaseInterface error: invalid state');
        }

        if (!this.configuration) {
            throw new Error('releaseInterface error: interface not found');
        }

        const iface = this.configuration.interfaces.find(usbInterface => usbInterface.interfaceNumber === interfaceNumber);
        if (!iface) {
            throw new Error('releaseInterface error: interface not found');
        }

        if (!iface.claimed) {
            return;
        }

        try {
            const iface = this.device.interface(interfaceNumber);
            const release = promisify(iface.release).bind(iface);
            await release();
        } catch (error) {
            throw new Error(`releaseInterface error: ${error}`);
        }
    }
}
