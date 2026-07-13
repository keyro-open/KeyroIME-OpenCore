// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// It is source-available under the KeyroIME OpenCore Non-Commercial Source
// License 1.0. See LICENSE. Commercial use requires a separate written license
// from 株式会社LocalPro.
// dll_exports.cpp
// Windows COM exports and TSF TIP registry registration for KeyroIME.dll.

#include "class_factory.h"
#include "module_path.h"
#include "tip_guid.h"

#include <msctf.h>
#include <new>
#include <strsafe.h>
#include <windows.h>

namespace KeyroIME {

namespace {

HMODULE g_moduleHandle = nullptr;
volatile LONG g_dllRefCount = 0;

constexpr wchar_t kComClsidRoot[] = L"SOFTWARE\\Classes\\CLSID\\";
constexpr wchar_t kTipRoot[] = L"SOFTWARE\\Microsoft\\CTF\\TIP\\";
constexpr wchar_t kServiceDescription[] = L"KeyroIME Text Service";
constexpr wchar_t kProfileDescription[] = L"KeyroIME Japanese Input";
constexpr LANGID kJapaneseLangId = 0x0411;

class ScopedComInitializer {
public:
    ScopedComInitializer()
        : hr_(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)),
          shouldUninitialize_(hr_ == S_OK || hr_ == S_FALSE)
    {
    }

    ~ScopedComInitializer()
    {
        if (shouldUninitialize_) {
            CoUninitialize();
        }
    }

    HRESULT Result() const
    {
        return hr_ == RPC_E_CHANGED_MODE ? S_OK : hr_;
    }

private:
    HRESULT hr_;
    bool shouldUninitialize_;
};

template <typename T>
void SafeRelease(T*& pointer)
{
    if (pointer) {
        pointer->Release();
        pointer = nullptr;
    }
}

HRESULT HResultFromWin32(LSTATUS status)
{
    return status == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(status);
}

HRESULT GetModulePath(wchar_t* path, DWORD pathCount)
{
    DWORD written = GetModuleFileNameW(g_moduleHandle, path, pathCount);
    if (written == 0) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (written >= pathCount) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }
    return S_OK;
}

HRESULT BuildPath(wchar_t* output, size_t outputCount, const wchar_t* left, const wchar_t* right)
{
    HRESULT hr = StringCchCopyW(output, outputCount, left);
    if (FAILED(hr)) {
        return hr;
    }
    return StringCchCatW(output, outputCount, right);
}

HRESULT BuildPath4(
    wchar_t* output,
    size_t outputCount,
    const wchar_t* first,
    const wchar_t* second,
    const wchar_t* third,
    const wchar_t* fourth)
{
    HRESULT hr = StringCchCopyW(output, outputCount, first);
    if (FAILED(hr)) return hr;
    hr = StringCchCatW(output, outputCount, second);
    if (FAILED(hr)) return hr;
    hr = StringCchCatW(output, outputCount, third);
    if (FAILED(hr)) return hr;
    return StringCchCatW(output, outputCount, fourth);
}

HRESULT CreateKey(HKEY root, const wchar_t* path, HKEY* key)
{
    LSTATUS status = RegCreateKeyExW(
        root,
        path,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        key,
        nullptr);
    return HResultFromWin32(status);
}

HRESULT SetStringValue(HKEY key, const wchar_t* name, const wchar_t* value)
{
    DWORD bytes = static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t));
    LSTATUS status = RegSetValueExW(
        key,
        name,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(value),
        bytes);
    return HResultFromWin32(status);
}

HRESULT SetDwordValue(HKEY key, const wchar_t* name, DWORD value)
{
    LSTATUS status = RegSetValueExW(
        key,
        name,
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&value),
        sizeof(value));
    return HResultFromWin32(status);
}

HRESULT RegisterComServer()
{
    wchar_t modulePath[MAX_PATH] = {};
    HRESULT hr = GetModulePath(modulePath, ARRAYSIZE(modulePath));
    if (FAILED(hr)) {
        return hr;
    }

    wchar_t clsidPath[256] = {};
    hr = BuildPath(clsidPath, ARRAYSIZE(clsidPath), kComClsidRoot, kTextServiceClsidString);
    if (FAILED(hr)) {
        return hr;
    }

    HKEY clsidKey = nullptr;
    hr = CreateKey(HKEY_LOCAL_MACHINE, clsidPath, &clsidKey);
    if (FAILED(hr)) {
        return hr;
    }

    hr = SetStringValue(clsidKey, nullptr, kServiceDescription);
    RegCloseKey(clsidKey);
    if (FAILED(hr)) {
        return hr;
    }

    wchar_t inprocPath[320] = {};
    hr = BuildPath(inprocPath, ARRAYSIZE(inprocPath), clsidPath, L"\\InprocServer32");
    if (FAILED(hr)) {
        return hr;
    }

    HKEY inprocKey = nullptr;
    hr = CreateKey(HKEY_LOCAL_MACHINE, inprocPath, &inprocKey);
    if (FAILED(hr)) {
        return hr;
    }

    hr = SetStringValue(inprocKey, nullptr, modulePath);
    if (SUCCEEDED(hr)) {
        hr = SetStringValue(inprocKey, L"ThreadingModel", L"Apartment");
    }
    RegCloseKey(inprocKey);
    return hr;
}

HRESULT RegisterTipRoot(const wchar_t* modulePath)
{
    wchar_t tipPath[256] = {};
    HRESULT hr = BuildPath(tipPath, ARRAYSIZE(tipPath), kTipRoot, kTextServiceClsidString);
    if (FAILED(hr)) {
        return hr;
    }

    HKEY tipKey = nullptr;
    hr = CreateKey(HKEY_LOCAL_MACHINE, tipPath, &tipKey);
    if (FAILED(hr)) {
        return hr;
    }

    hr = SetStringValue(tipKey, nullptr, kServiceDescription);
    if (SUCCEEDED(hr)) hr = SetStringValue(tipKey, L"Description", kServiceDescription);
    if (SUCCEEDED(hr)) hr = SetStringValue(tipKey, L"Display Description", kServiceDescription);
    if (SUCCEEDED(hr)) hr = SetStringValue(tipKey, L"IconFile", modulePath);
    if (SUCCEEDED(hr)) hr = SetDwordValue(tipKey, L"IconIndex", 0);
    if (SUCCEEDED(hr)) hr = SetDwordValue(tipKey, L"Enable", 1);

    RegCloseKey(tipKey);
    return hr;
}

HRESULT RegisterLanguageProfile(const wchar_t* modulePath)
{
    wchar_t profilePath[512] = {};
    HRESULT hr = BuildPath4(
        profilePath,
        ARRAYSIZE(profilePath),
        kTipRoot,
        kTextServiceClsidString,
        L"\\LanguageProfile\\",
        kJapaneseLangIdString);
    if (FAILED(hr)) {
        return hr;
    }

    hr = StringCchCatW(profilePath, ARRAYSIZE(profilePath), L"\\");
    if (SUCCEEDED(hr)) {
        hr = StringCchCatW(profilePath, ARRAYSIZE(profilePath), kProfileGuidString);
    }
    if (FAILED(hr)) {
        return hr;
    }

    HKEY profileKey = nullptr;
    hr = CreateKey(HKEY_LOCAL_MACHINE, profilePath, &profileKey);
    if (FAILED(hr)) {
        return hr;
    }

    hr = SetStringValue(profileKey, nullptr, kProfileDescription);
    if (SUCCEEDED(hr)) hr = SetStringValue(profileKey, L"Description", kProfileDescription);
    if (SUCCEEDED(hr)) hr = SetStringValue(profileKey, L"Display Description", kProfileDescription);
    if (SUCCEEDED(hr)) hr = SetStringValue(profileKey, L"IconFile", modulePath);
    if (SUCCEEDED(hr)) hr = SetDwordValue(profileKey, L"IconIndex", 0);
    if (SUCCEEDED(hr)) hr = SetDwordValue(profileKey, L"Enable", 1);

    RegCloseKey(profileKey);
    return hr;
}

HRESULT RegisterCategory(const wchar_t* categoryGuid)
{
    wchar_t categoryPath[512] = {};
    HRESULT hr = BuildPath4(
        categoryPath,
        ARRAYSIZE(categoryPath),
        kTipRoot,
        kTextServiceClsidString,
        L"\\Category\\",
        categoryGuid);
    if (FAILED(hr)) {
        return hr;
    }

    hr = StringCchCatW(categoryPath, ARRAYSIZE(categoryPath), L"\\");
    if (SUCCEEDED(hr)) {
        hr = StringCchCatW(categoryPath, ARRAYSIZE(categoryPath), kTextServiceClsidString);
    }
    if (FAILED(hr)) {
        return hr;
    }

    HKEY categoryKey = nullptr;
    hr = CreateKey(HKEY_LOCAL_MACHINE, categoryPath, &categoryKey);
    if (SUCCEEDED(hr)) {
        RegCloseKey(categoryKey);
    }
    return hr;
}

HRESULT RegisterInputProcessor(ITfInputProcessorProfiles* profiles)
{
    HRESULT hr = profiles->Register(CLSID_KeyroTextService);
    if (SUCCEEDED(hr)) {
        return S_OK;
    }

    profiles->Unregister(CLSID_KeyroTextService);
    return profiles->Register(CLSID_KeyroTextService);
}

HRESULT AddJapaneseLanguageProfile(ITfInputProcessorProfiles* profiles, const wchar_t* modulePath)
{
    ULONG descriptionChars = static_cast<ULONG>(wcslen(kProfileDescription));
    ULONG iconPathChars = static_cast<ULONG>(wcslen(modulePath));

    HRESULT hr = profiles->AddLanguageProfile(
        CLSID_KeyroTextService,
        kJapaneseLangId,
        GUID_KeyroProfile,
        kProfileDescription,
        descriptionChars,
        modulePath,
        iconPathChars,
        0);
    if (SUCCEEDED(hr)) {
        return S_OK;
    }

    profiles->RemoveLanguageProfile(CLSID_KeyroTextService, kJapaneseLangId, GUID_KeyroProfile);
    return profiles->AddLanguageProfile(
        CLSID_KeyroTextService,
        kJapaneseLangId,
        GUID_KeyroProfile,
        kProfileDescription,
        descriptionChars,
        modulePath,
        iconPathChars,
        0);
}

HRESULT EnableJapaneseLanguageProfile(ITfInputProcessorProfiles* profiles, BOOL enable)
{
    return profiles->EnableLanguageProfile(
        CLSID_KeyroTextService,
        kJapaneseLangId,
        GUID_KeyroProfile,
        enable);
}

HRESULT RegisterKeyboardCategory(ITfCategoryMgr* categoryMgr)
{
    HRESULT hr = categoryMgr->RegisterCategory(
        CLSID_KeyroTextService,
        GUID_TFCAT_TIP_KEYBOARD,
        CLSID_KeyroTextService);
    if (SUCCEEDED(hr)) {
        return S_OK;
    }

    categoryMgr->UnregisterCategory(
        CLSID_KeyroTextService,
        GUID_TFCAT_TIP_KEYBOARD,
        CLSID_KeyroTextService);
    return categoryMgr->RegisterCategory(
        CLSID_KeyroTextService,
        GUID_TFCAT_TIP_KEYBOARD,
        CLSID_KeyroTextService);
}

HRESULT RegisterTipWithTsfApi(const wchar_t* modulePath)
{
    ScopedComInitializer comInitializer;
    HRESULT hr = comInitializer.Result();
    if (FAILED(hr)) {
        return hr;
    }

    ITfInputProcessorProfiles* profiles = nullptr;
    hr = CoCreateInstance(
        CLSID_TF_InputProcessorProfiles,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfInputProcessorProfiles,
        reinterpret_cast<void**>(&profiles));
    if (FAILED(hr)) {
        return hr;
    }

    hr = RegisterInputProcessor(profiles);
    if (SUCCEEDED(hr)) {
        hr = AddJapaneseLanguageProfile(profiles, modulePath);
    }
    if (SUCCEEDED(hr)) {
        hr = EnableJapaneseLanguageProfile(profiles, TRUE);
    }
    SafeRelease(profiles);
    if (FAILED(hr)) {
        return hr;
    }

    ITfCategoryMgr* categoryMgr = nullptr;
    hr = CoCreateInstance(
        CLSID_TF_CategoryMgr,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfCategoryMgr,
        reinterpret_cast<void**>(&categoryMgr));
    if (FAILED(hr)) {
        return hr;
    }

    hr = RegisterKeyboardCategory(categoryMgr);
    SafeRelease(categoryMgr);
    return hr;
}

HRESULT RegisterTip()
{
    wchar_t modulePath[MAX_PATH] = {};
    HRESULT hr = GetModulePath(modulePath, ARRAYSIZE(modulePath));
    if (FAILED(hr)) {
        return hr;
    }

    hr = RegisterTipWithTsfApi(modulePath);
    if (FAILED(hr)) return hr;

    hr = RegisterTipRoot(modulePath);
    if (FAILED(hr)) return hr;

    hr = RegisterLanguageProfile(modulePath);
    if (FAILED(hr)) return hr;

    hr = RegisterCategory(kTipKeyboardCategoryString);
    if (FAILED(hr)) return hr;

    hr = RegisterCategory(kTipSecureModeCategoryString);
    if (FAILED(hr)) return hr;

    return RegisterCategory(kTipUiElementCategoryString);
}

HRESULT DeleteRegistryTree(HKEY root, const wchar_t* path)
{
    LSTATUS status = RegDeleteTreeW(root, path);
    if (status == ERROR_FILE_NOT_FOUND) {
        return S_OK;
    }
    return HResultFromWin32(status);
}

HRESULT UnregisterComServer()
{
    wchar_t clsidPath[256] = {};
    HRESULT hr = BuildPath(clsidPath, ARRAYSIZE(clsidPath), kComClsidRoot, kTextServiceClsidString);
    if (FAILED(hr)) {
        return hr;
    }
    return DeleteRegistryTree(HKEY_LOCAL_MACHINE, clsidPath);
}

HRESULT UnregisterTip()
{
    wchar_t tipPath[256] = {};
    HRESULT hr = BuildPath(tipPath, ARRAYSIZE(tipPath), kTipRoot, kTextServiceClsidString);
    if (FAILED(hr)) {
        return hr;
    }
    return DeleteRegistryTree(HKEY_LOCAL_MACHINE, tipPath);
}

HRESULT UnregisterTipWithTsfApi()
{
    ScopedComInitializer comInitializer;
    HRESULT hr = comInitializer.Result();
    if (FAILED(hr)) {
        return hr;
    }

    ITfCategoryMgr* categoryMgr = nullptr;
    hr = CoCreateInstance(
        CLSID_TF_CategoryMgr,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfCategoryMgr,
        reinterpret_cast<void**>(&categoryMgr));
    if (SUCCEEDED(hr)) {
        categoryMgr->UnregisterCategory(
            CLSID_KeyroTextService,
            GUID_TFCAT_TIP_KEYBOARD,
            CLSID_KeyroTextService);
        SafeRelease(categoryMgr);
    }

    ITfInputProcessorProfiles* profiles = nullptr;
    hr = CoCreateInstance(
        CLSID_TF_InputProcessorProfiles,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfInputProcessorProfiles,
        reinterpret_cast<void**>(&profiles));
    if (SUCCEEDED(hr)) {
        EnableJapaneseLanguageProfile(profiles, FALSE);
        profiles->RemoveLanguageProfile(CLSID_KeyroTextService, kJapaneseLangId, GUID_KeyroProfile);
        profiles->Unregister(CLSID_KeyroTextService);
        SafeRelease(profiles);
    }

    return S_OK;
}

} // namespace

void DllAddRef()
{
    InterlockedIncrement(&g_dllRefCount);
}

void DllRelease()
{
    InterlockedDecrement(&g_dllRefCount);
}

LONG DllRefCount()
{
    return g_dllRefCount;
}

void SetModuleHandle(HMODULE moduleHandle)
{
    g_moduleHandle = moduleHandle;
}

HMODULE DllModuleHandle()
{
    return g_moduleHandle;
}

} // namespace KeyroIME

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        KeyroIME::SetModuleHandle(hModule);
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}

STDAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    void** ppvObject)
{
    if (!ppvObject) {
        return E_POINTER;
    }

    *ppvObject = nullptr;

    if (!IsEqualCLSID(rclsid, KeyroIME::CLSID_KeyroTextService)) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    KeyroIME::KeyroClassFactory* factory = new (std::nothrow) KeyroIME::KeyroClassFactory();
    if (!factory) {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = factory->QueryInterface(riid, ppvObject);
    factory->Release();
    return hr;
}

STDAPI DllRegisterServer()
{
    HRESULT hr = KeyroIME::RegisterComServer();
    if (FAILED(hr)) {
        return hr;
    }
    return KeyroIME::RegisterTip();
}

STDAPI DllUnregisterServer()
{
    HRESULT apiHr = KeyroIME::UnregisterTipWithTsfApi();
    HRESULT tipHr = KeyroIME::UnregisterTip();
    HRESULT comHr = KeyroIME::UnregisterComServer();
    if (FAILED(apiHr)) return apiHr;
    return FAILED(tipHr) ? tipHr : comHr;
}

STDAPI DllCanUnloadNow()
{
    return KeyroIME::DllRefCount() == 0 ? S_OK : S_FALSE;
}
