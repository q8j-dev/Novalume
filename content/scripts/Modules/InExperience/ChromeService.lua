-- State and integration registry for the in-experience Chrome. The recovered
-- Player separates registration from presentation so unavailable features do
-- not leave dead buttons in the Unibar or its submenu; this service preserves
-- that runtime contract without depending on the standalone app shell.

local ChromeService = {}
ChromeService.__index = ChromeService

local function compareIntegrations(a, b)
	if a.Order == b.Order then
		return a.Id < b.Id
	end
	return a.Order < b.Order
end

function ChromeService.new()
	local self = setmetatable({}, ChromeService)
	self.integrations = {}
	self.listeners = {}
	self.submenuOpen = false
	return self
end

function ChromeService:Register(integration)
	assert(type(integration) == "table", "Chrome integration must be a table")
	assert(type(integration.Id) == "string" and integration.Id ~= "", "Chrome integration requires an Id")
	assert(integration.Region == "Unibar" or integration.Region == "SubMenu", "Chrome integration requires a valid Region")
	assert(type(integration.Order) == "number", "Chrome integration requires an Order")
	assert(type(integration.Label) == "string", "Chrome integration requires a Label")
	assert(type(integration.Activated) == "function", "Chrome integration requires an Activated callback")
	assert(self.integrations[integration.Id] == nil, "duplicate Chrome integration: " .. integration.Id)

	self.integrations[integration.Id] = integration
	self:_Changed()
	return integration
end

function ChromeService:SetAvailable(id, available)
	local integration = assert(self.integrations[id], "unknown Chrome integration: " .. tostring(id))
	available = available == true
	if integration.Available ~= available then
		integration.Available = available
		self:_Changed()
	end
end

function ChromeService:SetActive(id, active)
	local integration = assert(self.integrations[id], "unknown Chrome integration: " .. tostring(id))
	active = active == true
	if integration.Active ~= active then
		integration.Active = active
		self:_Changed()
	end
end

function ChromeService:Get(region)
	local result = {}
	for _, integration in pairs(self.integrations) do
		if integration.Region == region and integration.Available ~= false then
			table.insert(result, integration)
		end
	end
	table.sort(result, compareIntegrations)
	return result
end

function ChromeService:IsSubMenuOpen()
	return self.submenuOpen
end

function ChromeService:SetSubMenuOpen(open)
	open = open == true
	if self.submenuOpen ~= open then
		self.submenuOpen = open
		self:_Changed()
	end
end

function ChromeService:Activate(id)
	local integration = assert(self.integrations[id], "unknown Chrome integration: " .. tostring(id))
	if integration.Available == false then
		return
	end
	integration.Activated(integration)
	self:_Changed()
end

function ChromeService:Subscribe(listener)
	assert(type(listener) == "function", "Chrome listener must be a function")
	table.insert(self.listeners, listener)
	local connected = true
	return function()
		if not connected then
			return
		end
		connected = false
		for index, candidate in ipairs(self.listeners) do
			if candidate == listener then
				table.remove(self.listeners, index)
				break
			end
		end
	end
end

function ChromeService:_Changed()
	for _, listener in ipairs(self.listeners) do
		listener()
	end
end

return ChromeService
