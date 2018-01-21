var app = new Moon({
  el: "#main",
  data: {
    counter: 0,
    cfg: [],
    settings: [],
    formEditable: false,
    active: 0
  },
  hooks: {
  	init: function() {
  		var me = this;
  		ajax("cfgmanifest.json", {
			success: function(r){
  				var cfg = JSON.parse(r.responseText);
				console.log(cfg);
				me.set("cfg", cfg);
				me.callMethod("fetchSettings");
  			},
  			data: ""
		});
  	}
  },
  methods: {
  	fetchSettings: function() {
  		var me = this;
  		ajax("config.json", {
			success: function(r){
  				var op = JSON.parse(r.responseText);
  				var model = JSON.parse(r.responseText);
  				me.set("settings", op);
  				me.callMethod("mergeSettings");
			},
			data: ""
		})
  	},
  	mergeSettings: function() {
  		var cfg = this.get("cfg");
		var s = this.get("settings");
		for (var i = 0; i < cfg.length; i++) {
		for (var j = 0; j < cfg[i].options.length; j++) {
				var id = cfg[i].options[j].id;
				cfg[i].options[j].value = s[id];
			}
		}
		this.set("cfg", cfg);
  	},
  	setActive: function(index) {
  		this.set("active", index);	
  	},
  	setFormEditable: function(val) {
  		this.set('formEditable', val);
  	},
  	formCancel: function() {
		this.callMethod("mergeSettings");
  		this.set('formEditable', false);
  		this.set('counter', this.get('counter') + 1);
  	},
  	formConfirm: function() {
		this.set('formEditable', false);
  	}
  }
});




