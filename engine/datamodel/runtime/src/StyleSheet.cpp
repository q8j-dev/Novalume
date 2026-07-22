#include "v8datamodel/StyleSheet.h"

#include <algorithm>
#include <map>
#include <set>

namespace RBX {

namespace {

thread_local std::set<Instance*> resolvingInstances;

class ResolutionGuard
{
public:
    explicit ResolutionGuard(Instance* instance)
        : instance(instance), inserted(resolvingInstances.insert(instance).second)
    {
    }

    ~ResolutionGuard()
    {
        if (inserted)
            resolvingInstances.erase(instance);
    }

    bool ownsResolution() const { return inserted; }

private:
    Instance* instance;
    bool inserted;
};

StyleSheet* findLinkedStyleSheet(Instance* instance)
{
    for (Instance* scope = instance; scope; scope = scope->getParent())
    {
        for (std::size_t index = 0; index < scope->numChildren(); ++index)
            if (StyleLink* link = Instance::fastDynamicCast<StyleLink>(scope->getChild(index)))
                if (StyleSheet* sheet = link->getStyleSheet())
                    return sheet;
    }
    return NULL;
}

bool selectorMatchesTag(const std::string& selector, const std::string& tag,
                        std::string* pseudo)
{
    const std::string classSelector = "." + tag;
    std::size_t begin = 0;
    while (begin < selector.size())
    {
        std::size_t end = selector.find(',', begin);
        if (end == std::string::npos)
            end = selector.size();
        std::string branch = selector.substr(begin, end - begin);
        const std::size_t first = branch.find_first_not_of(" \t");
        const std::size_t last = branch.find_last_not_of(" \t");
        branch = first == std::string::npos ? std::string()
                                            : branch.substr(first, last - first + 1);
        if (branch.compare(0, classSelector.size(), classSelector) == 0)
        {
            std::string suffix = branch.substr(classSelector.size());
            const std::size_t pseudoMarker = suffix.find("::");
            if (pseudoMarker != std::string::npos)
            {
                std::string name = suffix.substr(pseudoMarker + 2);
                const std::size_t nameEnd = name.find_first_of(" \t:>.");
                if (nameEnd != std::string::npos)
                    name.resize(nameEnd);
                if (!name.empty())
                {
                    *pseudo = name;
                    return true;
                }
            }
            else if (suffix.empty())
            {
                pseudo->clear();
                return true;
            }
        }
        begin = end + 1;
    }
    return false;
}

Instance* findOrCreatePseudo(Instance* owner, const std::string& className,
                             bool* createdPseudo)
{
    for (std::size_t index = 0; index < owner->numChildren(); ++index)
        if (owner->getChild(index)->getClassNameStr() == className)
            return owner->getChild(index);

    shared_ptr<Instance> created = Creatable<Instance>::createByName(
        Name::lookup(className.c_str()), EngineCreator);
    if (!created)
        return NULL;
    created->setName(className);
    created->setParent(owner);
    *createdPseudo = true;
    return created.get();
}

struct ResolvedProperty
{
    int priority;
    Reflection::Variant value;
    StyleSheet* sheet;
};

void collectStyleSheets(StyleSheet* sheet, std::set<StyleSheet*>& visiting,
                        std::set<StyleSheet*>& collected,
                        std::vector<StyleSheet*>& result)
{
    if (!sheet || collected.count(sheet) || !visiting.insert(sheet).second)
        return;

    shared_ptr<const Instances> derives = sheet->getDerives();
    for (const shared_ptr<Instance>& value : *derives)
        collectStyleSheets(Instance::fastDynamicCast<StyleSheet>(value.get()),
            visiting, collected, result);

    visiting.erase(sheet);
    if (collected.insert(sheet).second)
        result.push_back(sheet);
}

std::vector<StyleSheet*> getStyleSheetCascade(StyleSheet* sheet)
{
    std::set<StyleSheet*> visiting;
    std::set<StyleSheet*> collected;
    std::vector<StyleSheet*> result;
    collectStyleSheets(sheet, visiting, collected, result);
    return result;
}

bool applyRuleProperties(Instance* target,
                         const std::map<std::string, ResolvedProperty>& properties)
{
    bool changed = false;
    shared_ptr<Instance> defaults = Creatable<Instance>::createByName(
        target->getDescriptor().name, EngineCreator);
    for (const auto& entry : properties)
    {
        Reflection::PropertyDescriptor* descriptor =
            target->findPropertyDescriptor(entry.first.c_str());
        if (!descriptor || descriptor->isReadOnly() || descriptor->isWriteOnly())
            continue;

        Reflection::Variant value = entry.second.value;
        if (value.isType<std::string>())
        {
            const std::string reference = value.cast<std::string>();
            if (reference.size() > 1 && reference[0] == '$')
                value = entry.second.sheet->getAttribute(reference.substr(1));
        }
        if (value.isVoid())
            continue;

        // Style attributes are serialized with Luau's numeric representation
        // (double), while several native UI properties retain float or int
        // storage. The production style resolver coerces those numeric values
        // before reflection assignment; strict type equality silently drops
        // rules such as BackgroundTransparency and TextSize.
        if (value.type() != descriptor->type)
        {
            if (descriptor->type == Reflection::Type::singleton<float>())
            {
                if (value.isType<double>())
                    value = static_cast<float>(value.cast<double>());
                else if (value.isType<int>())
                    value = static_cast<float>(value.cast<int>());
            }
            else if (descriptor->type == Reflection::Type::singleton<double>())
            {
                if (value.isType<float>())
                    value = static_cast<double>(value.cast<float>());
                else if (value.isType<int>())
                    value = static_cast<double>(value.cast<int>());
            }
            else if (descriptor->type == Reflection::Type::singleton<int>())
            {
                if (value.isType<double>())
                    value = static_cast<int>(value.cast<double>());
                else if (value.isType<float>())
                    value = static_cast<int>(value.cast<float>());
            }
        }
        if (value.type() != descriptor->type)
            continue;

        // Directly authored properties outrank a stylesheet. A property still
        // holding its concrete class default has no local declaration and can
        // safely receive the resolved style value.
        const bool isDefault = !defaults || descriptor->equalValues(target, defaults.get());
        const bool wasStyled = target->isPropertyStyleManaged(entry.first);
        if (!isDefault && !wasStyled)
            continue;
        descriptor->setVariant(target, value);
        target->markPropertyStyleManaged(entry.first);
        if (!defaults || !descriptor->equalValues(target, defaults.get()))
            changed = true;
    }
    return changed;
}

} // namespace

bool applyResolvedStyles(Instance* instance)
{
    if (!instance || instance->getTagsInternal().empty())
        return false;
    ResolutionGuard guard(instance);
    if (!guard.ownsResolution())
        return false;
    StyleSheet* sheet = findLinkedStyleSheet(instance);
    if (!sheet)
        return false;

    std::map<std::string, std::map<std::string, ResolvedProperty> > resolved;
    const std::vector<StyleSheet*> cascade = getStyleSheetCascade(sheet);
    for (StyleSheet* cascadeSheet : cascade)
    {
        shared_ptr<const Instances> rules = cascadeSheet->getStyleRules();
        for (const shared_ptr<Instance>& candidate : *rules)
        {
            StyleRule* rule = Instance::fastDynamicCast<StyleRule>(candidate.get());
            if (!rule)
                continue;
            for (const std::string& tag : instance->getTagsInternal())
            {
                std::string pseudo;
                if (!selectorMatchesTag(rule->getSelector(), tag, &pseudo))
                    continue;
                shared_ptr<const Reflection::ValueTable> values = rule->getProperties();
                std::map<std::string, ResolvedProperty>& destination = resolved[pseudo];
                for (const auto& property : *values)
                {
                    auto existing = destination.find(property.first);
                    if (existing == destination.end() ||
                        rule->getPriority() >= existing->second.priority)
                        destination[property.first] = ResolvedProperty{
                            rule->getPriority(), property.second, cascadeSheet};
                }
            }
        }
    }

    bool changed = false;
    for (const auto& targetRules : resolved)
    {
        bool createdPseudo = false;
        Instance* target = targetRules.first.empty()
            ? instance : findOrCreatePseudo(instance, targetRules.first, &createdPseudo);
        if (target)
            changed = applyRuleProperties(target, targetRules.second) ||
                      createdPseudo || changed;
    }
    return changed;
}

bool applyResolvedStylesToSubtree(Instance* root)
{
    if (!root)
        return false;
    bool changed = applyResolvedStyles(root);
    shared_ptr<const Instances> descendants = root->getDescendants();
    for (const shared_ptr<Instance>& descendant : *descendants)
        changed = applyResolvedStyles(descendant.get()) || changed;
    return changed;
}

const char* const sStyleBase = "StyleBase";
const char* const sStyleRule = "StyleRule";
const char* const sStyleSheet = "StyleSheet";
const char* const sStyleLink = "StyleLink";
const char* const sStyleDerive = "StyleDerive";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<StyleBase, shared_ptr<const Instances>()>
    funcGetStyleRules(&StyleBase::getStyleRules, "GetStyleRules", Security::None);
static Reflection::BoundFuncDesc<StyleBase, void(shared_ptr<Instance>, int)>
    funcInsertStyleRule(&StyleBase::insertStyleRule, "InsertStyleRule", "rule",
        "priority", 0, Security::None);
static Reflection::BoundFuncDesc<StyleBase, void(shared_ptr<const Instances>)>
    funcSetStyleRules(&StyleBase::setStyleRules, "SetStyleRules", "rules",
        Security::None);
static Reflection::EventDesc<StyleBase, void()> eventStyleRulesChanged(
    &StyleBase::styleRulesChangedSignal, "StyleRulesChanged", Security::None);

static Reflection::PropDescriptor<StyleRule, int> propStyleRulePriority(
    "Priority", category_Data, &StyleRule::getPriority, &StyleRule::setPriority);
static Reflection::PropDescriptor<StyleRule, std::string> propStyleRuleSelector(
    "Selector", category_Data, &StyleRule::getSelector, &StyleRule::setSelector);
static Reflection::PropDescriptor<StyleRule, std::string> propStyleRuleSelectorError(
    "SelectorError", category_Data, &StyleRule::getSelectorError, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::BoundFuncDesc<StyleRule,
    shared_ptr<const Reflection::ValueTable>()> funcGetStyleProperties(
        &StyleRule::getProperties, "GetProperties", Security::None);
static Reflection::BoundFuncDesc<StyleRule,
    shared_ptr<const Reflection::ValueTable>()> funcGetStylePropertiesResolved(
        &StyleRule::getPropertiesResolved, "GetPropertiesResolved",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<StyleRule, Reflection::Variant(std::string)>
    funcGetStyleProperty(&StyleRule::getProperty, "GetProperty", "name",
        Security::None);
static Reflection::BoundFuncDesc<StyleRule, Reflection::Variant(std::string)>
    funcGetStylePropertyResolved(&StyleRule::getPropertyResolved,
        "GetPropertyResolved", "name", Security::RobloxScript);
static Reflection::BoundFuncDesc<StyleRule,
    void(shared_ptr<const Reflection::ValueTable>)> funcSetStyleProperties(
        &StyleRule::setProperties, "SetProperties", "styleProperties",
        Security::None);
static Reflection::BoundFuncDesc<StyleRule,
    void(std::string, Reflection::Variant)> funcSetStyleProperty(
        &StyleRule::setProperty, "SetProperty", "name", "value",
        Security::None);
static Reflection::BoundFuncDesc<StyleRule, Reflection::Variant()>
    funcGetDefaultPropertyTransition(&StyleRule::getDefaultPropertyTransition,
        "GetDefaultPropertyTransition", Security::None);
static Reflection::BoundFuncDesc<StyleRule, void(Reflection::Variant)>
    funcSetDefaultPropertyTransition(&StyleRule::setDefaultPropertyTransition,
        "SetDefaultPropertyTransition", "transitionParams", Security::None);
static Reflection::BoundFuncDesc<StyleRule,
    shared_ptr<const Reflection::ValueTable>()> funcGetPropertyTransitions(
        &StyleRule::getPropertyTransitions, "GetPropertyTransitions",
        Security::None);
static Reflection::BoundFuncDesc<StyleRule,
    void(std::string, Reflection::Variant)> funcSetPropertyTransition(
        &StyleRule::setPropertyTransition, "SetPropertyTransition", "property",
        "transitionParams", Security::None);
static Reflection::BoundFuncDesc<StyleRule,
    void(shared_ptr<const Reflection::ValueTable>)> funcSetPropertyTransitions(
        &StyleRule::setPropertyTransitions, "SetPropertyTransitions",
        "properties", Security::None);
static Reflection::EventDesc<StyleRule, void(std::string)>
    eventStyleRulePropertyChanged(&StyleRule::styleRulePropertyChangedSignal,
        "StyleRulePropertyChanged", "styleProperty", Security::RobloxScript);

static Reflection::BoundFuncDesc<StyleSheet, shared_ptr<const Instances>()>
    funcGetStyleSheetDerives(&StyleSheet::getDerives, "GetDerives",
        Security::None);
static Reflection::BoundFuncDesc<StyleSheet, void(shared_ptr<const Instances>)>
    funcSetStyleSheetDerives(&StyleSheet::setDerives, "SetDerives", "derives",
        Security::None);

static Reflection::RefPropDescriptor<StyleLink, StyleSheet> propLinkedStyleSheet(
    "StyleSheet", category_Data, &StyleLink::getStyleSheet,
    &StyleLink::setStyleSheet);
static Reflection::PropDescriptor<StyleDerive, int> propStyleDerivePriority(
    "Priority", category_Data, &StyleDerive::getPriority,
    &StyleDerive::setPriority);
static Reflection::RefPropDescriptor<StyleDerive, StyleSheet> propDerivedStyleSheet(
    "StyleSheet", category_Data, &StyleDerive::getStyleSheet,
    &StyleDerive::setStyleSheet);
REFLECTION_END();

StyleBase::StyleBase()
{
}

shared_ptr<const Instances> StyleBase::getStyleRules()
{
    shared_ptr<Instances> result(new Instances());
    shared_ptr<const Instances> children = getChildren().read();
    if (children)
    {
        for (const shared_ptr<Instance>& child : *children)
            if (dynamic_cast<StyleRule*>(child.get()))
                result->push_back(child);
    }
    return result;
}

void StyleBase::insertStyleRule(shared_ptr<Instance> value, int priority)
{
    shared_ptr<StyleRule> rule = shared_dynamic_cast<StyleRule>(value);
    if (!rule)
        throw runtime_error("StyleBase:InsertStyleRule expected a StyleRule");
    if (priority != 0)
        rule->setPriority(priority);
    rule->setParent(this);
}

void StyleBase::setStyleRules(shared_ptr<const Instances> rules)
{
    if (!rules)
        throw runtime_error("StyleBase:SetStyleRules expected an Instances array");

    std::vector<shared_ptr<StyleRule> > wanted;
    for (const shared_ptr<Instance>& value : *rules)
    {
        shared_ptr<StyleRule> rule = shared_dynamic_cast<StyleRule>(value);
        if (!rule)
            throw runtime_error("StyleBase:SetStyleRules only accepts StyleRule instances");
        wanted.push_back(rule);
    }

    shared_ptr<const Instances> existing = getStyleRules();
    for (const shared_ptr<Instance>& value : *existing)
    {
        shared_ptr<StyleRule> rule = shared_dynamic_cast<StyleRule>(value);
        if (std::find(wanted.begin(), wanted.end(), rule) == wanted.end())
            rule->setParent(NULL);
    }
    for (const shared_ptr<StyleRule>& rule : wanted)
        rule->setParent(this);
}

void StyleBase::onChildAdded(Instance* child)
{
    Super::onChildAdded(child);
    if (dynamic_cast<StyleRule*>(child))
        styleRulesChangedSignal();
}

void StyleBase::onChildRemoving(Instance* child)
{
    if (dynamic_cast<StyleRule*>(child))
        styleRulesChangedSignal();
    Super::onChildRemoving(child);
}

StyleRule::StyleRule()
    : priority(1)
{
    setName(sStyleRule);
}

void StyleRule::propertyChanged(const std::string& name)
{
    styleRulePropertyChangedSignal(name);
    if (StyleBase* base = dynamic_cast<StyleBase*>(getParent()))
        base->styleRulesChangedSignal();
}

void StyleRule::setPriority(int value)
{
    if (priority == value)
        return;
    priority = value;
    raisePropertyChanged(propStyleRulePriority);
    propertyChanged("Priority");
}

void StyleRule::setSelector(std::string value)
{
    if (selector == value)
        return;
    selector = std::move(value);
    selectorError.clear();
    raisePropertyChanged(propStyleRuleSelector);
    propertyChanged("Selector");
}

shared_ptr<const Reflection::ValueTable> StyleRule::getProperties()
{
    return shared_ptr<const Reflection::ValueTable>(
        new Reflection::ValueTable(properties));
}

shared_ptr<const Reflection::ValueTable> StyleRule::getPropertiesResolved()
{
    return getProperties();
}

Reflection::Variant StyleRule::getProperty(std::string name)
{
    Reflection::ValueTable::const_iterator it = properties.find(name);
    return it == properties.end() ? Reflection::Variant() : it->second;
}

Reflection::Variant StyleRule::getPropertyResolved(std::string name)
{
    return getProperty(std::move(name));
}

void StyleRule::setProperties(shared_ptr<const Reflection::ValueTable> values)
{
    if (!values)
        throw runtime_error("StyleRule:SetProperties expected a dictionary");
    properties = *values;
    propertyChanged("*");
}

void StyleRule::setProperty(std::string name, Reflection::Variant value)
{
    if (name.empty())
        throw runtime_error("StyleRule property name cannot be empty");
    if (value.isVoid())
        properties.erase(name);
    else
        properties[name] = value;
    propertyChanged(name);
}

Reflection::Variant StyleRule::getDefaultPropertyTransition()
{
    return defaultTransition;
}

void StyleRule::setDefaultPropertyTransition(Reflection::Variant value)
{
    defaultTransition = value;
    propertyChanged("DefaultPropertyTransition");
}

shared_ptr<const Reflection::ValueTable> StyleRule::getPropertyTransitions()
{
    return shared_ptr<const Reflection::ValueTable>(
        new Reflection::ValueTable(transitions));
}

void StyleRule::setPropertyTransition(std::string name, Reflection::Variant value)
{
    if (value.isVoid())
        transitions.erase(name);
    else
        transitions[name] = value;
    propertyChanged(name);
}

void StyleRule::setPropertyTransitions(
    shared_ptr<const Reflection::ValueTable> values)
{
    if (!values)
        throw runtime_error("StyleRule:SetPropertyTransitions expected a dictionary");
    transitions = *values;
    propertyChanged("*");
}

StyleSheet::StyleSheet()
{
    setName(sStyleSheet);
}

shared_ptr<const Instances> StyleSheet::getDerives()
{
    return shared_ptr<const Instances>(new Instances(derives));
}

void StyleSheet::setDerives(shared_ptr<const Instances> values)
{
    if (!values)
        throw runtime_error("StyleSheet:SetDerives expected an Instances array");
    Instances updated;
    for (const shared_ptr<Instance>& value : *values)
    {
        if (!dynamic_cast<StyleSheet*>(value.get()))
            throw runtime_error("StyleSheet:SetDerives only accepts StyleSheet instances");
        updated.push_back(value);
    }
    derives.swap(updated);
    styleRulesChangedSignal();
}

StyleLink::StyleLink()
{
    setName(sStyleLink);
}

StyleSheet* StyleLink::getStyleSheet() const
{
    return styleSheet.lock().get();
}

void StyleLink::setStyleSheet(StyleSheet* value)
{
    StyleSheet* current = getStyleSheet();
    if (current == value)
        return;

    styleSheet = value ? shared_dynamic_cast<StyleSheet>(shared_from(value))
                       : shared_ptr<StyleSheet>();
    reconnectStyleSheets();

    raisePropertyChanged(propLinkedStyleSheet);
    refreshLinkedSubtree();
}

void StyleLink::refreshLinkedSubtree()
{
    applyResolvedStylesToSubtree(getParent());
}

void StyleLink::reconnectStyleSheets()
{
    styleRulesChangedConnections.clear();
    for (StyleSheet* sheet : getStyleSheetCascade(getStyleSheet()))
        styleRulesChangedConnections.push_back(
            std::unique_ptr<rbx::signals::scoped_connection>(
                new rbx::signals::scoped_connection(
                    sheet->styleRulesChangedSignal.connect(
                        [this]() { onLinkedStyleRulesChanged(); }))));
}

void StyleLink::onLinkedStyleRulesChanged()
{
    reconnectStyleSheets();
    refreshLinkedSubtree();
}

void StyleLink::onAncestorChanged(const AncestorChanged& event)
{
    Super::onAncestorChanged(event);
    refreshLinkedSubtree();
}

StyleDerive::StyleDerive()
    : priority(0)
{
    setName(sStyleDerive);
}

void StyleDerive::setPriority(int value)
{
    if (priority == value)
        return;
    priority = value;
    raisePropertyChanged(propStyleDerivePriority);
}

StyleSheet* StyleDerive::getStyleSheet() const
{
    return styleSheet.lock().get();
}

void StyleDerive::setStyleSheet(StyleSheet* value)
{
    StyleSheet* current = getStyleSheet();
    if (current == value)
        return;
    styleSheet = value ? shared_dynamic_cast<StyleSheet>(shared_from(value))
                       : shared_ptr<StyleSheet>();
    raisePropertyChanged(propDerivedStyleSheet);
}

} // namespace RBX
