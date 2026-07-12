// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) v1.0 OpenCore.
// KeyroIME is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software Foundation.
//
// For commercial use licensing, custom deployment, or proprietary integrations,
// please contact 株式会社LocalPro via https://localpro.jp. Unauthorized closed-source
// commercial exploitation is strictly prohibited.
#include <msctf.h>
#include <windows.h>

#include <iostream>
#include <string>

#include "tsf_core/module_path.h"
#include "tsf_core/tip_guid.h"

using DllGetClassObjectFn = HRESULT(STDAPICALLTYPE*)(REFCLSID, REFIID, void**);

int wmain(int argc, wchar_t** argv)
{
    HRESULT comHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool uninitialize = comHr == S_OK || comHr == S_FALSE;
    if (FAILED(comHr) && comHr != RPC_E_CHANGED_MODE) {
        return 1;
    }

    bool registeredMode = argc > 1 && wcscmp(argv[1], L"--registered") == 0;
    std::wstring dllPath;
    if (!registeredMode && argc > 1 && argv[1] && argv[1][0] != L'\0') {
        dllPath = argv[1];
    } else if (!registeredMode) {
        if (!KeyroIME::BuildModuleSiblingPath(
                GetModuleHandleW(nullptr),
                L"KeyroIME.dll",
                dllPath)) {
            if (uninitialize) CoUninitialize();
            return 2;
        }
    }

    HMODULE module = nullptr;
    ITfTextInputProcessor* processor = nullptr;
    HRESULT hr = E_FAIL;
    if (registeredMode) {
        hr = CoCreateInstance(
            KeyroIME::CLSID_KeyroTextService,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_ITfTextInputProcessor,
            reinterpret_cast<void**>(&processor));
    } else {
        module = LoadLibraryW(dllPath.c_str());
        if (!module) {
            if (uninitialize) CoUninitialize();
            return 3;
        }

        auto dllGetClassObject = reinterpret_cast<DllGetClassObjectFn>(
            GetProcAddress(module, "DllGetClassObject"));
        if (!dllGetClassObject) {
            FreeLibrary(module);
            if (uninitialize) CoUninitialize();
            return 4;
        }

        IClassFactory* factory = nullptr;
        hr = dllGetClassObject(
            KeyroIME::CLSID_KeyroTextService,
            IID_IClassFactory,
            reinterpret_cast<void**>(&factory));
        if (FAILED(hr) || !factory) {
            FreeLibrary(module);
            if (uninitialize) CoUninitialize();
            return 5;
        }
        hr = factory->CreateInstance(
            nullptr,
            IID_ITfTextInputProcessor,
            reinterpret_cast<void**>(&processor));
        factory->Release();
    }
    if (FAILED(hr) || !processor) {
        if (module) FreeLibrary(module);
        if (uninitialize) CoUninitialize();
        return 6;
    }

    ITfThreadMgr* threadMgr = nullptr;
    hr = CoCreateInstance(
        CLSID_TF_ThreadMgr,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfThreadMgr,
        reinterpret_cast<void**>(&threadMgr));
    if (FAILED(hr) || !threadMgr) {
        processor->Release();
        if (module) FreeLibrary(module);
        if (uninitialize) CoUninitialize();
        return 7;
    }

    TfClientId clientId = TF_CLIENTID_NULL;
    hr = threadMgr->Activate(&clientId);
    if (SUCCEEDED(hr)) {
        hr = processor->Activate(threadMgr, clientId);
        processor->Deactivate();
        threadMgr->Deactivate();
    }

    threadMgr->Release();
    processor->Release();
    if (module) FreeLibrary(module);
    if (uninitialize) CoUninitialize();

    if (FAILED(hr)) {
        std::wcerr << L"TSF activation smoke test failed: 0x"
                   << std::hex << static_cast<unsigned long>(hr) << std::endl;
        return 8;
    }

    std::wcout << L"TSF activation smoke test passed." << std::endl;
    return 0;
}
