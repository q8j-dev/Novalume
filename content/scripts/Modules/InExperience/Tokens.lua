-- Desktop in-experience token subset consumed by Chrome and Unibar.
-- Values are taken from the Player's Foundation/UIBlox dark-theme contracts.

local Tokens = {
	Size = {
		Size_600 = 24,
		Size_700 = 28,
		Size_900 = 36,
		Size_1100 = 44,
		Size_1400 = 56,
	},
	Gap = {
		XSmall = 4,
		Small = 8,
		Medium = 12,
		Large = 16,
		XLarge = 20,
		XXLarge = 24,
	},
	Padding = {
		XXSmall = 2,
		XSmall = 4,
		Small = 8,
		Medium = 12,
		Large = 16,
		XLarge = 20,
		XXLarge = 24,
	},
	Radius = {
		XSmall = 2,
		Small = 4,
		Medium = 8,
		Large = 16,
	},
	Theme = {
		BackgroundUIContrast = {
			Color = Color3.fromRGB(0, 0, 0),
			Transparency = 0.3,
		},
		BackgroundOnHover = {
			Color = Color3.fromRGB(255, 255, 255),
			Transparency = 0.9,
		},
		IconEmphasis = {
			Color = Color3.fromRGB(255, 255, 255),
			Transparency = 0,
		},
		Divider = {
			Color = Color3.fromRGB(255, 255, 255),
			Transparency = 0.8,
		},
		MenuIconBackground = {
			Color = Color3.fromRGB(18, 18, 21),
			Transparency = 0.08,
		},
	},
}

return Tokens
