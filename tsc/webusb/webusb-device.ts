import * as usb from '../usb';
import { promisify } from 'util';
import { Endpoint, InEndpoint, OutEndpoint } from '../usb/endpoint';

const LIBUSB_TRANSFER_TYPE_MASK = 0x03;
const ENDPOINT_NUMBER_MASK = 0x7f;
const CLEAR_FEATURE = 0x01;
const ENDPOINT_HALT = 0x00;

/**
 * Wrapper to make a node-usb device look like a webusb device
 */
export class WebUSBDevice implements USBDevice {
    public static async createInstance(device: usb.Device): Promise<WebUSBDevice | undefined> {
        try {
            const instance = new WebUSBDevice(device);
            await instance.initialize();
            return instance;
        } catch {
            return undefined;
        }
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

    private constructor(private device: usb.Device) {
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
            if (this.opened) {
                return;
            }

            this.device.open();
        } catch (error) {
            throw new Error(`open error: ${error}`);
        }
    }

    public async close(): Promise<void> {
        try {
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
                            alternate: iface.alternate,
                            alternates: iface.alternates,
                            claimed: false
                        };
                    }
                }
            } catch (_error) {
                // Ignore
            }

            this.device.close();
        } catch (error) {
            throw new Error(`close error: ${error}`);
        }
    }

    public async selectConfiguration(configurationValue: number): Promise<void> {
        if (!this.opened || !this.device.configDescriptor) {
            throw new Error('selectConfiguration error: invalid state');
        }

        if (this.device.configDescriptor.bConfigurationValue === configurationValue) {
            return;
        }

        const config = this.configurations.find(configuration => configuration.configurationValue === configurationValue);
        if (!config) {
            throw new Error('selectConfiguration error: configuration not found');
        }

        try {
            const setConfiguration = promisify(this.device.setConfiguration).bind(this.device);
            await setConfiguration(configurationValue);
        } catch (error) {
            throw new Error(`selectConfiguration error: ${error}`);
        }
    }

    public async claimInterface(interfaceNumber: number): Promise<void> {
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
                alternate: iface.alternate,
                alternates: iface.alternates,
                claimed: true
            };
        } catch (error) {
            throw new Error(`claimInterface error: ${error}`);
        }
    }

    public async releaseInterface(interfaceNumber: number): Promise<void> {
        await this._releaseInterface(interfaceNumber);

        if (this.configuration) {
            const iface = this.configuration.interfaces.find(usbInterface => usbInterface.interfaceNumber === interfaceNumber);
            if (iface) {
                // Re-create the USBInterface to set the claimed attribute
                this.configuration.interfaces[this.configuration.interfaces.indexOf(iface)] = {
                    interfaceNumber,
                    alternate: iface.alternate,
                    alternates: iface.alternates,
                    claimed: false
                };
            }
        }
    }

    public async selectAlternateInterface(interfaceNumber: number, alternateSetting: number): Promise<void> {
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
    }

    public async controlTransferIn(setup: USBControlTransferParameters, length: number): Promise<USBInTransferResult> {
        try {
            const type = this.controlTransferParamsToType(setup, usb.LIBUSB_ENDPOINT_IN);
            const controlTransfer = promisify(this.device.controlTransfer).bind(this.device);
            const result = await controlTransfer(type, setup.request, setup.value, setup.index, length);

            return {
                data: result ? new DataView(new Uint8Array(result as Buffer).buffer) : undefined,
                status: 'ok'
            };
        } catch (error) {
            if (error.errno === usb.LIBUSB_TRANSFER_STALL) {
                return {
                    status: 'stall'
                };
            }

            if (error.errno === usb.LIBUSB_TRANSFER_OVERFLOW) {
                return {
                    status: 'babble'
                };
            }

            throw new Error(`controlTransferIn error: ${error}`);
        }
    }

    public async controlTransferOut(setup: USBControlTransferParameters, data?: ArrayBuffer): Promise<USBOutTransferResult> {
        try {
            const type = this.controlTransferParamsToType(setup, usb.LIBUSB_ENDPOINT_OUT);
            const controlTransfer = promisify(this.device.controlTransfer).bind(this.device);
            const buffer = data ? Buffer.from(data) : Buffer.alloc(0);
            const bytesWritten = <number>await controlTransfer(type, setup.request, setup.value, setup.index, buffer);

            return {
                bytesWritten,
                status: 'ok'
            };
        } catch (error) {
            if (error.errno === usb.LIBUSB_TRANSFER_STALL) {
                return {
                    bytesWritten: 0,
                    status: 'stall'
                };
            }

            throw new Error(`controlTransferOut error: ${error}`);
        }
    }

    public async clearHalt(direction: USBDirection, endpointNumber: number): Promise<void> {
        try {
            const wIndex = endpointNumber | (direction === 'in' ? usb.LIBUSB_ENDPOINT_IN : usb.LIBUSB_ENDPOINT_OUT);
            const controlTransfer = promisify(this.device.controlTransfer).bind(this.device);
            await controlTransfer(usb.LIBUSB_RECIPIENT_ENDPOINT, CLEAR_FEATURE, ENDPOINT_HALT, wIndex, 0);
        } catch (error) {
            throw new Error(`clearHalt error: ${error}`);
        }
    }

    public async transferIn(endpointNumber: number, length: number): Promise<USBInTransferResult> {
        try {
            const endpoint = this.getEndpoint(endpointNumber | usb.LIBUSB_ENDPOINT_IN) as InEndpoint;
            const transfer = promisify(endpoint.transfer).bind(endpoint);
            const result = await transfer(length);

            return {
                data: result ? new DataView(new Uint8Array(result).buffer) : undefined,
                status: 'ok'
            };
        } catch (error) {
            if (error.errno === usb.LIBUSB_TRANSFER_STALL) {
                return {
                    status: 'stall'
                };
            }

            if (error.errno === usb.LIBUSB_TRANSFER_OVERFLOW) {
                return {
                    status: 'babble'
                };
            }

            throw new Error(`transferIn error: ${error}`);
        }
    }

    public async transferOut(endpointNumber: number, data: ArrayBuffer): Promise<USBOutTransferResult> {
        try {
            const endpoint = this.getEndpoint(endpointNumber | usb.LIBUSB_ENDPOINT_OUT) as OutEndpoint;
            const transfer = promisify(endpoint.transfer).bind(endpoint);
            const buffer = Buffer.from(data);
            const bytesWritten = await transfer(buffer);

            return {
                bytesWritten,
                status: 'ok'
            };
        } catch (error) {
            if (error.errno === usb.LIBUSB_TRANSFER_STALL) {
                return {
                    bytesWritten: 0,
                    status: 'stall'
                };
            }

            throw new Error(`transferOut error: ${error}`);
        }
    }

    public async reset(): Promise<void> {
        try {
            const reset = promisify(this.device.reset).bind(this.device);
            await reset();
        } catch (error) {
            throw new Error(`reset error: ${error}`);
        }
    }

    public async isochronousTransferIn(_endpointNumber: number, _packetLengths: number[]): Promise<USBIsochronousInTransferResult> {
        throw new Error('isochronousTransferIn error: method not implemented');
    }

    public async isochronousTransferOut(_endpointNumber: number, _data: BufferSource, _packetLengths: number[]): Promise<USBIsochronousOutTransferResult> {
        throw new Error('isochronousTransferOut error: method not implemented');
    }

    public async forget(): Promise<void> {
        throw new Error('forget error: method not implemented');
    }

    private async initialize(): Promise<void> {
        try {
            if (!this.opened) {
                this.device.open();
            }

            this.manufacturerName = await this.getStringDescriptor(this.device.deviceDescriptor.iManufacturer);
            this.productName = await this.getStringDescriptor(this.device.deviceDescriptor.iProduct);
            this.serialNumber = await this.getStringDescriptor(this.device.deviceDescriptor.iSerialNumber);
            this.configurations = await this.getConfigurations();
        } catch (error) {
            throw new Error(`initialize error: ${error}`);
        } finally {
            if (this.opened) {
                this.device.close();
            }
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
                            direction: endpoint.bEndpointAddress & usb.LIBUSB_ENDPOINT_IN ? 'in' : 'out',
                            type: (endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) === usb.LIBUSB_TRANSFER_TYPE_BULK ? 'bulk'
                                : (endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) === usb.LIBUSB_TRANSFER_TYPE_INTERRUPT ? 'interrupt'
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
        const recipient = setup.recipient === 'device' ? usb.LIBUSB_RECIPIENT_DEVICE
            : setup.recipient === 'interface' ? usb.LIBUSB_RECIPIENT_INTERFACE
                : setup.recipient === 'endpoint' ? usb.LIBUSB_RECIPIENT_ENDPOINT
                    : usb.LIBUSB_RECIPIENT_OTHER;

        const requestType = setup.requestType === 'standard' ? usb.LIBUSB_REQUEST_TYPE_STANDARD
            : setup.requestType === 'class' ? usb.LIBUSB_REQUEST_TYPE_CLASS
                : usb.LIBUSB_REQUEST_TYPE_VENDOR;

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
