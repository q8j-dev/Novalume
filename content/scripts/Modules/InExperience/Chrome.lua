-- In-experience desktop Chrome and Unibar presentation. Geometry, colors,
-- icon names, ordering behavior, and submenu placement are backported from
-- the Player's Chrome/TopBar modules. This is an imperative renderer for the
-- same runtime contracts because this engine predates the React host used by
-- the supplied Player package.

local BuilderIcon = require(script.Parent.BuilderIcon)
local IntegrationIcons = require(script.Parent.IntegrationIcons)
local Tokens = require(script.Parent.Tokens)

local Chrome = {}

local TOP_BAR_HEIGHT = 58
local TOP_BAR_BUTTON_HEIGHT = 44
local TOP_BAR_TOP_MARGIN = 10
local SCREEN_SIDE_OFFSET = 16
local TOP_BAR_GAP = 8
local ICON_CELL_WIDTH = 44
local ICON_HIGHLIGHT_SIZE = 36
local ICON_SIZE = 36
local UNIBAR_END_PADDING = 4
local UNIBAR_LEFT_MARGIN = 8
local MENU_SUBMENU_PADDING = 8
local SUB_MENU_ROW_HEIGHT = 56
local SUBMENU_CORNER_RADIUS = 10
local SUBMENU_PADDING_LEFT = 8
local SUBMENU_PADDING_RIGHT = 8
local SUBMENU_ROW_PADDING = 8
local SUBMENU_ROW_CORNER_RADIUS = 10
local SUBMENU_BOTTOM_PADDING = 20
local SUBMENU_WIDTH = ICON_CELL_WIDTH * 4 + UNIBAR_LEFT_MARGIN + UNIBAR_END_PADDING * 2
local SUBMENU_X_OFFSET = -TOP_BAR_HEIGHT - 2 + UNIBAR_LEFT_MARGIN

local function create(className, properties)
	local instance = Instance.new(className)
	for name, value in pairs(properties) do
		instance[name] = value
	end
	return instance
end

local function addCorner(parent, radius)
	return create("UICorner", {
		Name = "Corner",
		CornerRadius = UDim.new(0, radius),
		Parent = parent,
	})
end

local function iconFor(integration)
	if integration.Active and integration.ActiveIcon then
		return integration.ActiveIcon
	end
	return integration.Icon
end

local function setIcon(iconLabel, definition)
	iconLabel.Text = definition.Name
	BuilderIcon.SetVariant(iconLabel, definition.Variant)
end

local function createIconButton(parent, name, position, definition, activated)
	local control = { Selected = false }
	local cell = create("Frame", {
		Name = name,
		Size = UDim2.new(0, ICON_CELL_WIDTH, 0, ICON_CELL_WIDTH),
		Position = position,
		BackgroundTransparency = 1,
		BorderSizePixel = 0,
		Parent = parent,
	})
	local highlight = create("Frame", {
		Name = "Highlighter",
		Size = UDim2.new(0, ICON_HIGHLIGHT_SIZE, 0, ICON_HIGHLIGHT_SIZE),
		Position = UDim2.new(0.5, -ICON_HIGHLIGHT_SIZE / 2, 0.5, -ICON_HIGHLIGHT_SIZE / 2),
		BackgroundColor3 = Tokens.Theme.BackgroundOnHover.Color,
		BackgroundTransparency = 1,
		BorderSizePixel = 0,
		Parent = cell,
	})
	addCorner(highlight, ICON_HIGHLIGHT_SIZE / 2)

	local icon = BuilderIcon.Create(definition.Name, definition.Variant, ICON_SIZE, {
		Name = "Icon",
		Color3 = Tokens.Theme.IconEmphasis.Color,
		Transparency = Tokens.Theme.IconEmphasis.Transparency,
		ZIndex = 2,
	})
	icon.Position = UDim2.new(0.5, -ICON_SIZE / 2, 0.5, -ICON_SIZE / 2)
	icon.Parent = cell

	local hitArea = create("TextButton", {
		Name = "IconHitArea_" .. name,
		Size = UDim2.new(1, 0, 1, 0),
		BackgroundTransparency = 1,
		BorderSizePixel = 0,
		Text = "",
		ZIndex = 3,
		Parent = cell,
	})
	hitArea.MouseEnter:connect(function()
		highlight.BackgroundTransparency = Tokens.Theme.BackgroundOnHover.Transparency
	end)
	hitArea.MouseLeave:connect(function()
		highlight.BackgroundTransparency = control.Selected and Tokens.Theme.BackgroundOnHover.Transparency or 1
	end)
	hitArea.MouseButton1Click:connect(activated)

	control.Cell = cell
	control.Icon = icon
	control.Highlight = highlight
	control.HitArea = hitArea
	return control
end

local function updateIconButton(button, integration, selected)
	setIcon(button.Icon, iconFor(integration))
	button.Selected = selected == true
	button.Highlight.BackgroundTransparency = selected and Tokens.Theme.BackgroundOnHover.Transparency or 1
end

local function destroyChildren(parent)
	for _, child in pairs(parent:GetChildren()) do
		child:Destroy()
	end
end

function Chrome.Mount(parent, service, menuActivated)
	assert(parent ~= nil, "Chrome.Mount requires a parent")
	assert(service ~= nil, "Chrome.Mount requires a ChromeService")
	assert(type(menuActivated) == "function", "Chrome.Mount requires a menu callback")

	local root = create("Frame", {
		Name = "InExperienceChrome",
		Size = UDim2.new(1, 0, 0, TOP_BAR_HEIGHT),
		Position = UDim2.new(0, 0, 0, 0),
		BackgroundTransparency = 1,
		BorderSizePixel = 0,
		ZIndex = 6,
		Parent = parent,
	})

	local menuCell = create("Frame", {
		Name = "RobloxMenu",
		Size = UDim2.new(0, TOP_BAR_BUTTON_HEIGHT, 0, TOP_BAR_BUTTON_HEIGHT),
		Position = UDim2.new(0, SCREEN_SIDE_OFFSET, 0, TOP_BAR_TOP_MARGIN),
		BackgroundColor3 = Tokens.Theme.MenuIconBackground.Color,
		BackgroundTransparency = Tokens.Theme.MenuIconBackground.Transparency,
		BorderSizePixel = 0,
		Parent = root,
	})
	addCorner(menuCell, TOP_BAR_BUTTON_HEIGHT / 2)
	local menuIcon = BuilderIcon.Create(IntegrationIcons.RobloxMenu.Name, IntegrationIcons.RobloxMenu.Variant, 24, {
		Name = "Tilt",
		ZIndex = 2,
	})
	menuIcon.Position = UDim2.new(0.5, -12, 0.5, -12)
	menuIcon.Parent = menuCell
	local menuHitArea = create("TextButton", {
		Name = "RobloxMenuHitArea",
		Size = UDim2.new(1, 0, 1, 0),
		BackgroundTransparency = 1,
		BorderSizePixel = 0,
		Text = "",
		ZIndex = 3,
		Parent = menuCell,
	})
	menuHitArea.MouseButton1Click:connect(menuActivated)

	local unibar = create("Frame", {
		Name = "UnibarMenu",
		Position = UDim2.new(0, SCREEN_SIDE_OFFSET + TOP_BAR_BUTTON_HEIGHT + TOP_BAR_GAP, 0, TOP_BAR_TOP_MARGIN),
		Size = UDim2.new(0, 0, 0, ICON_CELL_WIDTH),
		BackgroundColor3 = Tokens.Theme.BackgroundUIContrast.Color,
		BackgroundTransparency = Tokens.Theme.BackgroundUIContrast.Transparency,
		BorderSizePixel = 0,
		Parent = root,
	})
	addCorner(unibar, ICON_CELL_WIDTH / 2)

	local submenu = create("Frame", {
		Name = "NineDotSubMenu",
		Position = UDim2.new(0, SCREEN_SIDE_OFFSET, 0, TOP_BAR_TOP_MARGIN + ICON_CELL_WIDTH + MENU_SUBMENU_PADDING),
		Size = UDim2.new(0, SUBMENU_WIDTH, 0, 0),
		BackgroundColor3 = Tokens.Theme.BackgroundUIContrast.Color,
		BackgroundTransparency = Tokens.Theme.BackgroundUIContrast.Transparency,
		BorderSizePixel = 0,
		Visible = false,
		Parent = root,
	})
	addCorner(submenu, SUBMENU_CORNER_RADIUS)

	local unibarButtons = {}
	local submenuRows = {}

	local function renderSubmenu()
		destroyChildren(submenu)
		addCorner(submenu, SUBMENU_CORNER_RADIUS)
		submenuRows = {}
		local items = service:Get("SubMenu")
		submenu.Size = UDim2.new(0, SUBMENU_WIDTH, 0, #items * SUB_MENU_ROW_HEIGHT + SUBMENU_BOTTOM_PADDING)

		for index, integration in ipairs(items) do
			local row = create("TextButton", {
				Name = integration.Id,
				Size = UDim2.new(1, 0, 0, SUB_MENU_ROW_HEIGHT),
				Position = UDim2.new(0, 0, 0, (index - 1) * SUB_MENU_ROW_HEIGHT),
				BackgroundColor3 = Tokens.Theme.BackgroundOnHover.Color,
				BackgroundTransparency = 1,
				BorderSizePixel = 0,
				Text = "",
				Parent = submenu,
			})
			addCorner(row, SUBMENU_ROW_CORNER_RADIUS)
			local icon = BuilderIcon.Create(iconFor(integration).Name, iconFor(integration).Variant, ICON_SIZE, {
				Name = "Icon",
				ZIndex = 2,
			})
			icon.Position = UDim2.new(0, SUBMENU_PADDING_LEFT, 0.5, -ICON_SIZE / 2)
			icon.Parent = row
			local labelX = SUBMENU_PADDING_LEFT + ICON_SIZE + SUBMENU_ROW_PADDING
			local label = create("TextLabel", {
				Name = "Label",
				Size = UDim2.new(1, -labelX - SUBMENU_PADDING_RIGHT, 1, 0),
				Position = UDim2.new(0, labelX, 0, 0),
				BackgroundTransparency = 1,
				BorderSizePixel = 0,
				Text = integration.Label,
				TextColor3 = Color3.new(1, 1, 1),
				TextTransparency = 0,
				TextXAlignment = Enum.TextXAlignment.Left,
				TextYAlignment = Enum.TextYAlignment.Center,
				Font = Enum.Font.BuilderSansBold,
				TextSize = 20.16,
				ZIndex = 2,
				Parent = row,
			})
			row.MouseEnter:connect(function()
				row.BackgroundTransparency = Tokens.Theme.BackgroundOnHover.Transparency
			end)
			row.MouseLeave:connect(function()
				row.BackgroundTransparency = 1
			end)
			row.MouseButton1Click:connect(function()
				service:SetSubMenuOpen(false)
				service:Activate(integration.Id)
			end)
			submenuRows[integration.Id] = { Row = row, Icon = icon, Label = label }
		end
	end

	local function renderUnibar()
		destroyChildren(unibar)
		addCorner(unibar, ICON_CELL_WIDTH / 2)
		unibarButtons = {}
		local items = service:Get("Unibar")
		unibar.Size = UDim2.new(0, UNIBAR_END_PADDING * 2 + #items * ICON_CELL_WIDTH, 0, ICON_CELL_WIDTH)
		for index, integration in ipairs(items) do
			local button = createIconButton(
				unibar,
				integration.Id,
				UDim2.new(0, UNIBAR_END_PADDING + (index - 1) * ICON_CELL_WIDTH, 0, 0),
				iconFor(integration),
				function()
					service:Activate(integration.Id)
				end
			)
			unibarButtons[integration.Id] = button
		end
	end

	local function refresh()
		renderUnibar()
		renderSubmenu()
		local open = service:IsSubMenuOpen()
		submenu.Visible = open
		local more = service.integrations.nine_dot
		if more and unibarButtons.nine_dot then
			local definition = open and IntegrationIcons.Close or IntegrationIcons.More
			setIcon(unibarButtons.nine_dot.Icon, definition)
			unibarButtons.nine_dot.Selected = open
			unibarButtons.nine_dot.Highlight.BackgroundTransparency = open and Tokens.Theme.BackgroundOnHover.Transparency or 1
		end
		for id, button in pairs(unibarButtons) do
			local integration = service.integrations[id]
			if integration and id ~= "nine_dot" then
				updateIconButton(button, integration, integration.Active == true)
			end
		end
		for id, row in pairs(submenuRows) do
			local integration = service.integrations[id]
			if integration then
				setIcon(row.Icon, iconFor(integration))
			end
		end
	end

	local unsubscribe = service:Subscribe(refresh)
	refresh()

	return {
		Root = root,
		Refresh = refresh,
		Destroy = function()
			unsubscribe()
			root:Destroy()
		end,
	}
end

Chrome.Constants = {
	TopBarHeight = TOP_BAR_HEIGHT,
	TopBarButtonHeight = TOP_BAR_BUTTON_HEIGHT,
	TopBarTopMargin = TOP_BAR_TOP_MARGIN,
	ScreenSideOffset = SCREEN_SIDE_OFFSET,
	IconCellWidth = ICON_CELL_WIDTH,
	IconHighlightSize = ICON_HIGHLIGHT_SIZE,
	SubMenuRowHeight = SUB_MENU_ROW_HEIGHT,
	SubMenuWidth = SUBMENU_WIDTH,
	SubMenuBottomPadding = SUBMENU_BOTTOM_PADDING,
	SubMenuXOffset = SUBMENU_X_OFFSET,
}

return Chrome
