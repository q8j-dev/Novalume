/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#include "V8Tree/Instance.h"
#include "RbxAssert.h"
#include "V8Xml/Serializer.h"
#include "V8Tree/Service.h"
#include "util/standardout.h"
#include "V8DataModel/ScriptService.h"
#include "V8DataModel/DataModel.h"
#include "security/ApiSecurity.h"
#include "v8datamodel/HackDefines.h"
#include "v8datamodel/NumberSequence.h"
#include "v8datamodel/ColorSequence.h"
#include "util/Font.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
#include "security/ProtectionMarkers.h"
#endif

LOGVARIABLE(InstanceTreeManipulation, 0)

namespace {
thread_local unsigned int destroyingSuppressionDepth = 0;

class ScopedDestroyingSuppression
{
public:
	explicit ScopedDestroyingSuppression(bool suppress)
		: active(suppress)
	{
		if (active)
			++destroyingSuppressionDepth;
	}

	~ScopedDestroyingSuppression()
	{
		if (active)
			--destroyingSuppressionDepth;
	}

private:
	bool active;
};
}

DYNAMIC_FASTFLAGVARIABLE(LockViolationInstanceCrash, false)

namespace RBX {

///////////////////////////////////

const char* const sInstance = "Instance";

REFLECTION_BEGIN();
Reflection::PropDescriptor<Instance, bool> Instance::propArchivable("Archivable", category_Behavior, &Instance::getIsArchivable,  &Instance::setIsArchivable, Reflection::PropertyDescriptor::SCRIPTING);
Reflection::PropDescriptor<Instance, BinaryString>
	Instance::propAttributesSerialize("AttributesSerialize", category_Data,
		&Instance::getSerializedAttributes, &Instance::setSerializedAttributes,
		Reflection::PropertyDescriptor::STREAMING);
Reflection::PropDescriptor<Instance, bool> propArchivableDeprecated("archivable", category_Behavior, &Instance::getIsArchivable,  &Instance::setIsArchivable, Reflection::PropertyDescriptor::LEGACY_SCRIPTING);
static Reflection::BoundFuncDesc<Instance, bool(std::string)> func_isA(&Instance::isA, "IsA",  "className", Security::None);
static Reflection::BoundFuncDesc<Instance, bool(std::string)> func_isA_deprecated(&Instance::isA, "isA",  "className", Security::None);
static Reflection::BoundFuncDesc<Instance, shared_ptr<Instance>(std::string, bool)> findFirstChild(&Instance::findFirstChildByName2, "FindFirstChild", "name", "recursive", false, Security::None);
static Reflection::BoundFuncDesc<Instance, shared_ptr<Instance>(std::string, bool)> findFirstChildDeprecated(&Instance::findFirstChildByName2, "findFirstChild", "name", "recursive", false, Security::None);
static Reflection::BoundFuncDesc<Instance, shared_ptr<Instance>(std::string)> findFirstAncestor(&Instance::findFirstAncestor, "FindFirstAncestor", "name", Security::None);
static Reflection::BoundFuncDesc<Instance, shared_ptr<Instance>(std::string)> findFirstAncestorOfClass(&Instance::findFirstAncestorOfClass, "FindFirstAncestorOfClass", "className", Security::None);
static Reflection::BoundFuncDesc<Instance, shared_ptr<Instance>(std::string)> findFirstAncestorWhichIsA(&Instance::findFirstAncestorWhichIsA, "FindFirstAncestorWhichIsA", "className", Security::None);
static Reflection::BoundFuncDesc<Instance, shared_ptr<Instance>(std::string)> findFirstChildOfClass(&Instance::findFirstChildOfClass, "FindFirstChildOfClass", "className", Security::None);
static Reflection::BoundFuncDesc<Instance, shared_ptr<Instance>(std::string, bool)> findFirstChildWhichIsA(&Instance::findFirstChildWhichIsA, "FindFirstChildWhichIsA", "className", "recursive", false, Security::None);
static Reflection::BoundFuncDesc<Instance, shared_ptr<Instance>()> func_clone(&Instance::luaClone, "Clone", Security::None);
static Reflection::BoundFuncDesc<Instance, shared_ptr<Instance>()> func_cloneDeprecated(&Instance::luaClone, "clone", Security::None);
static Reflection::BoundFuncDesc<Instance, void()> func_Remove(&Instance::remove, "Remove", Security::None);
static Reflection::BoundFuncDesc<Instance, void()> func_RemoveDeprecated(&Instance::remove, "remove", Security::None);
static Reflection::BoundFuncDesc<Instance, void()> func_RemoveAllChildren(&Instance::destroyAllChildrenLua, "ClearAllChildren", Security::None);
static Reflection::BoundFuncDesc<Instance, void()> func_Destroy(&Instance::destroy, "Destroy", Security::None);
static Reflection::BoundFuncDesc<Instance, void()> dep_Destroy(&Instance::destroy, "destroy", Security::None, Reflection::PropertyDescriptor::Attributes::deprecated(func_Destroy));
static Reflection::BoundFuncDesc<Instance, shared_ptr<const Instances>()> func_childrenOld(&Instance::getChildren2, "children", Security::None);
static Reflection::BoundFuncDesc<Instance, shared_ptr<const Instances>()> func_childrenOld2(&Instance::getChildren2, "getChildren", Security::None);
static Reflection::BoundFuncDesc<Instance, shared_ptr<const Instances>()> func_children(&Instance::getChildren2, "GetChildren", Security::None);
static Reflection::BoundFuncDesc<Instance, shared_ptr<const Instances>()> func_descendants(&Instance::getDescendants, "GetDescendants", Security::None);
static Reflection::BoundFuncDesc<Instance, std::string()> func_GetFullName(&Instance::getFullNameForReflection, "GetFullName", Security::None);
static Reflection::BoundFuncDesc<Instance, Reflection::Variant(std::string)> func_GetAttribute(&Instance::getAttribute, "GetAttribute", "attribute", Security::None);
static Reflection::BoundFuncDesc<Instance, shared_ptr<const Reflection::ValueTable>()> func_GetAttributes(&Instance::getAttributes, "GetAttributes", Security::None);
static Reflection::BoundFuncDesc<Instance, void(std::string, Reflection::Variant)> func_SetAttribute(&Instance::setAttribute, "SetAttribute", "attribute", "value", Reflection::Variant(), Security::None);
static Reflection::BoundFuncDesc<Instance, void(std::string)> func_ResetPropertyToDefault(&Instance::resetPropertyToDefault, "ResetPropertyToDefault", "propertyName", Security::None);
static Reflection::BoundFuncDesc<Instance, Reflection::Variant(std::string, std::string)> func_GetStyled(
	&Instance::getStyled, "GetStyled", "name", "selector", std::string(), Security::None);
static Reflection::CustomBoundFuncDesc<Instance, Reflection::Variant(std::string)> func_GetAttributeChangedSignal(&Instance::getAttributeChangedSignal, "GetAttributeChangedSignal", "attribute", Security::None);
static Reflection::CustomBoundFuncDesc<Instance, Reflection::Variant(std::string)> func_GetPropertyChangedSignal(&Instance::getPropertyChangedSignal, "GetPropertyChangedSignal", "property", Security::None);
static Reflection::CustomBoundFuncDesc<Instance, Reflection::Variant(std::string)> func_GetStyledPropertyChangedSignal(
	&Instance::getPropertyChangedSignal, "GetStyledPropertyChangedSignal", "property", Security::None);
static Reflection::CustomBoundFuncDesc<Instance, void(std::string)> func_AddTag(&Instance::addTagLua, "AddTag", "tag", Security::None);
static Reflection::CustomBoundFuncDesc<Instance, void(std::string)> func_RemoveTag(&Instance::removeTagLua, "RemoveTag", "tag", Security::None);
static Reflection::CustomBoundFuncDesc<Instance, bool(std::string)> func_HasTag(&Instance::hasTagLua, "HasTag", "tag", Security::None);
static Reflection::CustomBoundFuncDesc<Instance, shared_ptr<const Reflection::ValueArray>()> func_GetTags(&Instance::getTagsLua, "GetTags", Security::None);
static Reflection::BoundFuncDesc<Instance, bool(shared_ptr<Instance>)> func_isDescendantOf(&Instance::isDescendantOf2, "IsDescendantOf", "ancestor", Security::None);
static Reflection::BoundFuncDesc<Instance, bool(shared_ptr<Instance>)> dep_isDescendantOf(&Instance::isDescendantOf2, "isDescendantOf", "ancestor", Security::None, Reflection::PropertyDescriptor::Attributes::deprecated(func_isDescendantOf));
static Reflection::BoundFuncDesc<Instance, bool(shared_ptr<Instance>)> func_isAncestorOf(&Instance::isAncestorOf2, "IsAncestorOf", "descendant", Security::None);
static Reflection::BoundFuncDesc<Instance, std::string(int)> func_GetReadableId(&Instance::getReadableDebugId, "GetDebugId", "scopeLength", 4, Security::Plugin);
static Reflection::PropDescriptor<Instance, std::string> prop_className("ClassName", category_Data, &Instance::getClassNameStr, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<Instance, std::string> prop_classNameDeprecated("className", category_Data, &Instance::getClassNameStr, NULL, Reflection::PropertyDescriptor::Attributes::deprecated(prop_className));
static Reflection::PropDescriptor<Instance, int> prop_dataCost("DataCost", category_Data, &Instance::getPersistentDataCost, NULL, Reflection::PropertyDescriptor::UI, Security::RobloxPlace);

static Reflection::BoundYieldFuncDesc<Instance, shared_ptr<Instance>(std::string)> func_WaitForChild(&Instance::waitForChild, "WaitForChild", "childName", Security::None);

const Reflection::PropDescriptor<Instance, std::string> Instance::desc_Name("Name", category_Data, &Instance::getName, &Instance::setName);
const Reflection::RefPropDescriptor<Instance, Instance> Instance::propParent("Parent", category_Data, &Instance::getParentDangerous, &Instance::setParent, Reflection::PropertyDescriptor::UI);
const Reflection::PropDescriptor<Instance, bool> Instance::propRobloxLocked("RobloxLocked", category_Data, &Instance::getRobloxLocked, &Instance::setRobloxLocked, Reflection::PropertyDescriptor::SCRIPTING, Security::Plugin);

Reflection::EventDesc<
	Instance, 
	void(shared_ptr<Instance>), 
	rbx::signal<void(shared_ptr<Instance>)>, 
	rbx::signal<void(shared_ptr<Instance>)>* (Instance::*)(bool)>
	event_childAdded(&Instance::getOrCreateChildAddedSignal, "ChildAdded", "child");

Reflection::EventDesc<
	Instance, 
	void(shared_ptr<Instance>), 
	rbx::signal<void(shared_ptr<Instance>)>, 
	rbx::signal<void(shared_ptr<Instance>)>* (Instance::*)(bool)>
	dep_childAdded(&Instance::getOrCreateChildAddedSignal, "childAdded", "child", Reflection::Descriptor::Attributes::deprecated(event_childAdded));

Reflection::EventDesc<
Instance, 
void(shared_ptr<Instance>), 
rbx::signal<void(shared_ptr<Instance>)>, 
rbx::signal<void(shared_ptr<Instance>)>* (Instance::*)(bool)>
event_childRemoved(&Instance::getOrCreateChildRemovedSignal, "ChildRemoved", "child");

Reflection::EventDesc<
Instance, 
void(shared_ptr<Instance>), 
rbx::signal<void(shared_ptr<Instance>)>, 
rbx::signal<void(shared_ptr<Instance>)>* (Instance::*)(bool)>
event_descendantAdded(&Instance::getOrCreateDescendantAddedSignal, "DescendantAdded", "descendant");

Reflection::EventDesc<
Instance, 
void(shared_ptr<Instance>), 
rbx::signal<void(shared_ptr<Instance>)>, 
rbx::signal<void(shared_ptr<Instance>)>* (Instance::*)(bool)>
event_descendantRemoving(&Instance::getOrCreateDescendantRemovingSignal, "DescendantRemoving", "descendant");

Reflection::EventDesc<
	Instance,
	void(),
	rbx::signal<void()>,
	rbx::signal<void()>* (Instance::*)(bool)>
event_destroying(&Instance::getOrCreateDestroyingSignal, "Destroying");

Reflection::EventDesc<Instance, void(shared_ptr<Instance>, shared_ptr<Instance>)> event_ancestryChanged(&Instance::ancestryChangedSignal, "AncestryChanged", "child", "parent");
Reflection::EventDesc<Instance, void(const Reflection::PropertyDescriptor*)> event_propertyChanged(&Instance::propertyChangedSignal, "Changed", "property");
Reflection::EventDesc<
	Instance,
	void(std::string),
	rbx::signal<void(std::string)>,
	rbx::signal<void(std::string)>* (Instance::*)(bool)>
	event_attributeChanged(&Instance::getOrCreateAttributeChangedSignal, "AttributeChanged", "attribute");
REFLECTION_END();

namespace {
void appendDescendants(const Instance& parent, Instances& result)
{
	shared_ptr<const Instances> children = parent.getChildren().read();
	if (!children)
		return;
	for (Instances::const_iterator it = children->begin(); it != children->end(); ++it)
	{
		result.push_back(*it);
		appendDescendants(**it, result);
	}
}
}

shared_ptr<const Instances> Instance::getDescendants()
{
	shared_ptr<Instances> result(new Instances());
	appendDescendants(*this, *result);
	return result;
}

const Reflection::EventDescriptor* Instance::getAttributeChangedEventDescriptor()
{
	return &event_attributeChanged;
}

const Reflection::EventDescriptor* Instance::getPropertyChangedEventDescriptor()
{
	return &event_propertyChanged;
}

bool Instance::addTagInternal(const std::string& tag)
{
	return tags.insert(tag).second;
}

bool Instance::removeTagInternal(const std::string& tag)
{
	return tags.erase(tag) != 0;
}

bool Instance::hasTagInternal(const std::string& tag) const
{
	return tags.find(tag) != tags.end();
}

bool Instance::isPropertyStyleManaged(const std::string& propertyName) const
{
	const OnDemandInstance* data = onDemandRead();
	return data && data->styleManagedProperties.find(propertyName) !=
		data->styleManagedProperties.end();
}

void Instance::markPropertyStyleManaged(const std::string& propertyName)
{
	onDemandWrite()->styleManagedProperties.insert(propertyName);
}

void Instance::clearPropertyStyleManaged(const std::string& propertyName)
{
	if (OnDemandInstance* data = onDemandPtr.get())
		data->styleManagedProperties.erase(propertyName);
}

namespace {

bool isValidAttributeName(const std::string& name)
{
	if (name.empty() || name.size() > 100)
		return false;
	for (std::string::const_iterator it = name.begin(); it != name.end(); ++it)
	{
		const unsigned char c = static_cast<unsigned char>(*it);
		if (!(std::isalnum(c) || c == '_' || c == '-'))
			return false;
	}
	return true;
}

bool isSupportedAttributeValue(const Reflection::Variant& value)
{
	return value.isVoid() || value.isType<bool>() || value.isType<double>() ||
		value.isType<std::string>() || value.isType<BrickColor>() ||
		value.isType<G3D::Color3>() || value.isType<G3D::Vector2>() ||
		value.isType<G3D::Vector3>() || value.isType<G3D::CoordinateFrame>() ||
		value.isType<UDim>() || value.isType<UDim2>() ||
		value.isType<NumberRange>() || value.isType<G3D::Rect2D>() ||
		value.isType<NumberSequence>() || value.isType<ColorSequence>() ||
		value.isType<Font>() || value.type().isEnum;
}

template<typename T>
bool equalIfType(const Reflection::Variant& left, const Reflection::Variant& right)
{
	return left.isType<T>() && right.isType<T>() && left.cast<T>() == right.cast<T>();
}

bool attributesEqual(const Reflection::Variant& left, const Reflection::Variant& right)
{
	if (left.isVoid() || right.isVoid())
		return left.isVoid() && right.isVoid();
	if (left.type() != right.type())
		return false;
	if (left.type().isEnum)
	{
		const Reflection::EnumDescriptor& descriptor = static_cast<const Reflection::EnumDescriptor&>(left.type());
		const Reflection::EnumDescriptor::Item* leftItem = descriptor.lookup(left);
		const Reflection::EnumDescriptor::Item* rightItem = descriptor.lookup(right);
		return leftItem && rightItem && leftItem->value == rightItem->value;
	}
	return equalIfType<bool>(left, right) || equalIfType<double>(left, right) ||
		equalIfType<std::string>(left, right) || equalIfType<BrickColor>(left, right) ||
		equalIfType<G3D::Color3>(left, right) || equalIfType<G3D::Vector2>(left, right) ||
		equalIfType<G3D::Vector3>(left, right) ||
		equalIfType<G3D::CoordinateFrame>(left, right) || equalIfType<UDim>(left, right) ||
		equalIfType<UDim2>(left, right) || equalIfType<NumberRange>(left, right) ||
		equalIfType<G3D::Rect2D>(left, right) || equalIfType<NumberSequence>(left, right) ||
		equalIfType<ColorSequence>(left, right) || equalIfType<Font>(left, right);
}

class AttributeReader
{
public:
	explicit AttributeReader(const std::string& bytes)
		: cursor(reinterpret_cast<const unsigned char*>(bytes.data()))
		, end(cursor + bytes.size())
	{
	}

	template<typename T>
	bool read(T& value)
	{
		if (static_cast<std::size_t>(end - cursor) < sizeof(T))
			return false;
		std::memcpy(&value, cursor, sizeof(T));
		cursor += sizeof(T);
		return true;
	}

	bool readString(std::string& value)
	{
		std::uint32_t length = 0;
		if (!read(length) || static_cast<std::size_t>(end - cursor) < length)
			return false;
		value.assign(reinterpret_cast<const char*>(cursor), length);
		cursor += length;
		return true;
	}

	bool finished() const { return cursor == end; }

private:
	const unsigned char* cursor;
	const unsigned char* end;
};

template<typename T>
void appendAttributeScalar(std::string& bytes, const T& value)
{
	const char* begin = reinterpret_cast<const char*>(&value);
	bytes.append(begin, sizeof(T));
}

void appendAttributeString(std::string& bytes, const std::string& value)
{
	if (value.size() > std::numeric_limits<std::uint32_t>::max())
		throw runtime_error("Attribute string exceeds the serialized size limit");
	appendAttributeScalar(bytes, static_cast<std::uint32_t>(value.size()));
	bytes.append(value);
}

bool decodeSerializedAttributes(const BinaryString& serialized,
	std::map<std::string, Reflection::Variant>& attributes)
{
	AttributeReader reader(serialized.value());
	std::uint32_t count = 0;
	if (!reader.read(count) || count > 1000000U)
		return serialized.value().empty();

	std::map<std::string, Reflection::Variant> decoded;
	for (std::uint32_t index = 0; index < count; ++index)
	{
		std::string name;
		std::uint8_t type = 0;
		if (!reader.readString(name) || !isValidAttributeName(name) ||
			!reader.read(type))
			return false;

		switch (type)
		{
		case 2: // String
		{
			std::string value;
			if (!reader.readString(value))
				return false;
			decoded[name] = value;
			break;
		}
		case 3: // Boolean
		{
			std::uint8_t value = 0;
			if (!reader.read(value) || value > 1)
				return false;
			decoded[name] = value != 0;
			break;
		}
		case 5: // Integer-valued Number
		{
			std::int32_t value = 0;
			if (!reader.read(value))
				return false;
			decoded[name] = static_cast<double>(value);
			break;
		}
		case 6: // Number
		{
			double value = 0.0;
			if (!reader.read(value) || !std::isfinite(value))
				return false;
			decoded[name] = value;
			break;
		}
		case 15: // Color3
		{
			float red = 0.0f;
			float green = 0.0f;
			float blue = 0.0f;
			if (!reader.read(red) || !reader.read(green) || !reader.read(blue) ||
				!std::isfinite(red) || !std::isfinite(green) || !std::isfinite(blue))
				return false;
			decoded[name] = G3D::Color3(red, green, blue);
			break;
		}
		case 17: // Vector3
		{
			float x = 0.0f;
			float y = 0.0f;
			float z = 0.0f;
			if (!reader.read(x) || !reader.read(y) || !reader.read(z) ||
				!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
				return false;
			decoded[name] = G3D::Vector3(x, y, z);
			break;
		}
		default:
			return false;
		}
	}

	if (!reader.finished())
		return false;
	attributes.swap(decoded);
	return true;
}

BinaryString encodeSerializedAttributes(
	const std::map<std::string, Reflection::Variant>& attributes)
{
	if (attributes.size() > std::numeric_limits<std::uint32_t>::max())
		throw runtime_error("Attribute count exceeds the serialized size limit");
	std::string bytes;
	appendAttributeScalar(bytes, static_cast<std::uint32_t>(attributes.size()));
	for (const auto& entry : attributes)
	{
		appendAttributeString(bytes, entry.first);
		const Reflection::Variant& value = entry.second;
		if (value.isType<std::string>())
		{
			appendAttributeScalar(bytes, static_cast<std::uint8_t>(2));
			appendAttributeString(bytes, value.cast<std::string>());
		}
		else if (value.isType<bool>())
		{
			appendAttributeScalar(bytes, static_cast<std::uint8_t>(3));
			appendAttributeScalar(bytes,
				static_cast<std::uint8_t>(value.cast<bool>() ? 1 : 0));
		}
		else if (value.isType<double>())
		{
			appendAttributeScalar(bytes, static_cast<std::uint8_t>(6));
			appendAttributeScalar(bytes, value.cast<double>());
		}
		else if (value.isType<G3D::Color3>())
		{
			appendAttributeScalar(bytes, static_cast<std::uint8_t>(15));
			const G3D::Color3 color = value.cast<G3D::Color3>();
			appendAttributeScalar(bytes, color.r);
			appendAttributeScalar(bytes, color.g);
			appendAttributeScalar(bytes, color.b);
		}
		else if (value.isType<G3D::Vector3>())
		{
			appendAttributeScalar(bytes, static_cast<std::uint8_t>(17));
			const G3D::Vector3 vector = value.cast<G3D::Vector3>();
			appendAttributeScalar(bytes, vector.x);
			appendAttributeScalar(bytes, vector.y);
			appendAttributeScalar(bytes, vector.z);
		}
		else
		{
			throw runtime_error("Unsupported attribute type %s for serialization",
				value.type().name.c_str());
		}
	}
	return BinaryString(bytes);
}

} // namespace

Reflection::Variant Instance::getAttribute(std::string attribute)
{
	const OnDemandInstance* data = onDemandRead();
	if (!data)
		return Reflection::Variant();
	std::map<std::string, Reflection::Variant>::const_iterator found = data->attributes.find(attribute);
	return found == data->attributes.end() ? Reflection::Variant() : found->second;
}

shared_ptr<const Reflection::ValueTable> Instance::getAttributes()
{
	shared_ptr<Reflection::ValueTable> result(new Reflection::ValueTable());
	if (const OnDemandInstance* data = onDemandRead())
		result->insert(data->attributes.begin(), data->attributes.end());
	return result;
}

void Instance::setAttribute(std::string attribute, Reflection::Variant value)
{
	if (!isValidAttributeName(attribute))
		throw runtime_error("Attribute name must contain only letters, numbers, underscores, and hyphens and be at most 100 characters");
	if (!isSupportedAttributeValue(value))
		throw runtime_error("Unsupported attribute type %s", value.type().name.c_str());

	OnDemandInstance* data = onDemandWrite();
	std::map<std::string, Reflection::Variant>::iterator found = data->attributes.find(attribute);
	if (value.isVoid())
	{
		if (found == data->attributes.end())
			return;
		data->attributes.erase(found);
	}
	else
	{
		if (found != data->attributes.end() && attributesEqual(found->second, value))
			return;
		data->attributes[attribute] = value;
	}
	data->serializedAttributesDirty = true;
	data->attributeChangedSignal(attribute);
	raisePropertyChanged(propAttributesSerialize);
}

BinaryString Instance::getSerializedAttributes() const
{
	const OnDemandInstance* data = onDemandRead();
	if (!data)
		return BinaryString();
	if (!data->serializedAttributesDirty)
		return data->serializedAttributes;
	return encodeSerializedAttributes(data->attributes);
}

void Instance::setSerializedAttributes(const BinaryString& value)
{
	if (value.value().empty() && !onDemandRead())
		return;
	OnDemandInstance* data = onDemandWrite();
	std::map<std::string, Reflection::Variant> decoded;
	if (!decodeSerializedAttributes(value, decoded))
	{
		StandardOut::singleton()->printf(MESSAGE_WARNING,
			"Ignoring malformed or unsupported AttributesSerialize on %s",
			getFullName().c_str());
		data->serializedAttributes = value;
		data->serializedAttributesDirty = false;
		return;
	}
	data->attributes.swap(decoded);
	data->serializedAttributes = value;
	data->serializedAttributesDirty = false;
}

void Instance::resetPropertyToDefault(std::string propertyName)
{
	Reflection::PropertyDescriptor* property = findPropertyDescriptor(propertyName.c_str());
	if (!property)
		throw std::runtime_error(RBX::format("%s is not a valid property of %s", propertyName.c_str(), getClassNameStr().c_str()));
	if (property->isReadOnly() || property->isWriteOnly())
		throw std::runtime_error(RBX::format("Property %s cannot be reset", propertyName.c_str()));

	// Property defaults are established by the concrete class constructor.  A
	// fresh instance therefore provides the same canonical value used by
	// Instance.new and deserialization when the property is absent.
	shared_ptr<Instance> defaults = Creatable<Instance>::createByName(getDescriptor().name, EngineCreator);
	if (!defaults)
		throw std::runtime_error(RBX::format("Unable to obtain defaults for %s", getClassNameStr().c_str()));

	property->copyValue(defaults.get(), this);
}

Reflection::Variant Instance::getStyled(std::string propertyName, std::string selector)
{
	// A selector explicitly asks for a matching rule rather than the effective
	// value. Style-rule selection is handled by the style engine; an empty
	// selector asks for the value currently resolved on this instance.
	if (!selector.empty())
		throw std::runtime_error("Explicit GetStyled selectors are not available for this instance");

	Reflection::PropertyDescriptor* property = findPropertyDescriptor(propertyName.c_str());
	if (!property || property->isWriteOnly())
		throw std::runtime_error(RBX::format("%s is not a valid property of %s",
			propertyName.c_str(), getClassNameStr().c_str()));

	Reflection::Variant value;
	property->getVariant(this, value);
	return value;
}

shared_ptr<Instance> Instance::createChild(const RBX::Name& className, RBX::CreatorRole creatorRole)
{
	return Creatable<Instance>::createByName(className, creatorRole);
}

void Instance::readChild(const XmlElement* childElement, IReferenceBinder& binder, CreatorRole creatorRole)
{
	const RBX::Name* className = NULL;
	if (!childElement->findAttributeValue(tag_class, className))
		return;

	shared_ptr<Instance> childInstance = createChild(*className, creatorRole);

	if (childInstance!=NULL) {
		// First read the data before setting parent. This is more efficient and makes programming easier,
		// because an Instance's data is set *before* being added
		childInstance->read(childElement, binder, creatorRole);
		if (childInstance!=NULL)
			childInstance->setParent(this);
	} else {
#ifdef _DEBUG
		std::string message;
		message = "Instance::readChild: Ignoring unrecognized class \"";
		std::string text;
		if (className)
			message += className->c_str();
		message += "\"";
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_WARNING, message.c_str());
#endif
		// Read the referent attribute, if it exists
		// We do this here because instance is NULL and therefore can't read it!
		const XmlAttribute* referentAttribute = childElement->findAttribute(name_referent);
		if (referentAttribute!=NULL)
			binder.announceID(referentAttribute, NULL);
	}

}

void Instance::readChildren(const XmlElement* element, IReferenceBinder& binder, RBX::CreatorRole creatorRole)
{
	if (!element)
		return;
	const XmlElement* childElement = element->findFirstChildByTag(tag_Item);
	while (childElement)
	{
		readChild(childElement, binder, creatorRole);
		childElement = element->findNextChildWithSameTag(childElement);
	}
}

void Instance::readProperty(const XmlElement* propertyElement, IReferenceBinder& binder)
{
	std::string name;
	if (propertyElement->findAttributeValue(name_name, name))
	{
		Reflection::PropertyDescriptor* desc = this->findPropertyDescriptor(name.c_str());
		// Current Studio serializes BasePart.Color under its historical wire
		// name, Color3uint8, even though the public reflection property remains
		// Color.  Older clients looked up only the public name and silently left
		// every modern place part at the constructor's medium-gray default.
		// Resolve the wire alias through reflection so this remains independent
		// of the concrete BasePart implementation and also covers derived parts.
		if (!desc && name == "Color3uint8")
			desc = this->findPropertyDescriptor("Color");
		if (desc)
			if (!desc->isReadOnly())
				Reflection::Property(*desc, this).read(propertyElement, binder);
	}
}


void Instance::readProperties(const XmlElement* container, IReferenceBinder& binder)
{
	const XmlElement* propertyElement = container->firstChild();
	while (propertyElement) {
		this->readProperty(propertyElement, binder);
		propertyElement = container->nextChild(propertyElement);
	}
}

void Instance::read(const XmlElement* element, IReferenceBinder& binder, RBX::CreatorRole creatorRole)
{
	// Read the referent attribute, if it exists
	const XmlAttribute* referentAttribute = element->findAttribute(name_referent);
	if (referentAttribute!=NULL)
	{
		binder.announceID(referentAttribute, this);
	}

	if (element->getTag()==tag_Item)
	{
		// Read the Properties and Attributes
		const XmlElement* propertiesElement = element->findFirstChildByTag(tag_Properties);
		if (propertiesElement!=NULL)
			readProperties(propertiesElement, binder);

		// Read the child Instances
		readChildren(element, binder, creatorRole);
	}
	else
	{
		// Assume that we are reading a Property of this Instance
		this->readProperty(element, binder);
	}
}

static bool itIsInScope(Instance*)
{
	return true;
}

void Instance::writeChildren(XmlElement* container, RBX::CreatorRole creatorRole, const SaveFilter saveFilter)
{
	writeChildren(container, itIsInScope, creatorRole, saveFilter);
}

void Instance::writeChildren(XmlElement* container, const boost::function<bool(Instance*)>& isInScope, RBX::CreatorRole creatorRole, const SaveFilter saveFilter)
{
	if (children)
	{
		const Instances& c(*children);
		for (Instances::const_iterator iter = c.begin(); iter!=c.end(); ++iter)
		{
			if( Serializer::canWriteChild((*iter),saveFilter) )
				if (XmlElement* child = (*iter)->writeXml(isInScope, creatorRole))
					container->addChild(child);
		}
	}
}

void Instance::writeProperties(XmlElement* container) const
{
	// Write each Property, provided it wants to be streamed
	RBX::Reflection::ConstPropertyIterator iter = properties_begin();
	while (iter!=properties_end())
	{
		const Reflection::PropertyDescriptor& descriptor = (*iter).getDescriptor();
		
		XmlElement* element = descriptor.write(this);
		if (element!=NULL)
			container->addChild(element);

		++iter;
	}
}

XmlElement* Instance::writeXml(const boost::function<bool(Instance*)>& isInScope, RBX::CreatorRole creatorRole)
{
	// TODO: archivable==false messes up functions like clone()
	//       find a better way to do this
	if (!getIsArchivable() || (getClassName()==RBX::Name::getNullName()))
		return NULL;

	switch(creatorRole)
	{
	case ReplicationCreator:	
		RBXASSERT(0); 
		break;
	case SerializationCreator:
		if(!getDescriptor().isSerializable()) 
			return NULL;
		break;
	case ScriptingCreator:
		if(!getDescriptor().isScriptCreatable()) 
			return NULL;
		break;
	case EngineCreator:
		break;
	}

	XmlElement* element = new XmlElement(tag_Item);

	element->addAttribute(tag_class, &getClassName());

	// Write the referent GUID if it exists
	element->addAttribute(name_referent, InstanceHandle(this));

	// Write properties and attributes
	writeProperties(element->addChild(tag_Properties));
	
	// Write child items
	writeChildren(element, isInScope, creatorRole);
	
	return element;
}

static void computeChildCost(shared_ptr<Instance> instance, int* result)
{
	(*result) += instance->getPersistentDataCost();
}
int Instance::getPersistentDataCost() const
{
	int total = 4;
	visitChildren(boost::bind(&computeChildCost, _1, &total));
	return total;
}

void Instance::onChildChanged(Instance* instance, const PropertyChanged& event)
{
	if (getParent()!=NULL)
		getParent()->onChildChanged(instance, event);
}


int Instance::findChildIndex(const Instance* instance) const
{
	RBXASSERT(children);
    Instances::const_iterator it;
	const Instances& c(*children);
	it = std::find( c.begin(), c.end(), instance->shared_from_this() );
	debugAssertM((it != c.end()), "Trying to find child not in the Instance child list");
	return (it - c.begin());
}

void Instance::waitForChild(std::string childName, boost::function<void(shared_ptr<Instance>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (childName == "")
	{
		errorFunction("WaitForChild called with an empty child name.");
		return;
	}
	if (findFirstChildByName(childName) != NULL)
	{
		resumeFunction(shared_from(findFirstChildByName(childName)));
		return;
	}

	OnDemandInstance* onDemand = onDemandWrite();
	OnDemandInstance::ThreadWaitingForChild thread;
	thread.childName = childName;
	thread.resumeFunction = resumeFunction;
	onDemand->threadsWaitingForChildren.push_back(thread);
}

void Instance::checkParentWaitingForChildren()
{
	if (!parent || !parent->onDemandRead())
		return;

	const std::string name = getName();
	std::vector<OnDemandInstance::ThreadWaitingForChild>& waitingThreads = parent->onDemandWrite()->threadsWaitingForChildren;
	std::vector<OnDemandInstance::ThreadWaitingForChild>::iterator iter = waitingThreads.begin();

	for (;iter != waitingThreads.end();)
	{
		if (iter->childName == name)
		{
			iter->resumeFunction(shared_from(this));
			iter = waitingThreads.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

Instance* Instance::findFirstChildByNameRecursive(const std::string& findName)
{
	// breadth-first search
	Instance* child = findFirstChildByName(findName);
	if (child!=NULL)
		return child;

	if (!children)
		return NULL;

	const Instances& c(*children);
	for (size_t i = 0; i < c.size(); ++i) {
		child = c[i]->findFirstChildByNameRecursive(findName);
		if (child!=NULL)
			return child;
	}

	return NULL;
}


const Instance* Instance::findConstFirstChildByName(const std::string& findName) const
{
	if (!children)
		return NULL;
	const Instances& c(*children);
	for (size_t i = 0; i < c.size(); ++i) {
		if (c[i]->getName() == findName) {
			return c[i].get();
		}
	}
	return NULL;
}

shared_ptr<Instance> Instance::findFirstAncestorOf(const Instance* descendant) const
{
	if (!children)
		return shared_ptr<Instance>();
	const Instances& c(*children);
	for (size_t i = 0; i < c.size(); ++i) {
		if (c[i]->isAncestorOf(descendant)) {
			return c[i];
		}
	}
	return shared_ptr<Instance>();
}

shared_ptr<Instance> Instance::findFirstAncestor(std::string name)
{
	for (Instance* ancestor = getParent(); ancestor; ancestor = ancestor->getParent())
		if (ancestor->getName() == name)
			return shared_from(ancestor);
	return shared_ptr<Instance>();
}

shared_ptr<Instance> Instance::findFirstAncestorOfClass(std::string className)
{
	for (Instance* ancestor = getParent(); ancestor; ancestor = ancestor->getParent())
		if (ancestor->getClassNameStr() == className)
			return shared_from(ancestor);
	return shared_ptr<Instance>();
}

shared_ptr<Instance> Instance::findFirstAncestorWhichIsA(std::string className)
{
	for (Instance* ancestor = getParent(); ancestor; ancestor = ancestor->getParent())
		if (ancestor->isA(className))
			return shared_from(ancestor);
	return shared_ptr<Instance>();
}

shared_ptr<Instance> Instance::findFirstChildOfClass(std::string className)
{
	if (!children)
		return shared_ptr<Instance>();
	for (Instances::const_iterator child = children->begin(); child != children->end(); ++child)
		if ((*child)->getClassNameStr() == className)
			return *child;
	return shared_ptr<Instance>();
}

shared_ptr<Instance> Instance::findFirstChildWhichIsA(std::string className,
	bool recursive)
{
	if (!children)
		return shared_ptr<Instance>();
	for (Instances::const_iterator child = children->begin(); child != children->end(); ++child)
	{
		if ((*child)->isA(className))
			return *child;
		if (recursive)
		{
			shared_ptr<Instance> descendant =
				(*child)->findFirstChildWhichIsA(className, true);
			if (descendant)
				return descendant;
		}
	}
	return shared_ptr<Instance>();
}



ChildAdded::ChildAdded(Instance* child):child(shared_from(child)) 
{
}
ChildAdded::ChildAdded(const ChildAdded& event):child(event.child)
{
}

ChildRemoved::ChildRemoved(Instance* child):child(shared_from(child)) {
}
ChildRemoved::ChildRemoved(const ChildRemoved& event):child(event.child) {
}

void Instance::securityCheck() const
{
	securityCheck(RBX::Security::Context::current());
}

void Instance::securityCheck(RBX::Security::Context& context) const
{
	context.requirePermission(getDescriptor().security, "Class security check");

	// Parents affect child security. The notion is that anything inside a secure object should also be secure
	if (const Instance* parent = getParent())
		parent->securityCheck(context);
}

void Instance::verifySetAncestor(const Instance* const newParent, const Instance* const instanceGettingNewParent) const
{
	visitChildren(boost::bind(&Instance::verifySetAncestor, _1, newParent, instanceGettingNewParent));
}

void Instance::verifyAddDescendant(const Instance* newParent, const Instance* instanceGettingNewParent) const {
	// recur with this = this.parent
	const Instance* parent = getParent();
	const Instance* oldParent = instanceGettingNewParent->getParent();
	if (parent!=NULL && (parent!=oldParent) && !parent->isAncestorOf(oldParent))
	{
		parent->verifyAddDescendant(parent, instanceGettingNewParent);
	}
}

class SetParentSentry 
{

public:
	SetParentSentry(Instance* instance) : mInstance(instance) { mInstance->isSettingParent = true; }
	~SetParentSentry() { mInstance->isSettingParent = false; }

private:
	SetParentSentry() {}

private:
	Instance* mInstance;
};

// An anti-exploit feature in this function assumes "newParent" is the first argument to the function.
bool Instance::setParentInternal(Instance* newParent, bool ignoreLock)
{	
	if (newParent==parent) {
		return true;
	}

	

	//Things that have locked parents (not complete)
	//  All Services
	//  LuaDraggers (to prevent insertion)
	//  DataModel (to prevent parenting)
	//  Workspace (Service)
	//  GuiRoot
	//  GuiHooks
	//  ClientReplicators
	//  Player objects (when removed)
	//  Server Replicators
	if (!ignoreLock && getIsParentLocked()) {
		std::string message = RBX::format("The Parent property of %s is locked, current parent: %s, new parent %s", getFullName().c_str(), 
			parent ? parent->getName().c_str() : "NULL", 
			newParent ? newParent->getName().c_str() : "NULL");
		throw std::runtime_error(message);
	}

	if (this==newParent) {
		std::string message = RBX::format("Attempt to set %s as its own parent", getFullName().c_str());
		throw std::runtime_error(message);
	}

	if (this->isAncestorOf(newParent)) {
		RBXASSERT(newParent);
		std::string message = RBX::format("Attempt to set parent of %s to %s would result in circular reference", getFullName().c_str(), newParent->getFullName().c_str());
		throw std::runtime_error(message);
	}

	FASTLOG3(FLog::InstanceTreeManipulation, "Instance %p: Setting parent to: %p. Old parent: %p", this, newParent, parent);

	if (isSettingParent)
	{
		if (getParent() == newParent)
		{
			return true;
		}

		std::string parentName = parent ? parent->getName() : "NULL";
		std::string newParentName = newParent ? newParent->getName() : "NULL";
		std::string message = RBX::format("Something unexpectedly tried to set the parent of %s to %s while trying to set the parent of %s. Current parent is %s.", 
			getName().c_str(), newParentName.c_str(), getName().c_str(), parentName.c_str());
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_WARNING, message.c_str());

		return false;
	}

	shared_ptr<Instance> oldParent = shared_from(parent);
	// Keep a lock on ourself until we exit the function!
	shared_ptr<Instance> self = shared_from(this);

	SetParentSentry sentry(this);

	//May throw if the child doesn't like this
	this->verifySetParent(newParent);
	// may throw if any descendant doesn't like this
	this->verifySetAncestor(newParent, this);
	if (newParent) {
		// may throw if an ancestor doesn't like this
		newParent->verifyAddChild(this);
		newParent->verifyAddDescendant(newParent, this);
	}

	if (oldParent)
	{
		if (!oldParent->contains(newParent))
			signalDescendantRemoving(self, oldParent.get(), newParent);
		oldParent->onChildRemoving(this);
		
		{
			boost::shared_ptr<Instances>& c = oldParent->children.write();

			if (c->size() == 1)
			{
				c->clear();
				oldParent->children.reset();	// We never keep an empty list around
			}
			else
			{
				// TODO: It might be faster to search from back-to-front  (Since Instance::removeAllChildren starts at back)
				Instances::iterator iter = std::find(c->begin(), c->end(), self);

				if(iter != c->end())
				{
					if (c->size() > 20)
					{
						// Fast-remove. This can make a huge speed improvement over regular remove
						*iter = c->back();
						c->pop_back();
					}
					else
					{
						// TODO: We keep this around because hopper bins don't do well with re-ordering
						c->erase(iter);
					}
				}
			}
		}
		this->parent = NULL;

		
	}

	if (newParent!=NULL)
	{
		newParent->children.write()->push_back(self);
	}
	
	this->parent = newParent;

	// signals for the child being removed
	if (oldParent != NULL)
	{
		ChildRemovedSignalData data(self);
		oldParent->combinedSignal(CHILD_REMOVED, &data);
		oldParent->childRemovedSignal(self);
		oldParent->onChildRemoved(this);	// added by DB 4/3/07
	}

    // This is here to aid in preventing ALX style exploits
    const void* thisFunction = getSetParentAddr();
    checkRbxCaller<kCallCheckCallArg, callCheckSetBasicFlag<HATE_RETURN_CHECK> >(thisFunction);    

	// signals for the child being added
#if !defined(RBX_RCC_SECURITY) && !defined(RBX_STUDIO_BUILD) && !defined(_NOOPT) && !defined(_DEBUG) && defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
    bool detectedExploit = false;
#endif
	if (newParent != NULL)
	{
		newParent->onChildAdded(this);
		if (!newParent->contains(oldParent.get()))
			signalDescendantAdded(this, newParent, oldParent.get());

		const ChildAddedSignalData data(self);
		newParent->combinedSignal(CHILD_ADDED, &data);
		newParent->childAddedSignal(self);

		checkParentWaitingForChildren();
	}
#if !defined(RBX_RCC_SECURITY) && !defined(RBX_STUDIO_BUILD) && !defined(_NOOPT) && !defined(_DEBUG) && defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
    else
    {
        detectedExploit = (detectDllByExceptionChainStack<4>(&newParent, RBX::Security::kCheckDefault) != 0);
    }
#endif


	// TODO: Refactor this code - onAncestorChanged should happen BEFORE signalDescendantAdded
	this->onAncestorChanged(AncestorChanged(this, oldParent.get(), newParent));

	raiseChanged(propParent);

#if !defined(RBX_RCC_SECURITY) && !defined(RBX_STUDIO_BUILD) && !defined(_NOOPT) && !defined(_DEBUG) && defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
    if (detectedExploit)
    {
        Security::ProtectionMarkers::beginVirtualization();
        RBX::Security::setHackFlagVs<0>(RBX::Security::hackFlag3, HATE_SEH_CHECK);
        Security::ProtectionMarkers::end();
    }
#endif
	
	return true;
}


void Instance::onAncestorChanged(const AncestorChanged& event)
{
	// Notify all children
	if (children)
	{
		shared_ptr<const Instances> c(children.read());
		for (Instances::const_iterator iter = c->begin(); iter!=c->end(); ++iter)
			(*iter)->onAncestorChanged(event);
	}

	if (!combinedSignal.empty()){
		AncestryChangedSignalData data(shared_from(event.child), shared_from(event.newParent));
		combinedSignal(ANCESTRY_CHANGED, &data);
	}

	if (!ancestryChangedSignal.empty())
		ancestryChangedSignal(shared_from(event.child), shared_from(event.newParent));
}

void Instance::signalDescendantAdded(Instance* instance, Instance* beginParent, Instance* oldParent)
{
	//Notify parents
	Instance* parent = beginParent;
	while (parent!=NULL && (parent!=oldParent) && !parent->isAncestorOf(oldParent))
	{
		parent->onDescendantAdded(instance);
		parent = parent->getParent();
	}

	// recurse through children
	shared_ptr<const Instances> c(instance->getChildren().read());
	if (c)
		for (Instances::const_iterator iter = c->begin(); iter!=c->end(); ++iter)
			signalDescendantAdded(iter->get(), beginParent, oldParent);
}

void Instance::onDescendantAdded(Instance* instance)
{
    if(onDemandRead())
        onDemandWrite()->descendantAddedSignal(shared_from(instance));
}

void Instance::signalDescendantRemoving(const shared_ptr<Instance>& instance, Instance* beginParent, Instance* newParent)
{
	//Notify parents
	Instance* parent = beginParent;
	while (parent!=NULL && (parent!=newParent) && !parent->isAncestorOf(newParent))
	{
		parent->onDescendantRemoving(instance);
		parent = parent->getParent();
	}

	// recurse through children
	shared_ptr<const Instances> c(instance->getChildren().read());
	if (c)
		for (Instances::const_iterator iter = c->begin(); iter!=c->end(); ++iter)
			signalDescendantRemoving(*iter, beginParent, newParent);
}

void Instance::onDescendantRemoving(const shared_ptr<Instance>& instance)
{
	descendantRemovingSignal(instance);
}

static bool isInScope(Instance* parent, Instance* child)
{
	if (parent == child)
		return true;
	return parent->isAncestorOf(child);
}

// DB 12/5/05 - added here to centralize the spawner behavior - to, from XML
XmlElement* Instance::toNewXmlRoot(Instance* instance, RBX::CreatorRole creatorRole)
{
	XmlElement* answer = Serializer::newRootElement();
	if(XmlElement* child = instance->writeXml(boost::bind(isInScope, instance, _1), creatorRole))
		answer->addChild(child);
	return answer;
}

// Clones an instance and all its children using Reflection. Ref properties
// are also managed to reference items within the cloned tree.
class Cloner : boost::noncopyable
{
	// Map of Source -> Destination
	typedef boost::unordered_map<const Instance*, shared_ptr<Instance> > Map;
	Map map;
	const CreatorRole creatorRole;
	shared_ptr<Instance> clone;
public:
	// The constructor does the cloning
	Cloner(const Instance* instance, CreatorRole creatorRole):creatorRole(creatorRole)
	{
		shared_ptr<const Instance> source = shared_from(instance);

		// First just duplicate all the objects
		clone = createClones(source);

		// Copy all the property values (except for Parent)
		std::for_each(map.begin(), map.end(), boost::bind(&Cloner::copyProperties, this, _1));

		// Now that all properties are copied, re-create the Instance tree
		setParent(source);

		// Notify cloning
		std::for_each(map.begin(), map.end(), boost::bind(&Cloner::notifyCloning, this, _1));
	}

	shared_ptr<Instance> getClone() const
	{
		return clone;
	}

private:
	// This function simply duplicates all the instances, but doesn't create the tree nor copy properties
	shared_ptr<Instance> createClones(shared_ptr<const Instance> instance)
	{
		RBXASSERT(instance);

		if (!instance->getDescriptor().isSerializable())
			return shared_ptr<Instance>();

		if (!instance->getIsArchivable())
			return shared_ptr<Instance>();

		shared_ptr<Instance> copy = Creatable<Instance>::createByName(instance->getDescriptor().name, creatorRole);
		if (!copy)
			throw std::runtime_error(RBX::format("%s cannot be cloned", instance->getFullName().c_str()));

		RBXASSERT(map.find(instance.get()) == map.end());
		map[instance.get()] = copy;

		instance->visitChildren(boost::bind(&Cloner::createClones, this, _1));
		return copy;
	}

	// Copy all non-Parent serializable properties of an Instance
	void copyProperties(const Map::value_type& value)
	{
		const Instance* source = value.first;
		std::for_each(
			source->properties_begin(), source->properties_end(), 
			boost::bind(&Cloner::copyProperty, this, _1, source, value.second.get())
			);
	}

	// Copy non-Parent serializable property
	void copyProperty(const RBX::Reflection::ConstProperty& prop, const Instance* source, Instance* destination)
	{
		if (!prop.getDescriptor().alwaysClone())
		{
			if (!prop.getDescriptor().canXmlRead())
				return;
			if (!prop.getDescriptor().canXmlWrite())
				return;
		}
		RBXASSERT (prop.getDescriptor() != Instance::propParent);

		RBXASSERT(source == prop.getInstance());
		if (Reflection::RefPropertyDescriptor::isRefPropertyDescriptor(prop.getDescriptor()))
		{
			const Reflection::RefPropertyDescriptor* refDesc = static_cast<const Reflection::RefPropertyDescriptor*>(&prop.getDescriptor());
			RBXASSERT(refDesc);

			Instance* value = static_cast<Instance*>(refDesc->getRefValue(source));
			Map::const_iterator iter = map.find(value);
			if (iter != map.end())
				value = iter->second.get();	// redirect to the cloned copy

			refDesc->setRefValue(destination, value);
		}
		else
			prop.getDescriptor().copyValue(source, destination);
	}

	// Recursively re-create the Instance tree on the copied items (bottom-up)
	void setParent(shared_ptr<const Instance> source)
	{
		Map::const_iterator iter = map.find(source.get());
		if (iter == map.end())
			return;	// This item didn't get cloned, probably because it wasn't serializable

		const shared_ptr<Instance>& destination = iter->second;

		// First iterate down to the children, since we want to set the parents bottom-up
		source->visitChildren(boost::bind(&Cloner::setParent, this, _1));

		// Now set this node's parent
		if (destination == clone)
			return;	// don't set the root's parent

		RBXASSERT(source->getParent());
		RBXASSERT(map.find(source->getParent()) != map.end());

		Instance* parent = map[source->getParent()].get();
		RBXASSERT(parent);

		destination->setParent(parent);
	}

	void notifyCloning(const Map::value_type& value)
	{
		const Instance* source = value.first;
		if(source->onDemandRead())
			const_cast<RBX::Instance*>(source)->onDemandWrite()->instanceClonedSignal(value.second);
	}
};

shared_ptr<Instance> Instance::clone(CreatorRole creatorRole)
{
    return Cloner(this, creatorRole).getClone();
}

shared_ptr<Instance> Instance::luaClone()
{
	return clone(RBX::ScriptingCreator);
}


// Declare some dictionary entries
static const RBX::Name& nameName = RBX::Name::declare("Name");
static const RBX::Name& nameParent = RBX::Name::declare("Parent");

Instance::Instance()
	:parent(NULL)
	,name("Instance")
	,archivable(true)
	,isParentLocked(false)
	,robloxLocked(false)
	,isSettingParent(false)
{
}

Instance::Instance(const char* name)
	:parent(NULL)
	,name(name)
	,archivable(true)
	,isParentLocked(false)
	,robloxLocked(false)
	,isSettingParent(false)
{
}

Instance::~Instance() 
{
	RBXASSERT(parent==NULL);
}

void Instance::remove()
{
		boost::shared_ptr<const Instances> c(children.read());
		setParent(NULL);
		if (c)
			// To expedite garbage collection, we recursively call remove on the children
			std::for_each(c->begin(), c->end(), boost::bind(&Instance::remove, _1));
}

void Instance::destroy()
{
	if (getIsParentLocked() && getParent())
		throw RBX::runtime_error("The Parent property of %s is locked", getFullName().c_str());

	// This method will remove this object from the DataModel, possibly
	// removing the last shared_ptr reference to this. Make a local
	// self shared_ptr to make sure that 'this' remains valid inside this method.
	boost::shared_ptr<const Instances> c(children.read());
	boost::shared_ptr<Instance> self = shared_from(this);
	DataModel* dataModel = DataModel::get(this);
	ScopedDestroyingSuppression suppressDuringClosedDataModelTeardown(
		dataModel && dataModel->isClosed());
	if (destroyingSuppressionDepth == 0 && onDemandRead())
		onDemandWrite()->destroyingSignal();

	// Set the parent
	setAndLockParent(NULL);

	if (c)
		// To expedite garbage collection, we recursively call remove on the children
		std::for_each(c->begin(), c->end(), boost::bind(&Instance::destroy, _1));

	// Clear all events to help prevent memory leaks.
	// Without this code, the following Lua will leak:
	// while true do
	//    local i = Instance.new("Part")
	//    local c = i.Changed:connect(function ()
	//       print(i) -- this closure prevents i from being collected
	//    end)
	//    wait(0.1)
	//    i:Remove()
	// end
	std::for_each(
		getDescriptor().begin<Reflection::EventDescriptor>(), getDescriptor().end<Reflection::EventDescriptor>(), 
		boost::bind(&Reflection::EventDescriptor::disconnectAll, _1, this)
		);
}

void Instance::setName(const std::string& value)
{
	if (name.get() != value)
	{
		if (value.size() > 100)
			name = value.substr(0, 100);
		else
			name = value;
		this->raisePropertyChanged(desc_Name);

		checkParentWaitingForChildren();
	}
}

void Instance::setRobloxLocked(bool value)
{
	if(robloxLocked != value){
		robloxLocked = value;
		raisePropertyChanged(propRobloxLocked);
	}
    void (Instance::* thisFunction)(bool) = &Instance::setRobloxLocked;
    checkRbxCaller<kCallCheckCodeOnly, callCheckSetApiFlag<kRbxLockedApiOffset> >(
        reinterpret_cast<void*>((void*&)(thisFunction)));
}

void Instance::setIsArchivable(bool value)
{
	if(archivable != value)
	{
		archivable = value;
		raisePropertyChanged(propArchivable);
	}
}


void Instance::removeAllChildren()
{
	while (children) {
		// Note: this is using "->" which implies a read, but the reference
		// returned from back() is used to perform updates (removal).
		shared_ptr<Instance> child(children->back());
		child->remove();
	}
}

void Instance::destroyAllChildrenLua()
{
    // security that results in a kick
    void (Instance::* thisFunction)() = &Instance::destroyAllChildrenLua;
    checkRbxCaller<kCallCheckRegCall, callCheckSetBasicFlag<HATE_DESTROY_ALL> >(
        reinterpret_cast<void*>((void*&)(thisFunction)));

	while (children) 
	{
		shared_ptr<Instance> child(children->back());

        // this prevents these commands from actually being processed if incorrectly called.
        if ( !checkRbxCaller<kCallCheckCodeOnly, callCheckNop >(reinterpret_cast<void*>((void*&)(thisFunction))) )
        {
		    child->destroy();
        }
	}
}

void Instance::predelete(Instance* instance)
{
	instance->predelete();
}

void Instance::predelete()
{	
	RBXASSERT(parent==NULL);

	while (children)
	{
		shared_ptr<Instance> child;
		{
			// get the last element and pop it off (more efficient that way)
			shared_ptr<Instances> c(children.write());
			child = c->back();
			
			// TODO: Can we nuke this:
			child->signalDescendantRemoving(child, this, NULL);
			// TODO: Can we nuke this:
			onChildRemoving(child.get());

			child->parent = NULL;
			
			c->pop_back();

			if (c->size()==0)
				children.reset();
		}

		{
			// TODO: Can we nuke this:
			ChildRemovedSignalData data(child);
			combinedSignal(CHILD_REMOVED, &data);
		}
		// TODO: **** Can we nuke this:
		childRemovedSignal(child);

		// TODO: Can we nuke this:
		child->onAncestorChanged(AncestorChanged(child.get(), this, NULL));
		// TODO: Can we nuke this:
		child->raiseChanged(propParent);
	}

}



bool Instance::contains(const Instance* child) const
{
	while (child!=NULL)
	{
		if (child==this)
			return true;
		child = child->getParent();
	}
	return false;
}

bool Instance::askSetParent(const Instance* instance) const
{
	return false;
}
bool Instance::askForbidParent(const Instance* instance) const
{
	return false;
}

bool Instance::askAddChild(const Instance* instance) const
{
	return false;
}
bool Instance::askForbidChild(const Instance* instance) const
{
	return false;
}


// Moves all children to the same level as this
void Instance::promoteChildren()
{
	while (numChildren()!=0)
		this->getChild(0)->setParent(getParent());
}

void Instance::setAndLockParent(Instance* instance)
{
	bool wasLocked = getIsParentLocked();
	lockParent();
	bool failedToSetParent = false;
	try
	{
		failedToSetParent = !setLockedParent(instance);
	}
	catch (RBX::base_exception&)
	{
		if (!wasLocked)
		{
			unlockParent();
		}
		throw;
	}

	if (failedToSetParent && !wasLocked)
	{
		unlockParent();
	}
}

// Render up to (but not including) the root node
std::string Instance::getFullName() const
{
	if (getParent() && !Instance::fastDynamicCast<ServiceProvider>(getParent()))
		return getParent()->getFullName() + "." + getName();
	else
		return getName();
}

const OnDemandInstance* Instance::onDemandRead() const
{
	return onDemandPtr.get();
}

OnDemandInstance* Instance::onDemandWrite()
{
	OnDemandInstance* onDemand = onDemandPtr.get();
	if(onDemand == NULL)
	{
		onDemandPtr.reset(initOnDemand());
		onDemand = onDemandPtr.get();
		RBXASSERT(onDemand);
	}

	return onDemand;
}

OnDemandInstance* Instance::initOnDemand()
{
	return new OnDemandInstance();
}

static FORCEINLINE void validateThreadAccess(Instance* inst)
{
    // this is actually useful for combating exploits.
    if (!DataModel::currentThreadHasWriteLock(inst))
        RBXCRASH();
}

void Instance::raisePropertyChanged(const RBX::Reflection::PropertyDescriptor& descriptor)
{
    // Any normal property assignment takes authorship back from the style
    // cascade. The resolver marks the property again after its reflected
    // write completes, so later rule/token changes can distinguish their own
    // previous value from a real authored override.
    clearPropertyStyleManaged(descriptor.name.toString());
    PropertyChanged event(RBX::Reflection::Property(descriptor, this));
    this->onPropertyChanged(descriptor);
    // security (This ensures the caller has a data model lock)
    if (this->parent && DFFlag::LockViolationInstanceCrash) // fast-path for replication and serialization
    {
        validateThreadAccess(this);
    }

    const PropertyChangedSignalData data(&descriptor);
    combinedSignal(PROPERTY_CHANGED, &data);
    propertyChangedSignal(&descriptor);
    if (getParent()!=NULL)
        getParent()->onChildChanged(this, event);
}

void Instance::raiseEventInvocation(const RBX::Reflection::EventDescriptor& descriptor, const RBX::Reflection::EventArguments& args, const SystemAddress* target)
{
    if (this->parent && DFFlag::LockViolationInstanceCrash) // fast-path for replication and serialization
    {
        validateThreadAccess(this);
    }

	const EventInvocationSignalData data(&descriptor, &args, target);
	combinedSignal(EVENT_INVOCATION, &data);
}

void Instance::humanoidChanged()
{
	Instance::HumanoidChangedSignalData data;
	combinedSignal(Instance::HUMANOID_CHANGED, &data);
}

void Instance::onGuidChanged()
{
}

const void* Instance::getSetParentAddr()
{
#ifdef _WIN32
    // VS2012 allows this code here, but not in member functions.
    // This code becomes invalid if setParentInternal becomes virtual.
    bool (RBX::Instance::*thisPmfn)(RBX::Instance*, bool) = &setParentInternal;
    const void* thisCodeStruct = (const void*&)(thisPmfn);
    return thisCodeStruct;
#else
    return NULL;
#endif
}

}	// namespace RBX

namespace RBX{ namespace Security {
    // the .rdata section
    volatile const uintptr_t rbxTextEndNeg = 0x00400000;
    volatile const size_t rbxTextSizeNeg = 1024;
}
}
