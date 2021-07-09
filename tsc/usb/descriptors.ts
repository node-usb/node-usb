/** A structure representing the standard USB device descriptor */
export interface DeviceDescriptor {
    /** Size of this descriptor (in bytes) */
    bLength: number;

    /** Descriptor type. */
    bDescriptorType: number;

    /** USB specification release number in binary-coded decimal. */
    bcdUSB: number;

    /** USB-IF class code for the device. */
    bDeviceClass: number;

    /** USB-IF subclass code for the device, qualified by the bDeviceClass value. */
    bDeviceSubClass: number;

    /** USB-IF protocol code for the device, qualified by the bDeviceClass and bDeviceSubClass values. */
    bDeviceProtocol: number;

    /** Maximum packet size for endpoint 0. */
    bMaxPacketSize0: number;

    /** USB-IF vendor ID. */
    idVendor: number;

    /** USB-IF product ID. */
    idProduct: number;

    /** Device release number in binary-coded decimal. */
    bcdDevice: number;

    /** Index of string descriptor describing manufacturer. */
    iManufacturer: number;

    /** Index of string descriptor describing product. */
    iProduct: number;

    /** Index of string descriptor containing device serial number. */
    iSerialNumber: number;

    /** Number of possible configurations. */
    bNumConfigurations: number;
}

/** A structure representing the standard USB configuration descriptor */
export interface ConfigDescriptor {
    /** Size of this descriptor (in bytes) */
    bLength: number;

    /** Descriptor type. */
    bDescriptorType: number;

    /** Total length of data returned for this configuration. */
    wTotalLength: number;

    /** Number of interfaces supported by this configuration. */
    bNumInterfaces: number;

    /** Identifier value for this configuration. */
    bConfigurationValue: number;

    /** Index of string descriptor describing this configuration. */
    iConfiguration: number;

    /** Configuration characteristics. */
    bmAttributes: number;

    /** Maximum power consumption of the USB device from this bus in this configuration when the device is fully operation. */
    bMaxPower: number;

    /** Extra descriptors. */
    extra: Buffer;

    /** Array of interfaces supported by this configuration. */
    interfaces: InterfaceDescriptor[][];
}

/** A structure representing the standard USB interface descriptor */
export interface InterfaceDescriptor {
    /** Size of this descriptor (in bytes) */
    bLength: number;

    /** Descriptor type. */
    bDescriptorType: number;

    /** Number of this interface. */
    bInterfaceNumber: number;

    /** Value used to select this alternate setting for this interface. */
    bAlternateSetting: number;

    /** Number of endpoints used by this interface (excluding the control endpoint). */
    bNumEndpoints: number;

    /** USB-IF class code for this interface. */
    bInterfaceClass: number;

    /** USB-IF subclass code for this interface, qualified by the bInterfaceClass value. */
    bInterfaceSubClass: number;

    /** USB-IF protocol code for this interface, qualified by the bInterfaceClass and bInterfaceSubClass values. */
    bInterfaceProtocol: number;

    /** Index of string descriptor describing this interface. */
    iInterface: number;

    /** Extra descriptors. */
    extra: Buffer;

    /** Array of endpoint descriptors. */
    endpoints: EndpointDescriptor[];
}

/** A structure representing the standard USB endpoint descriptor */
export interface EndpointDescriptor {
    /** Size of this descriptor (in bytes) */
    bLength: number;

    /** Descriptor type. */
    bDescriptorType: number;

    /** The address of the endpoint described by this descriptor. */
    bEndpointAddress: number;

    /** Attributes which apply to the endpoint when it is configured using the bConfigurationValue. */
    bmAttributes: number;

    /** Maximum packet size this endpoint is capable of sending/receiving. */
    wMaxPacketSize: number;

    /** Interval for polling endpoint for data transfers. */
    bInterval: number;

    /** For audio devices only: the rate at which synchronization feedback is provided. */
    bRefresh: number;

    /** For audio devices only: the address if the synch endpoint. */
    bSynchAddress: number;

    /**
     * Extra descriptors.
     *
     * If libusb encounters unknown endpoint descriptors, it will store them here, should you wish to parse them.
     */
    extra: Buffer;
}

/** A generic representation of a BOS Device Capability descriptor */
export interface CapabilityDescriptor {
    /** Size of this descriptor (in bytes) */
    bLength: number;

    /** Descriptor type. */
    bDescriptorType: number;

    /** Device Capability type. */
    bDevCapabilityType: number;

    /** Device Capability data (bLength - 3 bytes) */
    dev_capability_data: Buffer;
}

/** A structure representing the Binary Device Object Store (BOS) descriptor */
export interface BosDescriptor {
    /** Size of this descriptor (in bytes) */
    bLength: number;

    /** Descriptor type. */
    bDescriptorType: number;

    /** Length of this descriptor and all of its sub descriptors. */
    wTotalLength: number;

    /** The number of separate device capability descriptors in the BOS. */
    bNumDeviceCaps: number;

    /** Device Capability Descriptors */
    capabilities: CapabilityDescriptor[];
}
