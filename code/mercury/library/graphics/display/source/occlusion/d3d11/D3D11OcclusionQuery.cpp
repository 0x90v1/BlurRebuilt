// [Blur1PCDX11] Hardware pixel-count queries through ID3D11DeviceContext.
#include "StdAfx.h"
#include "Display/Occlusion/D3D11/D3D11OcclusionQuery.h"
#include "Display/Device/Device.h"

namespace Display { namespace D3D11 {
namespace
{
ID3D11DeviceContext* ImmediateContext()
{
    ID3D11DeviceContext* context = nullptr;
    ID3D11Device* device = Display::Device()->GetD3DDevice();
    if (device) device->GetImmediateContext(&context);
    return context;
}
}

C_OcclusionQuery::C_OcclusionQuery() : m_Query(nullptr) {}
C_OcclusionQuery::~C_OcclusionQuery() { Destroy(); }

bool C_OcclusionQuery::Create()
{
    if (m_Query) return true;
    ID3D11Device* device = Display::Device()->GetD3DDevice();
    if (!device) return false;
    const D3D11_QUERY_DESC desc = { D3D11_QUERY_OCCLUSION, 0 };
    return SUCCEEDED(device->CreateQuery(&desc, &m_Query));
}

void C_OcclusionQuery::Destroy()
{
    if (m_Query) m_Query->Release();
    m_Query = nullptr;
}

void C_OcclusionQuery::Begin()
{
    if (!m_Query) return;
    ID3D11DeviceContext* context = ImmediateContext();
    if (context) { context->Begin(m_Query); context->Release(); }
}

void C_OcclusionQuery::End()
{
    if (!m_Query) return;
    ID3D11DeviceContext* context = ImmediateContext();
    if (context) { context->End(m_Query); context->Release(); }
}

E_OcclusionQueryResult C_OcclusionQuery::Visible(U32& count) const
{
    count = 0;
    if (!m_Query) return E_OcclusionQueryResult_NotVisible;
    ID3D11DeviceContext* context = ImmediateContext();
    if (!context) return E_OcclusionQueryResult_Failed;
    UINT64 samples = 0;
    const HRESULT result = context->GetData(m_Query, &samples, sizeof(samples),
                                            D3D11_ASYNC_GETDATA_DONOTFLUSH);
    context->Release();
    if (result != S_OK) return E_OcclusionQueryResult_Failed;
    count = samples > UINT_MAX ? UINT_MAX : static_cast<U32>(samples);
    return samples ? E_OcclusionQueryResult_Visible : E_OcclusionQueryResult_NotVisible;
}

E_OcclusionQueryResult C_OcclusionQuery::Visible() const
{
    U32 count = 0;
    return Visible(count);
}

E_OcclusionQueryResult C_OcclusionQuery::Visible(F32& fraction) const
{
    U32 count = 0;
    const E_OcclusionQueryResult result = Visible(count);
    fraction = result == E_OcclusionQueryResult_Visible ? 1.0f : 0.0f;
    return result;
}
} }
