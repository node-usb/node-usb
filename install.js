const fs = require('fs');
const os = require('os');
const https = require('https');
const pkgJson = require('./package.json');
const path = require('path');

const platform = process.env.PLATFORM || os.platform();
const arch = process.env.ARCH || os.arch();
const installElectron =
  typeof process.env.INSTALL_ELECTRON !== 'undefined'
  ? !! parseInt(process.env.INSTALL_ELECTRON, 10)
  : true;
const installNode =
  typeof process.env.INSTALL_NODE !== 'undefined'
  ? !! parseInt(process.env.INSTALL_NODE, 10)
  : true;


function deleteFolderRecursive(path) {
  if( fs.existsSync(path) ) {
    fs.readdirSync(path).forEach(function(file,index){
      var curPath = [path, file].join('/');
      if(fs.lstatSync(curPath).isDirectory()) { // recurse
        deleteFolderRecursive(curPath);
      } else { // delete file
        fs.unlinkSync(curPath);
      }
    });
    fs.rmdirSync(path);
  }
};

function downloadBinary(abiName, destFile) {
  var binaryName = [[abiName, pkgJson.version, platform, arch].join('-'), 'node'].join('.');
  var file = fs.createWriteStream(destFile);

  return new Promise(function(resolve, reject) {
    var url = [pkgJson.binary.host, binaryName].join('');
    console.log('Downloading ', url, ' -> ', destFile);
    const cb = (response) => {
      const statusCode = response.statusCode;
      if (statusCode === 200) {
        response.pipe(file);
        file.on('finish', function() {
            resolve();
        });
        file.on('error', function(e) {
          reject(e);
        });
      } else {
        reject([url, statusCode]);
      }
    }

    https.get(url, cb).on('error', function (e) {
      console.log(`Got error: ${e.message}`);
      console.log(`Retrying: ${e.message}`);
      https.get(url, cb).on('error', function (e) {
        console.log(`Got error on retry attempt: ${e.message}`);
        reject(e);
      });
    });
  });
}

var prebuildsDir = path.join(__dirname, 'prebuilds');
var binaryDir = path.join(prebuildsDir, [platform, arch].join('-'));

//deleteFolderRecursive(prebuildsDir);
if (!fs.existsSync(prebuildsDir)) {
  fs.mkdirSync(prebuildsDir);
}
if (!fs.existsSync(binaryDir)) {
  fs.mkdirSync(binaryDir);
}

const promises = [];
if (installNode) {
  for (const abiVersion of pkgJson.binary.builtABIs['node']) {
    const abiName = ['node', abiVersion].join('-');
    const destFile = path.join(binaryDir,
      [abiName, 'node'].join('.')
    );
    if (!fs.existsSync(destFile)) {
      promises.push(downloadBinary(abiName, destFile));
    }
  }
}

if (installElectron) {
  for (const abiVersion of pkgJson.binary.builtABIs['electron']) {
    const abiName = ['electron', abiVersion].join('-');
    const destFile = path.join(binaryDir,
      [abiName, 'node'].join('.')
    );
    if (!fs.existsSync(destFile)) {
      promises.push(downloadBinary(abiName, destFile));
    }
  }
}

Promise.all(promises).then(function() {
  console.log('Binaries installed');
}).catch(function(e) {
  console.log('Error ocurred:', e);
});
