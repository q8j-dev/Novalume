#pragma once

#include <cstdio>
#include <algorithm>
#include <charconv>
#include <cctype>
#include <mutex>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace RBX::Logging {

template<typename T>
decltype(auto) argument(T&& value) noexcept {
    if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::string>)
        return value.c_str();
    else
        return std::forward<T>(value);
}

inline void write(int channel, const char* message) {
    if (channel == 0 || !message) return;
    std::fprintf(stderr, "%s\n", message);
}

template<typename... Args>
    requires (sizeof...(Args) > 0)
void write(int channel, const char* format, Args&&... args) {
    if (channel == 0 || !format) return;
    std::fprintf(stderr, format, argument(std::forward<Args>(args))...);
    std::fputc('\n', stderr);
}

} // namespace RBX::Logging

enum FastVarType {
    FASTVARTYPE_STATIC = 1 << 0,
    FASTVARTYPE_DYNAMIC = 1 << 1,
    FASTVARTYPE_SYNC = 1 << 2,
    FASTVARTYPE_AB_NEWUSERS = 1 << 3,
    FASTVARTYPE_AB_NEWSTUDIOUSERS = 1 << 4,
    FASTVARTYPE_AB_ALLUSERS = 1 << 5,
    FASTVARTYPE_ANY = 0x7fffffff
};

namespace FLog {

using Channel = int;
using VariableVisitor = void (*)(const std::string&, const std::string&, void*);
inline Channel Always = 1;
inline Channel Error = 1;

inline void FastLogS(Channel channel, const char* message, void*) {
    RBX::Logging::write(channel, message);
}

enum class ValueKind { Boolean, Integer, String };

struct Variable {
    std::string name;
    void* value;
    ValueKind kind;
    FastVarType type;
    bool* synchronized;
};

struct PendingValue {
    std::string value;
    FastVarType type;
};

inline std::mutex& registryMutex() {
    static std::mutex mutex;
    return mutex;
}

inline std::unordered_map<std::string, Variable>& registry() {
    static std::unordered_map<std::string, Variable> values;
    return values;
}

inline std::unordered_map<std::string, PendingValue>& pendingValues() {
    static std::unordered_map<std::string, PendingValue> values;
    return values;
}

inline std::unordered_map<std::string, std::unique_ptr<bool>>& ownedBooleanValues() {
    static std::unordered_map<std::string, std::unique_ptr<bool>> values;
    return values;
}

inline std::unordered_map<std::string, std::unique_ptr<int>>& ownedIntegerValues() {
    static std::unordered_map<std::string, std::unique_ptr<int>> values;
    return values;
}

inline std::unordered_map<std::string, std::unique_ptr<std::string>>& ownedStringValues() {
    static std::unordered_map<std::string, std::unique_ptr<std::string>> values;
    return values;
}

inline bool typeMatches(FastVarType variableType, FastVarType requestedType) {
    return requestedType == FASTVARTYPE_ANY || variableType == requestedType;
}

inline bool parseBoolean(const std::string& text, bool& result) {
    std::string normalized(text);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (normalized == "true" || normalized == "1") {
        result = true;
        return true;
    }
    if (normalized == "false" || normalized == "0") {
        result = false;
        return true;
    }
    return false;
}

inline bool assign(Variable& variable, const std::string& text, FastVarType requestedType) {
    if (!typeMatches(variable.type, requestedType))
        return false;

    switch (variable.kind) {
    case ValueKind::Boolean: {
        bool parsed = false;
        if (!parseBoolean(text, parsed))
            return false;
        *static_cast<bool*>(variable.value) = parsed;
        break;
    }
    case ValueKind::Integer: {
        int parsed = 0;
        const char* begin = text.data();
        const char* end = begin + text.size();
        const std::from_chars_result result = std::from_chars(begin, end, parsed);
        if (result.ec != std::errc() || result.ptr != end)
            return false;
        *static_cast<int*>(variable.value) = parsed;
        break;
    }
    case ValueKind::String:
        *static_cast<std::string*>(variable.value) = text;
        break;
    }

    if (variable.synchronized && requestedType == FASTVARTYPE_SYNC)
        *variable.synchronized = true;
    return true;
}

inline std::string stringify(const Variable& variable) {
    switch (variable.kind) {
    case ValueKind::Boolean:
        return *static_cast<const bool*>(variable.value) ? "true" : "false";
    case ValueKind::Integer:
        return std::to_string(*static_cast<const int*>(variable.value));
    case ValueKind::String:
        return *static_cast<const std::string*>(variable.value);
    }
    return {};
}

inline void registerVariable(const char* name, void* value, ValueKind kind,
    FastVarType type, bool* synchronized = nullptr) {
    std::lock_guard<std::mutex> lock(registryMutex());
    Variable variable{name, value, kind, type, synchronized};
    registry().insert_or_assign(name, variable);
    const auto pending = pendingValues().find(name);
    if (pending != pendingValues().end() && assign(registry().at(name), pending->second.value, pending->second.type))
        pendingValues().erase(pending);
}

class Registrar {
public:
    Registrar(const char* name, bool* value, FastVarType type, bool* synchronized = nullptr) {
        registerVariable(name, value, ValueKind::Boolean, type, synchronized);
    }
    Registrar(const char* name, int* value, FastVarType type, bool* synchronized = nullptr) {
        registerVariable(name, value, ValueKind::Integer, type, synchronized);
    }
    Registrar(const char* name, std::string* value, FastVarType type) {
        registerVariable(name, value, ValueKind::String, type);
    }
};

inline bool SetValue(const std::string& name, const std::string& value,
    FastVarType type = FASTVARTYPE_ANY, bool = false) {
    std::lock_guard<std::mutex> lock(registryMutex());
    const auto found = registry().find(name);
    if (found != registry().end())
        return assign(found->second, value, type);
    pendingValues().insert_or_assign(name, PendingValue{value, type});
    return false;
}

inline bool GetValue(const char* name, std::string& value, bool = false) {
    std::lock_guard<std::mutex> lock(registryMutex());
    const auto found = registry().find(name ? name : "");
    if (found == registry().end())
        return false;
    value = stringify(found->second);
    return true;
}

inline bool DefineBoolean(const std::string& name, bool defaultValue, bool& value) {
    std::lock_guard<std::mutex> lock(registryMutex());
    const auto found = registry().find(name);
    if (found != registry().end()) {
        if (found->second.kind != ValueKind::Boolean)
            return false;
        value = *static_cast<bool*>(found->second.value);
        return true;
    }
    std::unique_ptr<bool> storage(new bool(defaultValue));
    const auto pending = pendingValues().find(name);
    if (pending != pendingValues().end()) {
        if (!parseBoolean(pending->second.value, *storage))
            return false;
        pendingValues().erase(pending);
    }
    bool* pointer = storage.get();
    ownedBooleanValues().insert_or_assign(name, std::move(storage));
    registry().insert_or_assign(name,
        Variable{name, pointer, ValueKind::Boolean, FASTVARTYPE_DYNAMIC, nullptr});
    value = *pointer;
    return true;
}

inline bool DefineInteger(const std::string& name, int defaultValue, int& value) {
    std::lock_guard<std::mutex> lock(registryMutex());
    const auto found = registry().find(name);
    if (found != registry().end()) {
        if (found->second.kind != ValueKind::Integer)
            return false;
        value = *static_cast<int*>(found->second.value);
        return true;
    }
    std::unique_ptr<int> storage(new int(defaultValue));
    const auto pending = pendingValues().find(name);
    if (pending != pendingValues().end()) {
        const char* begin = pending->second.value.data();
        const char* end = begin + pending->second.value.size();
        const std::from_chars_result parsed = std::from_chars(begin, end, *storage);
        if (parsed.ec != std::errc() || parsed.ptr != end)
            return false;
        pendingValues().erase(pending);
    }
    int* pointer = storage.get();
    ownedIntegerValues().insert_or_assign(name, std::move(storage));
    registry().insert_or_assign(name,
        Variable{name, pointer, ValueKind::Integer, FASTVARTYPE_DYNAMIC, nullptr});
    value = *pointer;
    return true;
}

inline bool DefineString(const std::string& name, const std::string& defaultValue,
    std::string& value) {
    std::lock_guard<std::mutex> lock(registryMutex());
    const auto found = registry().find(name);
    if (found != registry().end()) {
        if (found->second.kind != ValueKind::String)
            return false;
        value = *static_cast<std::string*>(found->second.value);
        return true;
    }
    std::unique_ptr<std::string> storage(new std::string(defaultValue));
    const auto pending = pendingValues().find(name);
    if (pending != pendingValues().end()) {
        *storage = pending->second.value;
        pendingValues().erase(pending);
    }
    std::string* pointer = storage.get();
    ownedStringValues().insert_or_assign(name, std::move(storage));
    registry().insert_or_assign(name,
        Variable{name, pointer, ValueKind::String, FASTVARTYPE_DYNAMIC, nullptr});
    value = *pointer;
    return true;
}

inline void ForEachVariable(VariableVisitor visitor, void* context,
    FastVarType type = FASTVARTYPE_ANY) {
    if (!visitor)
        return;
    std::vector<std::pair<std::string, std::string>> snapshot;
    {
        std::lock_guard<std::mutex> lock(registryMutex());
        snapshot.reserve(registry().size());
        for (const auto& [name, variable] : registry())
            if (typeMatches(variable.type, type))
                snapshot.emplace_back(name, stringify(variable));
    }
    for (const auto& [name, value] : snapshot)
        visitor(name, value, context);
}

inline std::uint32_t GetNumSynchronizedVariable() {
    std::lock_guard<std::mutex> lock(registryMutex());
    std::uint32_t count = 0;
    for (const auto& [name, variable] : registry()) {
        (void)name;
        if (typeMatches(variable.type, FASTVARTYPE_SYNC))
            ++count;
    }
    return count;
}

inline void RegisterLogGroup(const char* name, Channel* value) {
    registerVariable(name, value, ValueKind::Integer, FASTVARTYPE_STATIC);
}

inline void RegisterFlag(const char* name, bool* value) {
    registerVariable(name, value, ValueKind::Boolean, FASTVARTYPE_STATIC);
}

inline bool SetValueFromServer(const std::string& name, const std::string& value) {
    return SetValue(name, value, FASTVARTYPE_SYNC);
}

inline void ResetSynchronizedVariablesState() {
    std::lock_guard<std::mutex> lock(registryMutex());
    for (auto& [name, variable] : registry())
        if (variable.synchronized)
            *variable.synchronized = false;
}

} // namespace FLog

#define LOGGROUP(name) namespace FLog { extern int name; }
#define DYNAMIC_LOGGROUP(name) namespace DFLog { extern int name; }
#define LOGVARIABLE(name, value) namespace FLog { int name = value; Registrar name##Registrar(#name, &name, FASTVARTYPE_STATIC); }
#define DYNAMIC_LOGVARIABLE(name, value) namespace DFLog { int name = value; FLog::Registrar name##Registrar(#name, &name, FASTVARTYPE_DYNAMIC); }

#define FASTFLAG(name) namespace FFlag { extern bool name; }
#define FASTFLAGVARIABLE(name, value) namespace FFlag { bool name = value; FLog::Registrar name##Registrar(#name, &name, FASTVARTYPE_STATIC); }
#define DYNAMIC_FASTFLAG(name) namespace DFFlag { extern bool name; }
#define DYNAMIC_FASTFLAGVARIABLE(name, value) namespace DFFlag { bool name = value; FLog::Registrar name##Registrar(#name, &name, FASTVARTYPE_DYNAMIC); }

#define FASTINT(name) namespace FInt { extern int name; }
#define FASTINTVARIABLE(name, value) namespace FInt { int name = value; FLog::Registrar name##Registrar(#name, &name, FASTVARTYPE_STATIC); }
#define DYNAMIC_FASTINT(name) namespace DFInt { extern int name; }
#define DYNAMIC_FASTINTVARIABLE(name, value) namespace DFInt { int name = value; FLog::Registrar name##Registrar(#name, &name, FASTVARTYPE_DYNAMIC); }

#define FASTSTRING(name) namespace FString { extern std::string name; }
#define FASTSTRINGVARIABLE(name, value) namespace FString { std::string name = value; FLog::Registrar name##Registrar(#name, &name, FASTVARTYPE_STATIC); }
#define DYNAMIC_FASTSTRING(name) namespace DFString { extern std::string name; }
#define DYNAMIC_FASTSTRINGVARIABLE(name, value) namespace DFString { std::string name = value; FLog::Registrar name##Registrar(#name, &name, FASTVARTYPE_DYNAMIC); }

#define SYNCHRONIZED_FASTFLAG(name) namespace SFFlag { extern bool name; extern bool* name##IsSync; bool get##name(); }
#define SYNCHRONIZED_FASTFLAGVARIABLE(name, value) namespace SFFlag { bool name = value; bool name##SyncState = false; bool* name##IsSync = &name##SyncState; bool get##name() { return name; } FLog::Registrar name##Registrar(#name, &name, FASTVARTYPE_SYNC, name##IsSync); }
#define SYNCHRONIZED_FASTINT(name) namespace SFInt { extern int name; extern bool* name##IsSync; int get##name(); }
#define SYNCHRONIZED_FASTINTVARIABLE(name, value) namespace SFInt { int name = value; bool name##SyncState = false; bool* name##IsSync = &name##SyncState; int get##name() { return name; } FLog::Registrar name##Registrar(#name, &name, FASTVARTYPE_SYNC, name##IsSync); }

#define ABTEST_NEWUSERS_VARIABLE(name) namespace FInt { int name = 0; FLog::Registrar name##Registrar(#name, &name, FASTVARTYPE_AB_NEWUSERS); }
#define ABTEST_NEWSTUDIOUSERS_VARIABLE(name) namespace FInt { int name = 0; FLog::Registrar name##Registrar(#name, &name, FASTVARTYPE_AB_NEWSTUDIOUSERS); }
#define ABTEST_ALLUSERS_VARIABLE(name) namespace FInt { int name = 0; FLog::Registrar name##Registrar(#name, &name, FASTVARTYPE_AB_ALLUSERS); }

#define FASTLOG(channel, format) ::RBX::Logging::write((channel), (format))
#define FASTLOG1(channel, format, a1) ::RBX::Logging::write((channel), (format), (a1))
#define FASTLOG2(channel, format, a1, a2) ::RBX::Logging::write((channel), (format), (a1), (a2))
#define FASTLOG3(channel, format, a1, a2, a3) ::RBX::Logging::write((channel), (format), (a1), (a2), (a3))
#define FASTLOG4(channel, format, a1, a2, a3, a4) ::RBX::Logging::write((channel), (format), (a1), (a2), (a3), (a4))
#define FASTLOG5(channel, format, a1, a2, a3, a4, a5) ::RBX::Logging::write((channel), (format), (a1), (a2), (a3), (a4), (a5))
#define FASTLOGS(channel, format, value) ::RBX::Logging::write((channel), (format), (value))
#define FASTLOG1F FASTLOG1
#define FASTLOG2F FASTLOG2
#define FASTLOG3F FASTLOG3
#define FASTLOG4F FASTLOG4
