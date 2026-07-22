#pragma once

#include "v8tree/Instance.h"
#include "v8tree/Service.h"

namespace RBX {

extern const char* const sBaseCoreGuiConfiguration;
extern const char* const sCapturesViewConfiguration;
extern const char* const sPlayerListConfiguration;
extern const char* const sSelfViewConfiguration;
extern const char* const sCoreGuiConfiguration;

class BaseCoreGuiConfiguration
    : public DescribedNonCreatable<BaseCoreGuiConfiguration, Instance, sBaseCoreGuiConfiguration>
{
public:
    explicit BaseCoreGuiConfiguration(const char* name = "BaseCoreGuiConfiguration");
    bool getEnabled() const { return enabled; }
    void setEnabled(bool value);

private:
    bool enabled;
};

class CapturesViewConfiguration
    : public DescribedCreatable<CapturesViewConfiguration, BaseCoreGuiConfiguration,
          sCapturesViewConfiguration>
{
public:
    CapturesViewConfiguration();
    bool getOpen() const { return open; }
    void setOpen(bool value);

private:
    bool open;
};

class PlayerListConfiguration
    : public DescribedCreatable<PlayerListConfiguration, BaseCoreGuiConfiguration,
          sPlayerListConfiguration>
{
public:
    PlayerListConfiguration();
    bool getOpen() const { return open; }
    void setOpen(bool value);

private:
    bool open;
};

class SelfViewConfiguration
    : public DescribedCreatable<SelfViewConfiguration, BaseCoreGuiConfiguration,
          sSelfViewConfiguration>
{
public:
    SelfViewConfiguration();
    bool getOpen() const { return open; }
    void setOpen(bool value);

private:
    bool open;
};

class CoreGuiConfiguration
    : public DescribedNonCreatable<CoreGuiConfiguration, Instance, sCoreGuiConfiguration>
    , public Service
{
public:
    CoreGuiConfiguration();

    CapturesViewConfiguration* getCapturesViewConfiguration() const;
    PlayerListConfiguration* getPlayerListConfiguration() const;
    SelfViewConfiguration* getSelfViewConfiguration() const;

private:
    boost::shared_ptr<CapturesViewConfiguration> capturesViewConfiguration;
    boost::shared_ptr<PlayerListConfiguration> playerListConfiguration;
    boost::shared_ptr<SelfViewConfiguration> selfViewConfiguration;
};

} // namespace RBX
