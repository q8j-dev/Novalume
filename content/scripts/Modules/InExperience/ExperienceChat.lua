-- Functional backport of the Player ExperienceChat window. The presentation
-- uses the recovered ExpChat desktop configuration while message transport is
-- connected to the networking/chat surface provided by this engine revision.

local UserInputService = game:GetService("UserInputService")
local Players = game:GetService("Players")

local ExperienceChat = {}

local CHAT_POSITION_X = 8
local CHAT_POSITION_Y = 62 -- 58px top-bar inset plus the recovered 4px layout offset
local CHAT_WIDTH_SCALE = 0.4
local CHAT_HEIGHT_SCALE = 0.25
local CHAT_MAX_WIDTH = 475
local CHAT_MAX_HEIGHT = 275
local CHAT_WINDOW_TEXT_SIZE = 18
local CHAT_INPUT_TEXT_SIZE = 14
local CHAT_INPUT_HEIGHT = 32
local CHAT_SECTION_GAP = 8
local CHAT_PADDING = 8
local CHAT_BACKGROUND = Color3.new(25 / 255, 27 / 255, 29 / 255)
local CHAT_WINDOW_BACKGROUND_TRANSPARENCY = 0.3
local CHAT_INPUT_BACKGROUND_TRANSPARENCY = 0.2
local CHAT_BORDER_TRANSPARENCY = 0.8
local CHAT_PLACEHOLDER = "To chat click here or press \"/\" key"
local MAX_MESSAGES = 50

local NAME_COLORS = {
	Color3.new(253 / 255, 41 / 255, 67 / 255),
	Color3.new(1 / 255, 162 / 255, 255 / 255),
	Color3.new(2 / 255, 184 / 255, 87 / 255),
	BrickColor.new("Alder").Color,
	BrickColor.new("Bright orange").Color,
	BrickColor.new("Bright yellow").Color,
	BrickColor.new("Light reddish violet").Color,
	BrickColor.new("Brick yellow").Color,
}

local function create(className, properties)
	local instance = Instance.new(className)
	for name, value in pairs(properties) do
		instance[name] = value
	end
	return instance
end

local function addCorner(parent, radius)
	return create("UICorner", {
		CornerRadius = UDim.new(0, radius),
		Parent = parent,
	})
end

local function nameValue(name)
	local value = 0
	for index = 1, #name do
		local byte = string.byte(string.sub(name, index, index))
		local distance = #name - index + 1
		if #name % 2 == 1 then
			distance = distance - 1
		end
		if distance % 4 >= 2 then
			byte = -byte
		end
		value = value + byte
	end
	return value
end

local function getNameColor(player)
	if player and player.Team then
		return player.TeamColor.Color
	end
	local name = player and player.Name or ""
	return NAME_COLORS[(nameValue(name) % #NAME_COLORS) + 1]
end

local function disconnect(connection)
	if connection then
		connection:disconnect()
	end
end

function ExperienceChat.Mount(parent)
	assert(parent ~= nil, "ExperienceChat.Mount requires a parent")

	local localPlayer = Players.LocalPlayer
	while localPlayer == nil do
		Players.ChildAdded:wait()
		localPlayer = Players.LocalPlayer
	end

	local root = create("Frame", {
		Name = "ExperienceChat",
		Position = UDim2.new(0, CHAT_POSITION_X, 0, CHAT_POSITION_Y),
		Size = UDim2.new(CHAT_WIDTH_SCALE, 0, CHAT_HEIGHT_SCALE, 0),
		BackgroundTransparency = 1,
		BorderSizePixel = 0,
		Visible = false,
		Parent = parent,
	})
	create("UISizeConstraint", {
		MinSize = Vector2.new(180, 96),
		MaxSize = Vector2.new(CHAT_MAX_WIDTH, CHAT_MAX_HEIGHT),
		Parent = root,
	})

	local window = create("Frame", {
		Name = "ChatWindow",
		Size = UDim2.new(1, 0, 1, -(CHAT_INPUT_HEIGHT + CHAT_SECTION_GAP)),
		BackgroundColor3 = CHAT_BACKGROUND,
		BackgroundTransparency = CHAT_WINDOW_BACKGROUND_TRANSPARENCY,
		BorderSizePixel = 0,
		ClipsDescendants = true,
		Parent = root,
	})
	addCorner(window, 8)

	local messageView = create("ScrollingFrame", {
		Name = "MessageView",
		Size = UDim2.new(1, -CHAT_PADDING * 2, 1, -CHAT_PADDING * 2),
		Position = UDim2.new(0, CHAT_PADDING, 0, CHAT_PADDING),
		BackgroundTransparency = 1,
		BorderSizePixel = 0,
		CanvasSize = UDim2.new(0, 0, 0, 0),
		ScrollBarThickness = 8,
		Parent = window,
	})

	local inputBorder = create("Frame", {
		Name = "InputBorder",
		Size = UDim2.new(1, 0, 0, CHAT_INPUT_HEIGHT),
		Position = UDim2.new(0, 0, 1, -CHAT_INPUT_HEIGHT),
		BackgroundColor3 = Color3.new(1, 1, 1),
		BackgroundTransparency = CHAT_BORDER_TRANSPARENCY,
		BorderSizePixel = 0,
		Parent = root,
	})
	addCorner(inputBorder, CHAT_INPUT_HEIGHT / 2)
	local inputBackground = create("Frame", {
		Name = "InputBackground",
		Size = UDim2.new(1, -2, 1, -2),
		Position = UDim2.new(0, 1, 0, 1),
		BackgroundColor3 = CHAT_BACKGROUND,
		BackgroundTransparency = CHAT_INPUT_BACKGROUND_TRANSPARENCY,
		BorderSizePixel = 0,
		Parent = inputBorder,
	})
	addCorner(inputBackground, (CHAT_INPUT_HEIGHT - 2) / 2)
	local input = create("TextBox", {
		Name = "ChatInputBar",
		Size = UDim2.new(1, -CHAT_PADDING * 2, 1, 0),
		Position = UDim2.new(0, CHAT_PADDING, 0, 0),
		BackgroundTransparency = 1,
		BorderSizePixel = 0,
		ClearTextOnFocus = false,
		Font = Enum.Font.BuilderSansMedium,
		Text = CHAT_PLACEHOLDER,
		TextColor3 = Color3.new(178 / 255, 178 / 255, 178 / 255),
		TextSize = CHAT_INPUT_TEXT_SIZE,
		TextStrokeColor3 = Color3.new(0, 0, 0),
		TextStrokeTransparency = 0.5,
		TextXAlignment = Enum.TextXAlignment.Left,
		TextYAlignment = Enum.TextYAlignment.Center,
		Parent = inputBackground,
	})

	local messages = {}
	local visible = false
	local placeholderVisible = true
	local activityGeneration = 0
	local visibilityListeners = {}

	local function layoutMessages()
		local y = 0
		for _, message in ipairs(messages) do
			message.Frame.Position = UDim2.new(0, 0, 0, y)
			local height = math.max(CHAT_WINDOW_TEXT_SIZE + 4, message.Body.TextBounds.Y + 4)
			message.Frame.Size = UDim2.new(1, -2, 0, height)
			message.Body.Size = UDim2.new(1, 0, 0, height)
			message.Prefix.Size = UDim2.new(1, 0, 0, height)
			y = y + height
		end
		messageView.CanvasSize = UDim2.new(0, 0, 0, y)
		local viewHeight = messageView.AbsoluteSize.Y
		messageView.CanvasPosition = Vector2.new(0, math.max(0, y - viewHeight))
	end

	local function noteActivity()
		activityGeneration = activityGeneration + 1
		local generation = activityGeneration
		window.BackgroundTransparency = CHAT_WINDOW_BACKGROUND_TRANSPARENCY
		inputBackground.BackgroundTransparency = CHAT_INPUT_BACKGROUND_TRANSPARENCY
		for _, message in ipairs(messages) do
			message.Body.TextTransparency = 0
			message.Prefix.TextTransparency = 0
		end
		spawn(function()
			wait(3.5)
			if generation == activityGeneration and visible then
				window.BackgroundTransparency = 1
			end
			wait(26.5)
			if generation == activityGeneration and visible then
				for _, message in ipairs(messages) do
					message.Body.TextTransparency = 1
					message.Prefix.TextTransparency = 1
				end
			end
		end)
	end

	local function addMessage(player, text, playerChatType)
		if type(text) ~= "string" or text == "" then
			return
		end
		local prefix = player and (player.Name .. ":") or ""
		if playerChatType == Enum.PlayerChatType.Team then
			prefix = "[Team] " .. prefix
		elseif playerChatType == Enum.PlayerChatType.Whisper then
			prefix = "[Whisper] " .. prefix
		end
		local combined = prefix == "" and text or (prefix .. " " .. text)
		local frame = create("Frame", {
			Name = "Message",
			BackgroundTransparency = 1,
			BorderSizePixel = 0,
			Parent = messageView,
		})
		local body = create("TextLabel", {
			Name = "Body",
			BackgroundTransparency = 1,
			BorderSizePixel = 0,
			Font = Enum.Font.BuilderSansMedium,
			Text = combined,
			TextColor3 = Color3.new(1, 1, 1),
			TextSize = CHAT_WINDOW_TEXT_SIZE,
			TextStrokeColor3 = Color3.new(0, 0, 0),
			TextStrokeTransparency = 0.5,
			TextWrapped = true,
			TextXAlignment = Enum.TextXAlignment.Left,
			TextYAlignment = Enum.TextYAlignment.Top,
			Parent = frame,
		})
		local prefixLabel = create("TextLabel", {
			Name = "Prefix",
			BackgroundTransparency = 1,
			BorderSizePixel = 0,
			Font = Enum.Font.BuilderSansMedium,
			Text = prefix,
			TextColor3 = getNameColor(player),
			TextSize = CHAT_WINDOW_TEXT_SIZE,
			TextStrokeColor3 = Color3.new(0, 0, 0),
			TextStrokeTransparency = 0.5,
			TextWrapped = true,
			TextXAlignment = Enum.TextXAlignment.Left,
			TextYAlignment = Enum.TextYAlignment.Top,
			ZIndex = 2,
			Parent = frame,
		})
		table.insert(messages, { Frame = frame, Body = body, Prefix = prefixLabel })
		if #messages > MAX_MESSAGES then
			local removed = table.remove(messages, 1)
			removed.Frame:Destroy()
		end
		spawn(function()
			wait()
			layoutMessages()
		end)
		noteActivity()
	end

	local function setPlaceholder(show)
		placeholderVisible = show
		if show then
			input.Text = CHAT_PLACEHOLDER
			input.TextColor3 = Color3.new(178 / 255, 178 / 255, 178 / 255)
		else
			input.Text = ""
			input.TextColor3 = Color3.new(1, 1, 1)
		end
	end

	local function sendInput()
		local text = placeholderVisible and "" or input.Text
		text = string.match(text, "^%s*(.-)%s*$") or ""
		if text ~= "" then
			localPlayer:Chat(text)
		end
		setPlaceholder(true)
		noteActivity()
	end

	input.Focused:connect(function()
		if placeholderVisible then
			setPlaceholder(false)
		end
		noteActivity()
	end)
	input.FocusLost:connect(function(enterPressed)
		if enterPressed then
			sendInput()
		elseif input.Text == "" then
			setPlaceholder(true)
		end
	end)

	local inputConnection = UserInputService.InputBegan:connect(function(inputObject, processed)
		if not processed and inputObject.KeyCode == Enum.KeyCode.Slash then
			if not visible then
				visible = true
				root.Visible = true
			end
			input:CaptureFocus()
			noteActivity()
		end
	end)
	local chatConnection = Players.PlayerChatted:connect(function(playerChatType, sendingPlayer, text)
		addMessage(sendingPlayer, text, playerChatType)
	end)

	local controller = {}
	function controller:SetVisible(value)
		value = value == true
		if visible == value then
			return
		end
		visible = value
		root.Visible = value
		if value then
			noteActivity()
		end
		for _, listener in ipairs(visibilityListeners) do
			listener(value)
		end
	end
	function controller:Toggle()
		self:SetVisible(not visible)
	end
	function controller:IsVisible()
		return visible
	end
	function controller:SubscribeVisibility(listener)
		table.insert(visibilityListeners, listener)
	end
	function controller:Destroy()
		disconnect(inputConnection)
		disconnect(chatConnection)
		root:Destroy()
	end
	controller.Root = root
	controller.AddMessage = addMessage
	return controller
end

ExperienceChat.Config = {
	ChatLayoutPosition = UDim2.new(0, 8, 0, 4),
	ChatWindowSize = UDim2.new(0.4, 0, 0.25, 0),
	ChatWindowMaxWidth = CHAT_MAX_WIDTH,
	ChatWindowMaxHeight = CHAT_MAX_HEIGHT,
	ChatWindowTextSize = CHAT_WINDOW_TEXT_SIZE,
	ChatInputBarTextSize = CHAT_INPUT_TEXT_SIZE,
	ChatWindowBackgroundTransparency = CHAT_WINDOW_BACKGROUND_TRANSPARENCY,
	ChatInputBarBackgroundTransparency = CHAT_INPUT_BACKGROUND_TRANSPARENCY,
}

return ExperienceChat
