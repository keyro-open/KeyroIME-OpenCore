// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// It is source-available under the KeyroIME OpenCore Non-Commercial Source
// License 1.0. See LICENSE. Commercial use requires a separate written license
// from 株式会社LocalPro.
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
