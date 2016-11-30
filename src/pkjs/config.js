module.exports = [
	{ 
    	"type": "heading", 
    	"defaultValue": "Settings",
		  "size": 1
	},
	{
		"type": "section",
		"items": [
			{
				"type": "heading",
				"defaultValue": "Color Selection"
			},
			{
				"type": "toggle",
				"messageKey": "KEY_INVERT_COLORS",
				"label": "Invert Colors",
				"defaultValue": false
			}
		]
	},
	{
		"type": "submit",
		"defaultValue": "Apply Settings"
	},
	{
		"type": "text",
		"defaultValue": "Version 1.7"
	},
  {
    "type": "text",
    "defaultValue": "<a href='http://openweathermap.org/'>Powered by Open Weather Map</a>"
  }
];

