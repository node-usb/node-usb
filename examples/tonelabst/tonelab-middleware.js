var TonelabMiddleware = function() {
	var Validator = require('validator').Validator;

	Validator.prototype.isBetween = function(min, max) {
		if (this.str < min || this.str > max) {
			this.error(this.msg || this.str + ' is out of bounds');
		}	
		
		return this;
	}

	this.validator = new Validator();

	this.validator.check = function(str, fail_msg) {
		this.str = String(str || '');
		this.msg = fail_msg;
		this.argumentStack = new Array();
		for (var key in arguments) {
			if (key >= 2) {
				this.argumentStack.push(arguments[key]);
			}
		}
		
		return this;
	}
}

TonelabMiddleware.prototype = {
	amps: [
			{id: "0", name: "Clean"},
			{id: "1", name: "Cali Clean"},
			{id: "2", name: "US Blues"},
			{id: "3", name: "US 2x12"},
			{id: "4", name: "VOX AC15"},	
			{id: "5", name: "VOX AC30"}, 
			{id: "6", name: "UK Rock"}, 
			{id: "7", name: "UK Metal"}, 
			{id: "8", name: "US High Gain"}, 
			{id: "9", name: "US Metal"}, 
			{id: "10", name: "Boutique Metal"}],

	amp_types: [
			{id: "0", name: "Standard"},
			{id: "1", name: "Special"},
			{id: "2", name: "Custom"}],


	reverb_effects: [
			{id: "0", name: "Spring"},
			{id: "1", name: "Room"},
			{id: "2", name: "Hall"}],

	pedal_effects: [
			{id: "0", name: "Comp"},
			{id: "1", name: "Acoustic"},
			{id: "2", name: "Vox Wah"},
			{id: "3", name: "U-Vibe"},
			{id: "4", name: "Octave"},
			{id: "5", name: "Treble Boost"},
			{id: "6", name: "Tube OD"},
			{id: "7", name: "Boutique"},
			{id: "8", name: "Orange Dist"},
			{id: "9", name: "Metal Dist"},
			{id: "10", name: "Fuzz"}],

	mod_delay_effects: [
			{id: "0", name: "Classic Chorus"},
			{id: "1", name: "Multi Tap Chorus"},
			{id: "2", name: "Classic Flanger"},
			{id: "3", name: "Phaser"},
			{id: "4", name: "Textrem"},
			{id: "5", name: "Rotary"},
			{id: "6", name: "Pitch Shifter"},
			{id: "7", name: "Filtron"},
			{id: "8", name: "Echo Plus"},
			{id: "9", name: "Delay"},
			{id: "10", name: "Chorus+Delay"}],
		
	cabinets: [
			{id: "0", name: "Tweed 1x8"}, 
			{id: "1", name: "Tweed 1x12"}, 
			{id: "2", name: "Tweed 4x10"}, 
	 		{id: "3", name: "Black 2x10"}, 
			{id: "4", name: "Black 2x12"}, 
			{id: "5", name: "VOX AC15"}, 
			{id: "6", name: "VOX AC30"}, 
			{id: "7", name: "VOX AD120VTX"}, 
			{id: "8", name: "UK H30 4x12"}, 
			{id: "9", name: "UK T75 4x12"}, 
			{id: "10", name: "US V30 4x12"}],


	getDefaults: function() {
		return {
			"amps": this.amps, 
			"amp_types": this.amp_types, 
			"cabinets": this.cabinets, 
			"mod_delay_effects": this.mod_delay_effects, 
			"pedal_effects": this.pedal_effects, 
			"reverb_effects": this.reverb_effects
		};
	},

	validateSettings: function(_settings) {
		// Validation
		this.validator.errorStack = new Array();

		this.validator.error = function(msg) {
			this.errorStack.push({object: this.argumentStack[0], errmsg: msg}); 
		}

		this.validator.check(_settings.amp, 'Amp must be between 0 and 10', "amp").isInt().isBetween(0,10);
		this.validator.check(_settings.amp_type, 'Amp type must be between 0 and 2', "amp_type").isInt().isBetween(0,2);
		this.validator.check(_settings.preset, 'Preset must be between 0 and 99', "preset").isInt().isBetween(0,99);
		this.validator.check(_settings.gain, 'Gain must be between 0 and 50', "gain").isInt().isBetween(0,50);
		this.validator.check(_settings.treble, 'Treble must be between 0 and 50', "treble").isInt().isBetween(0,50);
		this.validator.check(_settings.middle, 'Middle must be between 0 and 50', "middle").isInt().isBetween(0,50);
		this.validator.check(_settings.bass, 'Bass must be between 0 and 50', "bass").isInt().isBetween(0,50);
		this.validator.check(_settings.presence, 'Presence must be between 0 and 50', "presence").isInt().isBetween(0,50);
		this.validator.check(_settings.volume, 'Volume must be between 0 and 50', "gain").isInt().isBetween(0,50);

		this.validator.check(_settings.cabinet_on_off, 'Cabinet on/off must be between 0 and 1', "cabinet_on_off").isInt().isBetween(0,1);
		this.validator.check(_settings.cabinet, 'Cabinet must be between 0 and 10', "gain").isInt().isBetween(0,10);

		this.validator.check(_settings.reverb_effect_on_off, 'Reverb effect on/off must be between 0 and 1', "reverb_effect_on_off").isInt().isBetween(0,1);
		this.validator.check(_settings.reverb_effect, 'Reverb effect must be between 0 and 2', "reverb_effect").isInt().isBetween(0,2);
		this.validator.check(_settings.reverb_value, 'Reverb value must be between 0 and 40', "reverb_value").isInt().isBetween(0,40);

		this.validator.check(_settings.pedal_effect_on_off, 'Pedal effect on/off must be between 0 and 1', "pedal_effect_on_off").isInt().isBetween(0,1);
		this.validator.check(_settings.pedal_effect, 'Pedal effect must be between 0 and 10', "pedal_effect").isInt().isBetween(0,10);
		this.validator.check(_settings.pedal_value, 'Pedal value must be between 0 and 40', "pedal_value").isInt().isBetween(0,40);

		this.validator.check(_settings.mod_delay_on_off, 'Mod/Delay on/off must be between 0 and 1', "mod_delay_on_off").isInt().isBetween(0,1);
		this.validator.check(_settings.mod_delay_effect, 'Mod/Delay effect must be between 0 and 10', "mod_delay_effect").isInt().isBetween(0,10);
		this.validator.check(_settings.mod_delay_depth, 'Mod/Delay depth must be between 0 and 100', "mod_delay_depth").isInt().isBetween(0,100);
		this.validator.check(_settings.mod_delay_feedback, 'Mod/Delay feedback must be between 0 and 100', "mod_delay_feedback").isInt().isBetween(0,100);

		return this.validator.errorStack;
	},

	buildPresetData: function(_settings) {
		// Build preset data
		var presetData = new Object();
		var settingBitField = 0;

		if (_settings.pedal_effect_on_off == 1) {
			settingBitField += 1;
		}

		if (_settings.cabinet_on_off == 1) {
			settingBitField += 4;
		}

		if (_settings.mod_delay_on_off == 1) {
			settingBitField += 8;
		}

		if (_settings.pedal_reverb_on_off == 1) {
			settingBitField += 32;
		}


		presetData.push(_settings.settingBitField); 
		presetData.push(_settings.pedal_value); 
		presetData.push(_settings.amp * (_settings.amp_type + 1));
		presetData.push(_settings.gain); 
		presetData.push(_settings.volume); 
		presetData.push(_settings.treble); 
		presetData.push(_settings.middle); 
		presetData.push(_settings.bass); 
		presetData.push(_settings.presence); 
		presetData.push(_settings.preset); 
		presetData.push(_settings.cabinet);
		presetData.push(_settings.reverb_effect); 
		presetData.push(_settings.reverb_value); 
		S0
		presetData.push(_settings.mod_delay_effect); 
		presetData.push(_settings.delay_depth); 
		presetData.push(_settings.delay_feedback); 

		return presetData;
	},
};

exports.TonelabMiddleware = TonelabMiddleware;
