// process.argv is the module name, then a list of versions to compile for.
// check this (platform, arch) tuple matches,
// then check that the ABI for this version was compiled.
function nodeABI () {
	return process.versions.modules
		? 'node-v' + (+process.versions.modules)
		: process.versions.v8.match(/^3\.14\./)
			? 'node-v11'
			: 'v8-' + process.versions.v8.split('.').slice(0,2).join('.');
}
var platarch = process.platform + '-' + process.arch;
process.exit(
	process.argv.slice(2).indexOf(platarch) > -1
	&& require('fs').existsSync('node_modules/' + require('./package.json').name + '-shyp-' + platarch + '/' + nodeABI())
		? 0
		: 1);