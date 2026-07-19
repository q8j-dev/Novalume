-- Builder Icons adapter for the in-experience Player UI.
-- The 2026 package renders icon names as ligatures from the shipped regular
-- and filled Builder Icons faces. Keeping that contract here lets CoreGui use
-- the original glyphs without raster replacements.

local BuilderIcon = {}

local function getFont(variant)
	if variant == "Filled" then
		return Enum.Font.BuilderIconsFilled
	end
	return Enum.Font.BuilderIconsRegular
end

function BuilderIcon.Create(name, variant, size, style)
	assert(type(name) == "string" and name ~= "", "BuilderIcon.Create requires an icon name")
	assert(variant == nil or variant == "Regular" or variant == "Filled", "invalid Builder Icons variant")
	assert(type(size) == "number" and size > 0, "BuilderIcon.Create requires a positive size")

	style = style or {}
	local icon = Instance.new("TextLabel")
	icon.Name = style.Name or "BuilderIcon"
	icon.BackgroundTransparency = 1
	icon.BorderSizePixel = 0
	icon.Size = UDim2.new(0, size, 0, size)
	icon.Text = name
	icon.Font = getFont(variant)
	icon.TextScaled = true
	icon.TextColor3 = style.Color3 or Color3.new(1, 1, 1)
	icon.TextTransparency = style.Transparency or 0
	icon.TextStrokeTransparency = 1
	icon.TextXAlignment = Enum.TextXAlignment.Center
	icon.TextYAlignment = Enum.TextYAlignment.Center
	icon.ZIndex = style.ZIndex or 1
	icon.Visible = style.Visible ~= false
	return icon
end

function BuilderIcon.SetVariant(icon, variant)
	assert(icon and icon:IsA("TextLabel"), "BuilderIcon.SetVariant requires a TextLabel")
	icon.Font = getFont(variant)
end

return BuilderIcon
