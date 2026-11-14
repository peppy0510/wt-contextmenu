// Windows 11 modern context menu entries for Windows Terminal.
// Loaded by Explorer through the sparse package declared in package/AppxManifest.xml.

#include <windows.h>
#include <shlwapi.h>
#include <shobjidl_core.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <wrl/module.h>

#include <string>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "runtimeobject.lib")

using Microsoft::WRL::ClassicCom;
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::InProc;
using Microsoft::WRL::Module;
using Microsoft::WRL::RuntimeClass;
using Microsoft::WRL::RuntimeClassFlags;

static HMODULE g_module = nullptr;

static std::wstring ExpandEnv(const wchar_t* text) {
    wchar_t buffer[MAX_PATH * 2];
    DWORD length = ExpandEnvironmentStringsW(text, buffer, ARRAYSIZE(buffer));
    if (length == 0 || length > ARRAYSIZE(buffer)) {
        return L"";
    }
    return buffer;
}

static std::wstring ModuleDir() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(g_module, path, ARRAYSIZE(path));
    PathRemoveFileSpecW(path);
    return path;
}

// wt treats ';' as a command separator, and a trailing backslash would escape the closing quote.
static std::wstring QuoteForWt(std::wstring dir) {
    if (!dir.empty() && dir.back() == L'\\') {
        dir += L'.';
    }
    std::wstring escaped;
    for (wchar_t ch : dir) {
        if (ch == L';') {
            escaped += L'\\';
        }
        escaped += ch;
    }
    return L"\"" + escaped + L"\"";
}

static void Spawn(std::wstring commandLine, const std::wstring& workingDir, DWORD flags) {
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    const wchar_t* cwd = workingDir.empty() ? nullptr : workingDir.c_str();
    if (CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE, flags, nullptr, cwd, &startup,
                       &process)) {
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }
}

class TerminalCommandBase
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IExplorerCommand, IObjectWithSite> {
public:
    IFACEMETHODIMP GetTitle(IShellItemArray*, LPWSTR* name) override { return SHStrDupW(Title(), name); }

    IFACEMETHODIMP GetIcon(IShellItemArray*, LPWSTR* icon) override {
        std::wstring path = ModuleDir() + L"\\wt.ico";
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

    IFACEMETHODIMP GetState(IShellItemArray*, BOOL, EXPCMDSTATE* state) override {
        *state = ECS_ENABLED;
        return S_OK;
    }

    IFACEMETHODIMP Invoke(IShellItemArray* items, IBindCtx*) override {
        std::wstring dir = ResolveDirectory(items);
        if (dir.empty()) {
            dir = ExpandEnv(L"%USERPROFILE%");
        }
        Launch(dir);
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

    IFACEMETHODIMP SetSite(IUnknown* site) override {
        site_ = site;
        return S_OK;
    }

    IFACEMETHODIMP GetSite(REFIID riid, void** result) override {
        *result = nullptr;
        if (!site_) {
            return E_FAIL;
        }
        return site_.CopyTo(riid, result);
    }

protected:
    virtual const wchar_t* Title() = 0;
    virtual void Launch(const std::wstring& dir) = 0;

private:
    // Selection first; on a folder background the selection is empty and the folder comes from the site.
    std::wstring ResolveDirectory(IShellItemArray* items) {
        ComPtr<IShellItem> item;
        DWORD count = 0;
        if (items && SUCCEEDED(items->GetCount(&count)) && count > 0) {
            items->GetItemAt(0, &item);
        } else if (site_) {
            ComPtr<IServiceProvider> provider;
            ComPtr<IFolderView> view;
            if (SUCCEEDED(site_.As(&provider)) &&
                SUCCEEDED(provider->QueryService(SID_SFolderView, IID_PPV_ARGS(&view)))) {
                view->GetFolder(IID_PPV_ARGS(&item));
            }
        }
        if (!item) {
            return L"";
        }
        PWSTR path = nullptr;
        if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
            return L"";
        }
        std::wstring result = path;
        CoTaskMemFree(path);
        return result;
    }

    ComPtr<IUnknown> site_;
};

class __declspec(uuid("8c933941-e1ce-4879-b0ac-f485fb783115")) OpenTerminal final : public TerminalCommandBase {
protected:
    const wchar_t* Title() override { return L"Windows Terminal"; }

    void Launch(const std::wstring& dir) override {
        std::wstring wt = ExpandEnv(L"%LOCALAPPDATA%\\Microsoft\\WindowsApps\\wt.exe");
        Spawn(L"\"" + wt + L"\" -d " + QuoteForWt(dir), dir, 0);
    }
};

// Runs the existing "WindowsTerminalAdmin" scheduled task (highest privileges, no UAC prompt).
// The task's wtadmintask.bat reads the directory from %TEMP%\wtcwd.tmp with `set /p`, so the file
// is written in the OEM code page that cmd uses.
class __declspec(uuid("fc265ada-8e17-4da7-9862-35e566508826")) OpenTerminalAdmin final
    : public TerminalCommandBase {
protected:
    const wchar_t* Title() override { return L"Windows Terminal (Admin)"; }

    void Launch(const std::wstring& dir) override {
        std::wstring line = QuoteForWt(dir) + L"\r\n";
        int size = WideCharToMultiByte(CP_OEMCP, 0, line.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (size <= 1) {
            return;
        }
        std::string bytes(size - 1, '\0');
        WideCharToMultiByte(CP_OEMCP, 0, line.c_str(), -1, bytes.data(), size, nullptr, nullptr);

        std::wstring tmp = ExpandEnv(L"%TEMP%\\wtcwd.tmp");
        HANDLE file = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            return;
        }
        DWORD written = 0;
        BOOL ok = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr);
        CloseHandle(file);
        if (!ok) {
            return;
        }

        std::wstring schtasks = ExpandEnv(L"%SystemRoot%\\System32\\schtasks.exe");
        Spawn(L"\"" + schtasks + L"\" /Run /TN WindowsTerminalAdmin", L"", CREATE_NO_WINDOW);
    }
};

CoCreatableClass(OpenTerminal);
CoCreatableClass(OpenTerminalAdmin);

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** result) {
    return Module<InProc>::GetModule().GetClassObject(clsid, riid, result);
}

STDAPI DllCanUnloadNow() {
    return Module<InProc>::GetModule().GetObjectCount() == 0 ? S_OK : S_FALSE;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = module;
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}
