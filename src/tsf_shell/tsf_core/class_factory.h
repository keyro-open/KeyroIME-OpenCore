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
