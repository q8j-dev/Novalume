-- In-experience Player chrome. This intentionally uses the compatibility GUI
-- primitives available in this source tree while preserving the 2026 Player's
-- measured layout and runtime-selected fallback assets.

local CoreGui = game:GetService("CoreGui")
local Players = game:GetService("Players")
local StarterGui = game:GetService("StarterGui")
local UserInputService = game:GetService("UserInputService")

local RobloxGui = CoreGui:WaitForChild("RobloxGui")
local LocalPlayer = Players.LocalPlayer
while LocalPlayer == nil do
	wait()
	LocalPlayer = Players.LocalPlayer
end

local TOP_MARGIN = 10
local CELL_SIZE = 42
local ICON_SIZE = 22
local CELL_GAP = 6
local PANEL_WIDTH = 285
local PANEL_HEADER_HEIGHT = 42
local PANEL_ROW_HEIGHT = 42
local PANEL_PADDING = 10

local BACKGROUND = Color3.new(25 / 255, 27 / 255, 29 / 255)
local BACKGROUND_HOVER = Color3.new(55 / 255, 57 / 255, 60 / 255)
local TEXT = Color3.new(1, 1, 1)
local TEXT_MUTED = Color3.new(189 / 255, 190 / 255, 192 / 255)

local legacyObjects = {
	PlayerListContainer = true,
	Backpack = true,
	SettingsShield = true,
	DropDownFullscreenFrame = true,
	HurtOverlay = true,
	TextLabel = true,
}

local function hideLegacyObject(instance)
	if legacyObjects[instance.Name] then
		pcall(function()
			instance.Visible = false
		end)
	end
end

for _, child in pairs(RobloxGui:GetChildren()) do
	hideLegacyObject(child)
end
RobloxGui.ChildAdded:connect(hideLegacyObject)

local function create(className, properties)
	local instance = Instance.new(className)
	for name, value in pairs(properties) do
		instance[name] = value
	end
	return instance
end

local root = create("Frame", {
	Name = "InExperienceUI",
	Size = UDim2.new(1, 0, 1, 0),
	BackgroundTransparency = 1,
	Parent = RobloxGui,
})

local chrome = create("Frame", {
	Name = "UnibarFrame",
	Size = UDim2.new(0, CELL_SIZE * 3 + CELL_GAP * 2, 0, CELL_SIZE),
	Position = UDim2.new(0, TOP_MARGIN, 0, TOP_MARGIN),
	BackgroundTransparency = 1,
	Parent = root,
})

local function makeChromeButton(name, order, image)
	local button = create("ImageButton", {
		Name = name,
		Size = UDim2.new(0, CELL_SIZE, 0, CELL_SIZE),
		Position = UDim2.new(0, order * (CELL_SIZE + CELL_GAP), 0, 0),
		BackgroundColor3 = BACKGROUND,
		BackgroundTransparency = 1,
		BorderSizePixel = 0,
		AutoButtonColor = false,
		Image = "rbxasset://textures/ui/TopBar/iconBase.png",
		Parent = chrome,
	})
	local icon = create("ImageLabel", {
		Name = "Icon",
		Size = UDim2.new(0, ICON_SIZE, 0, ICON_SIZE),
		Position = UDim2.new(0.5, -ICON_SIZE / 2, 0.5, -ICON_SIZE / 2),
		BackgroundTransparency = 1,
		Image = image,
		Parent = button,
	})
	button.MouseEnter:connect(function()
		button.BackgroundColor3 = BACKGROUND_HOVER
	end)
	button.MouseLeave:connect(function()
		button.BackgroundColor3 = BACKGROUND
	end)
	return button, icon
end

local menuButton = makeChromeButton(
	"RobloxMenu",
	0,
	"rbxasset://textures/ui/TopBar/coloredlogo.png"
)
local chatButton = makeChromeButton(
	"Chat",
	1,
	"rbxasset://textures/ui/TopBar/chatOff.png"
)
local playerListButton = makeChromeButton(
	"PlayerList",
	2,
	"rbxasset://textures/ui/TopBar/leaderboardOff.png"
)

local playerList = create("Frame", {
	Name = "PlayerList",
	Size = UDim2.new(0, PANEL_WIDTH, 0, PANEL_HEADER_HEIGHT + PANEL_ROW_HEIGHT + PANEL_PADDING),
	Position = UDim2.new(1, -PANEL_WIDTH - TOP_MARGIN, 0, TOP_MARGIN),
	BackgroundColor3 = BACKGROUND,
	BackgroundTransparency = 0.12,
	BorderSizePixel = 0,
	Parent = root,
})

create("TextLabel", {
	Name = "Title",
	Size = UDim2.new(1, -PANEL_PADDING * 2, 0, PANEL_HEADER_HEIGHT),
	Position = UDim2.new(0, PANEL_PADDING, 0, 0),
	BackgroundTransparency = 1,
	Text = "People",
	TextColor3 = TEXT,
	TextXAlignment = Enum.TextXAlignment.Left,
	TextYAlignment = Enum.TextYAlignment.Center,
	Font = Enum.Font.SourceSansBold,
	FontSize = Enum.FontSize.Size18,
	Parent = playerList,
})

local playerRow = create("Frame", {
	Name = "LocalPlayer",
	Size = UDim2.new(1, -PANEL_PADDING * 2, 0, PANEL_ROW_HEIGHT),
	Position = UDim2.new(0, PANEL_PADDING, 0, PANEL_HEADER_HEIGHT),
	BackgroundTransparency = 1,
	Parent = playerList,
})

local avatar = create("ImageLabel", {
	Name = "Avatar",
	Size = UDim2.new(0, 30, 0, 30),
	Position = UDim2.new(0, 0, 0.5, -15),
	BackgroundColor3 = Color3.new(70 / 255, 73 / 255, 77 / 255),
	BorderSizePixel = 0,
	Image = "rbxasset://textures/ui/PlayerList/ViewAvatar.png",
	Parent = playerRow,
})

create("TextLabel", {
	Name = "PlayerName",
	Size = UDim2.new(1, -42, 1, 0),
	Position = UDim2.new(0, 42, 0, 0),
	BackgroundTransparency = 1,
	Text = LocalPlayer.Name,
	TextColor3 = TEXT,
	TextXAlignment = Enum.TextXAlignment.Left,
	TextYAlignment = Enum.TextYAlignment.Center,
	Font = Enum.Font.SourceSans,
	FontSize = Enum.FontSize.Size18,
	Parent = playerRow,
})

local chat = create("Frame", {
	Name = "ExperienceChat",
	Size = UDim2.new(0, 360, 0, 180),
	Position = UDim2.new(0, TOP_MARGIN, 0, TOP_MARGIN + CELL_SIZE + CELL_GAP),
	BackgroundColor3 = BACKGROUND,
	BackgroundTransparency = 0.18,
	BorderSizePixel = 0,
	Visible = false,
	Parent = root,
})

create("TextLabel", {
	Name = "EmptyState",
	Size = UDim2.new(1, -24, 1, -58),
	Position = UDim2.new(0, 12, 0, 8),
	BackgroundTransparency = 1,
	Text = "Chat messages will appear here",
	TextColor3 = TEXT_MUTED,
	TextXAlignment = Enum.TextXAlignment.Left,
	TextYAlignment = Enum.TextYAlignment.Top,
	Font = Enum.Font.SourceSans,
	FontSize = Enum.FontSize.Size18,
	Parent = chat,
})

local chatInput = create("TextBox", {
	Name = "ChatInput",
	Size = UDim2.new(1, -24, 0, 38),
	Position = UDim2.new(0, 12, 1, -48),
	BackgroundColor3 = Color3.new(48 / 255, 50 / 255, 53 / 255),
	BackgroundTransparency = 0,
	BorderSizePixel = 0,
	Text = "",
	TextColor3 = TEXT,
	TextXAlignment = Enum.TextXAlignment.Left,
	Font = Enum.Font.SourceSans,
	FontSize = Enum.FontSize.Size18,
	Parent = chat,
})

local menu = create("Frame", {
	Name = "ExperienceMenu",
	Size = UDim2.new(0, 520, 0, 360),
	Position = UDim2.new(0.5, -260, 0.5, -180),
	BackgroundColor3 = BACKGROUND,
	BackgroundTransparency = 0.04,
	BorderSizePixel = 0,
	Visible = false,
	Parent = root,
})

create("TextLabel", {
	Name = "Title",
	Size = UDim2.new(1, -40, 0, 58),
	Position = UDim2.new(0, 20, 0, 0),
	BackgroundTransparency = 1,
	Text = "Menu",
	TextColor3 = TEXT,
	TextXAlignment = Enum.TextXAlignment.Left,
	Font = Enum.Font.SourceSansBold,
	FontSize = Enum.FontSize.Size24,
	Parent = menu,
})

local resume = create("TextButton", {
	Name = "Resume",
	Size = UDim2.new(1, -40, 0, 48),
	Position = UDim2.new(0, 20, 0, 72),
	BackgroundColor3 = Color3.new(57 / 255, 59 / 255, 63 / 255),
	BorderSizePixel = 0,
	Text = "Resume",
	TextColor3 = TEXT,
	Font = Enum.Font.SourceSansBold,
	FontSize = Enum.FontSize.Size18,
	Parent = menu,
})

local menuVisible = false
local function setMenuVisible(visible)
	menuVisible = visible
	menu.Visible = visible
end

menuButton.MouseButton1Click:connect(function()
	setMenuVisible(not menuVisible)
end)
resume.MouseButton1Click:connect(function()
	setMenuVisible(false)
end)
chatButton.MouseButton1Click:connect(function()
	chat.Visible = not chat.Visible
	if chat.Visible then
		chatInput:CaptureFocus()
	end
end)
playerListButton.MouseButton1Click:connect(function()
	playerList.Visible = not playerList.Visible
end)

UserInputService.InputBegan:connect(function(input, processed)
	if not processed and input.KeyCode == Enum.KeyCode.Escape then
		setMenuVisible(not menuVisible)
	end
end)

pcall(function()
	StarterGui:SetCoreGuiEnabled(Enum.CoreGuiType.Health, false)
end)
