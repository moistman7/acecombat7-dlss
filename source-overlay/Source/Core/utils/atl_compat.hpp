#pragma once
// Local build compatibility: the installed C++ toolchain has WRL but no ATL.
#include <Windows.h>
#include <wrl/client.h>
#include <string>
struct CAtlException { HRESULT m_hr; };
template<class T> class CComPtr : public Microsoft::WRL::ComPtr<T> {
public:
    using Microsoft::WRL::ComPtr<T>::ComPtr;
    operator T*() const noexcept { return this->Get(); }
    void Release() noexcept { this->Reset(); }
    template<class U> HRESULT QueryInterface(U** result) const {
        return this->Get()->QueryInterface(__uuidof(U), reinterpret_cast<void**>(result));
    }
};
class CA2W {
    std::wstring value;
public:
    CA2W(const char* text, UINT cp) {
        if (!text) return;
        int count = MultiByteToWideChar(cp, 0, text, -1, nullptr, 0);
        if (count == 0) throw CAtlException{HRESULT_FROM_WIN32(GetLastError())};
        if (count > 0) { value.resize(count); MultiByteToWideChar(cp, 0, text, -1, value.data(), count); }
    }
    operator const wchar_t*() const { return value.c_str(); }
};
