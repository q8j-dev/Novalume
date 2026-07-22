#pragma once

#include "Script/ThreadRef.h"
#include "v8tree/Service.h"

#include <filesystem>
#include <map>
#include <string>

namespace RBX {

extern const char* const sLocalStorageService;
extern const char* const sAppStorageService;

class LocalStorageService
    : public DescribedNonCreatable<LocalStorageService, Instance, sLocalStorageService>
    , public Service
{
public:
    explicit LocalStorageService(std::string fileName = "localStorage.json");

    static void setStorageRoot(std::filesystem::path root);

    void flush();
    std::string getItem(std::string key);
    void setItem(std::string key, std::string value);
    void whenLoaded(Lua::WeakFunctionRef callback);

    rbx::signal<void(std::string, std::string)> itemWasSet;
    rbx::signal<void()> storeWasCleared;

protected:
    void load();

private:
    std::filesystem::path path() const;

    static std::filesystem::path storageRoot;
    std::string fileName;
    std::map<std::string, std::string> items;
    bool loaded;
    bool dirty;
};

class AppStorageService
    : public DescribedNonCreatable<AppStorageService, LocalStorageService,
          sAppStorageService>
{
public:
    typedef DescribedNonCreatable<AppStorageService, LocalStorageService,
        sAppStorageService> Super;
    AppStorageService();
};

} // namespace RBX
