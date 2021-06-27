import * as usb from './usb';
import { WebUSB } from './webusb';

declare module './usb' {
    const webusb: WebUSB;
}

Object.assign(usb, {
    webusb: new WebUSB()
});

export = usb;
