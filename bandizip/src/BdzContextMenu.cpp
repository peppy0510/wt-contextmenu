// Windows 11 modern context menu entries for Bandizip 7.21, which predates Bandizip's own support (7.22).
// Each entry runs Bandizip.exe with the command its classic shell extension (bdzshl.x64.dll) uses.

#include <windows.h>
#include <shlwapi.h>
#include <shobjidl_core.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <wrl/module.h>

#include <string>
#include <vector>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "runtimeobject.lib")

using Microsoft::WRL::ClassicCom;
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::InProc;
using Microsoft::WRL::Module;
using Microsoft::WRL::RuntimeClass;
using Microsoft::WRL::RuntimeClassFlags;

// CreateProcess accepts 32767 characters including the terminator.
constexpr size_t kMaxCommandLine = 32000;

static std::wstring ReadRegString(HKEY root, const wchar_t* subKey, const wchar_t* name) {
    wchar_t buffer[MAX_PATH * 2];
    DWORD size = sizeof(buffer);
    if (RegGetValueW(root, subKey, name, RRF_RT_REG_SZ, nullptr, buffer, &size) != ERROR_SUCCESS) {
        return L"";
    }
    return buffer;
}

static const std::wstring& BandizipExe() {
    static const std::wstring path = [] {
        std::wstring exe = ReadRegString(
            HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Bandizip.exe", nullptr);
        return !exe.empty() && PathFileExistsW(exe.c_str()) ? exe : std::wstring();
    }();
    return path;
}

// Bandizip registers the extensions it handles as value names of this key (".zip", ".7z", ...).
static const std::vector<std::wstring>& ArchiveExtensions() {
    static const std::vector<std::wstring> extensions = [] {
        std::vector<std::wstring> result;
        HKEY key = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Bandizip\\Capabilities\\FileAssociations", 0, KEY_READ,
                          &key) != ERROR_SUCCESS) {
            return result;
        }
        for (DWORD index = 0;; ++index) {
            wchar_t name[64];
            DWORD length = ARRAYSIZE(name);
            LONG status = RegEnumValueW(key, index, name, &length, nullptr, nullptr, nullptr, nullptr);
            if (status == ERROR_NO_MORE_ITEMS) {
                break;
            }
            if (status == ERROR_SUCCESS && name[0] == L'.') {
                result.emplace_back(name);
            }
        }
        RegCloseKey(key);
        return result;
    }();
    return extensions;
}

static bool IsArchive(const std::wstring& path) {
    const wchar_t* extension = PathFindExtensionW(path.c_str());
    for (const std::wstring& known : ArchiveExtensions()) {
        if (_wcsicmp(extension, known.c_str()) == 0) {
            return true;
        }
    }
    return false;
}

static bool GetPath(IShellItem* item, std::wstring& path) {
    PWSTR buffer = nullptr;
    if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &buffer))) {
        return false;
    }
    path = buffer;
    CoTaskMemFree(buffer);
    return true;
}

static bool GetPaths(IShellItemArray* items, std::vector<std::wstring>& paths) {
    DWORD count = 0;
    if (!items || FAILED(items->GetCount(&count)) || count == 0) {
        return false;
    }
    paths.reserve(count);
    for (DWORD index = 0; index < count; ++index) {
        ComPtr<IShellItem> item;
        std::wstring path;
        if (FAILED(items->GetItemAt(index, &item)) || !GetPath(item.Get(), path)) {
            return false;
        }
        paths.push_back(std::move(path));
    }
    return true;
}

// A selection normally shares one folder; running Bandizip there with bare names keeps large
// selections under the command-line limit.
static void Launch(const wchar_t* command, const std::vector<std::wstring>& paths) {
    const std::wstring& first = paths.front();
    std::wstring folder = first.substr(0, first.find_last_of(L'\\') + 1);
    bool relative = !folder.empty();
    for (const std::wstring& path : paths) {
        if (path.size() <= folder.size() || _wcsnicmp(path.c_str(), folder.c_str(), folder.size()) != 0 ||
            path.find(L'\\', folder.size()) != std::wstring::npos) {
            relative = false;
            break;
        }
    }

    std::wstring commandLine = L"\"" + BandizipExe() + L"\" " + command;
    for (const std::wstring& path : paths) {
        commandLine += L" \"";
        commandLine += relative ? path.substr(folder.size()) : path;
        commandLine += L"\"";
    }
    if (commandLine.size() > kMaxCommandLine) {
        MessageBoxW(nullptr,
                    L"Too many items are selected to pass to Bandizip at once.\n"
                    L"Use the Bandizip entry in the classic menu (Shift+right-click) instead.",
                    L"Bandizip", MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
        return;
    }

    // Lets the Bandizip window come to the front instead of opening behind Explorer.
    AllowSetForegroundWindow(ASFW_ANY);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    const wchar_t* cwd = folder.empty() ? nullptr : folder.c_str();
    if (CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, cwd, &startup, &process)) {
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }
}

class BandizipCommandBase : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IExplorerCommand> {
public:
    IFACEMETHODIMP GetTitle(IShellItemArray* items, LPWSTR* name) override {
        return SHStrDupW(Title(items).c_str(), name);
    }

    IFACEMETHODIMP GetIcon(IShellItemArray*, LPWSTR* icon) override {
        const std::wstring& exe = BandizipExe();
        if (exe.empty()) {
            *icon = nullptr;
            return E_FAIL;
        }
        std::wstring path = exe.substr(0, exe.find_last_of(L'\\')) + L"\\shellicons\\bandizip.ico";
        return SHStrDupW(path.c_str(), icon);
    }

    IFACEMETHODIMP GetToolTip(IShellItemArray*, LPWSTR* tip) override {
        *tip = nullptr;
        return E_NOTIMPL;
    }

    IFACEMETHODIMP GetCanonicalName(GUID* guid) override {
        *guid = GUID_NULL;
        return S_OK;
    }

    IFACEMETHODIMP GetState(IShellItemArray* items, BOOL, EXPCMDSTATE* state) override {
        *state = !BandizipExe().empty() && Applies(items) ? ECS_ENABLED : ECS_HIDDEN;
        return S_OK;
    }

    IFACEMETHODIMP Invoke(IShellItemArray* items, IBindCtx*) override {
        std::vector<std::wstring> paths;
        if (!BandizipExe().empty() && GetPaths(items, paths)) {
            Run(paths);
        }
        return S_OK;
    }

    IFACEMETHODIMP GetFlags(EXPCMDFLAGS* flags) override {
        *flags = ECF_DEFAULT;
        return S_OK;
    }

    IFACEMETHODIMP EnumSubCommands(IEnumExplorerCommand** commands) override {
        *commands = nullptr;
        return E_NOTIMPL;
    }

protected:
    virtual std::wstring Title(IShellItemArray* items) = 0;
    virtual bool Applies(IShellItemArray* items) = 0;
    virtual void Run(const std::vector<std::wstring>& paths) = 0;
};

class __declspec(uuid("387eb6f1-3706-4ea1-b860-ed6c7734a1a1")) CompressCommand final : public BandizipCommandBase {
protected:
    std::wstring Title(IShellItemArray*) override { return L"Create an archive (Bandizip)..."; }

    // Items inside a zip folder, Control Panel and the like have no file-system path.
    bool Applies(IShellItemArray* items) override {
        ComPtr<IShellItem> item;
        std::wstring path;
        return items && SUCCEEDED(items->GetItemAt(0, &item)) && GetPath(item.Get(), path);
    }

    void Run(const std::vector<std::wstring>& paths) override { Launch(L"cd", paths); }
};

class __declspec(uuid("367445c0-5776-49de-a982-c3eaa8b2443b")) ExtractCommand final : public BandizipCommandBase {
protected:
    std::wstring Title(IShellItemArray* items) override {
        DWORD count = 0;
        if (items && SUCCEEDED(items->GetCount(&count)) && count > 1) {
            return L"Extract each archive Here (Smart) (" + std::to_wstring(count) + L")";
        }
        return L"Extract Here (Smart)";
    }

    bool Applies(IShellItemArray* items) override {
        std::vector<std::wstring> paths;
        if (!GetPaths(items, paths)) {
            return false;
        }
        for (const std::wstring& path : paths) {
            if (!IsArchive(path)) {
                return false;
            }
        }
        return true;
    }

    void Run(const std::vector<std::wstring>& paths) override { Launch(L"bx -target:auto", paths); }
};

CoCreatableClass(CompressCommand);
CoCreatableClass(ExtractCommand);

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** result) {
    return Module<InProc>::GetModule().GetClassObject(clsid, riid, result);
}

STDAPI DllCanUnloadNow() {
    return Module<InProc>::GetModule().GetObjectCount() == 0 ? S_OK : S_FALSE;
}
