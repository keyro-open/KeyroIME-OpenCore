// Copyright (C) 2025-2026 Localpro株式会社 (Localpro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) v1.0 OpenCore.
// KeyroIME is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software Foundation.
//
// For commercial use licensing, custom deployment, or proprietary integrations,
// please contact Localpro株式会社 via https://localpro.jp. Unauthorized closed-source
// commercial exploitation is strictly prohibited.
// class_factory.cpp

#include "class_factory.h"

#include "text_service.h"

#include <new>
#include <windows.h>

namespace KeyroIME {

KeyroClassFactory::KeyroClassFactory()
    : m_refCount(1)
{
    DllAddRef();
}

KeyroClassFactory::~KeyroClassFactory()
{
    DllRelease();
}

HRESULT STDMETHODCALLTYPE KeyroClassFactory::QueryInterface(REFIID riid, void** ppvObject)
{
    if (!ppvObject) {
        return E_POINTER;
    }

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
        *ppvObject = static_cast<IClassFactory*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE KeyroClassFactory::AddRef()
{
    return static_cast<ULONG>(InterlockedIncrement(&m_refCount));
}

ULONG STDMETHODCALLTYPE KeyroClassFactory::Release()
{
    ULONG refCount = static_cast<ULONG>(InterlockedDecrement(&m_refCount));
    if (refCount == 0) {
        delete this;
    }
    return refCount;
}

HRESULT STDMETHODCALLTYPE KeyroClassFactory::CreateInstance(
    IUnknown* pUnkOuter,
    REFIID riid,
    void** ppvObject)
{
    if (!ppvObject) {
        return E_POINTER;
    }

    *ppvObject = nullptr;

    if (pUnkOuter) {
        return CLASS_E_NOAGGREGATION;
    }

    KeyroTextService* service = new (std::nothrow) KeyroTextService();
    if (!service) {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = service->QueryInterface(riid, ppvObject);
    service->Release();
    return hr;
}

HRESULT STDMETHODCALLTYPE KeyroClassFactory::LockServer(BOOL fLock)
{
    if (fLock) {
        DllAddRef();
    } else {
        DllRelease();
    }
    return S_OK;
}

} // namespace KeyroIME
