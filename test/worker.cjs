const parentPort = require('worker_threads').parentPort
const findByIds = require('../dist').findByIds;

const device = findByIds(0x59e3, 0x0a23);
if (!device) {
    console.error('No test device connected, tests require this device to be present');
    return;
}
device.open();
device.getStringDescriptor(device.deviceDescriptor.iSerialNumber, (_, serial) => {
    parentPort?.postMessage(serial);
    device.close();
});
