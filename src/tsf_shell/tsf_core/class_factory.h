// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// GNU GPLv3に基づいて配布されます。LICENSE（英語正文）を参照してください。
// class_factory.h
// IClassFactory implementation for KeyroIME.dll.

#pragma once

#include <unknwn.h>

namespace KeyroIME {

class KeyroClassFactory final : public IClassFactory {
public:
    KeyroClassFactory();
    ~KeyroClassFactory();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE CreateInstance(
        IUnknown* pUnkOuter,
        REFIID riid,
        void** ppvObject) override;

    HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock) override;

private:
    volatile LONG m_refCount;
};

} // namespace KeyroIME
