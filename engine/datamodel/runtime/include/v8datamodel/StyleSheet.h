#pragma once

#include "v8tree/Instance.h"

namespace RBX {

extern const char* const sStyleBase;
extern const char* const sStyleRule;
extern const char* const sStyleSheet;
extern const char* const sStyleLink;
extern const char* const sStyleDerive;

class StyleRule;
class StyleSheet;

class StyleBase
    : public DescribedNonCreatable<StyleBase, Instance, sStyleBase>
{
public:
    typedef DescribedNonCreatable<StyleBase, Instance, sStyleBase> Super;

    shared_ptr<const Instances> getStyleRules();
    void insertStyleRule(shared_ptr<Instance> rule, int priority);
    void setStyleRules(shared_ptr<const Instances> rules);

    rbx::signal<void()> styleRulesChangedSignal;

protected:
    StyleBase();
    void onChildAdded(Instance* child) override;
    void onChildRemoving(Instance* child) override;
};

class StyleRule
    : public DescribedCreatable<StyleRule, StyleBase, sStyleRule>
{
public:
    typedef DescribedCreatable<StyleRule, StyleBase, sStyleRule> Super;

    StyleRule();

    int getPriority() const { return priority; }
    void setPriority(int value);
    std::string getSelector() const { return selector; }
    void setSelector(std::string value);
    std::string getSelectorError() const { return selectorError; }

    shared_ptr<const Reflection::ValueTable> getProperties();
    shared_ptr<const Reflection::ValueTable> getPropertiesResolved();
    Reflection::Variant getProperty(std::string name);
    Reflection::Variant getPropertyResolved(std::string name);
    void setProperties(shared_ptr<const Reflection::ValueTable> values);
    void setProperty(std::string name, Reflection::Variant value);

    Reflection::Variant getDefaultPropertyTransition();
    void setDefaultPropertyTransition(Reflection::Variant value);
    shared_ptr<const Reflection::ValueTable> getPropertyTransitions();
    void setPropertyTransition(std::string name, Reflection::Variant value);
    void setPropertyTransitions(shared_ptr<const Reflection::ValueTable> values);

    rbx::signal<void(std::string)> styleRulePropertyChangedSignal;

private:
    void propertyChanged(const std::string& name);

    int priority;
    std::string selector;
    std::string selectorError;
    Reflection::ValueTable properties;
    Reflection::Variant defaultTransition;
    Reflection::ValueTable transitions;
};

class StyleSheet
    : public DescribedCreatable<StyleSheet, StyleBase, sStyleSheet>
{
public:
    typedef DescribedCreatable<StyleSheet, StyleBase, sStyleSheet> Super;

    StyleSheet();
    shared_ptr<const Instances> getDerives();
    void setDerives(shared_ptr<const Instances> values);

private:
    Instances derives;
};

class StyleLink
    : public DescribedCreatable<StyleLink, Instance, sStyleLink>
{
public:
    typedef DescribedCreatable<StyleLink, Instance, sStyleLink> Super;

    StyleLink();
    StyleSheet* getStyleSheet() const;
    void setStyleSheet(StyleSheet* value);

protected:
    void onAncestorChanged(const AncestorChanged& event) override;

private:
    void refreshLinkedSubtree();
    void reconnectStyleSheets();
    void onLinkedStyleRulesChanged();

    weak_ptr<StyleSheet> styleSheet;
    std::vector<std::unique_ptr<rbx::signals::scoped_connection> > styleRulesChangedConnections;
};

class StyleDerive
    : public DescribedCreatable<StyleDerive, Instance, sStyleDerive>
{
public:
    typedef DescribedCreatable<StyleDerive, Instance, sStyleDerive> Super;

    StyleDerive();
    int getPriority() const { return priority; }
    void setPriority(int value);
    StyleSheet* getStyleSheet() const;
    void setStyleSheet(StyleSheet* value);

private:
    int priority;
    weak_ptr<StyleSheet> styleSheet;
};

// Resolves the StyleLink visible to an instance, matches its CollectionService
// tags against StyleRule selectors, and applies the resulting native
// properties/pseudo-elements. This is the runtime half of React.Tag and is
// intentionally independent of Foundation's particular generated rule set.
bool applyResolvedStyles(Instance* instance);
bool applyResolvedStylesToSubtree(Instance* root);

} // namespace RBX
