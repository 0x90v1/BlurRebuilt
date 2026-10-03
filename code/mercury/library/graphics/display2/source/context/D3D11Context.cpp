// [Blur1PCDX11] Native Display2 resource objects and context state.
#include "StdAfx.h"
#include "Display2/Context/D3D11Context.h"
#include "Display2/Device/Device.h"
#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#include <map>
#include <mutex>
#include <vector>
#include <system/debug/Blur2PCAutomation.h>

namespace
{
void DumpNativeInfoQueue(ID3D11Device* device, const char* where)
{
	if (!device) return;
	ID3D11InfoQueue* queue = nullptr;
	if (FAILED(device->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<void**>(&queue))) || !queue)
		return;
	const UINT64 count = queue->GetNumStoredMessages();
	for (UINT64 index = 0; index < count; ++index)
	{
		SIZE_T size = 0;
		queue->GetMessage(index, nullptr, &size);
		std::vector<U8> bytes(size);
		D3D11_MESSAGE* message = reinterpret_cast<D3D11_MESSAGE*>(bytes.data());
		if (SUCCEEDED(queue->GetMessage(index, message, &size)))
			Blur2PCAutomation::LogF("DX11 debug %s sev=%u: %s", where,
				static_cast<unsigned>(message->Severity), message->pDescription);
	}
	queue->Release();
}

// [Blur1PCDX11] D3D11 default resources cannot be mapped. Keep a staging copy
// for the duration of each legacy resource lock and copy writes back on unlock.
struct S_StagingLock
{
	ID3D11Resource* staging;
	ID3D11DeviceContext* context;
	UINT subresource;
	bool write;
	D3DFORMAT pcFormat;
	UINT width, height, depth;
	D3D11_MAPPED_SUBRESOURCE nativeMap;
	std::vector<U8> pcBytes;
};
using S_StagingKey = std::pair<const ID3D11Resource*, UINT>;
std::map<S_StagingKey, S_StagingLock> g_stagingLocks;
std::mutex g_stagingMutex;

void TraceLockFailure(const char* reason, ID3D11Resource* resource, UINT subresource, HRESULT result)
{
	static unsigned count = 0;
	if (count++ >= 16) return;
	void* frames[8] = {};
	const USHORT captured = CaptureStackBackTrace(1,8,frames,nullptr);
	Blur2PCAutomation::LogF("DX11 resource lock failed %s resource=%p sub=%u hr=%08x",
		reason,resource,subresource,static_cast<unsigned>(result));
	for (USHORT i=0; i<captured; ++i) Blur2PCAutomation::LogF("DX11 lock caller[%u]=%p",i,frames[i]);
}

// [Blur1PCDX11] The D3D11 debug layer raises 0x87D on the corruption it detects;
// catch it so the message reaches the log instead of the crash handler.
HRESULT TryMapStaging(ID3D11DeviceContext* context, ID3D11Resource* staging,
                      D3D11_MAPPED_SUBRESOURCE& mapped,
                      D3D11_MAP mode = D3D11_MAP_READ_WRITE)
{
	__try
	{
		return context->Map(staging, 0, mode, 0, &mapped);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
}

bool LockResource(ID3D11Resource* resource, UINT subresource, bool write,
                 D3D11_MAPPED_SUBRESOURCE& mapped, U32 flags = 0)
{
	if (!resource) { TraceLockFailure("null resource",resource,subresource,E_POINTER); return false; }
	std::lock_guard<std::mutex> lock(g_stagingMutex);
	const S_StagingKey key(resource, subresource);
	if (g_stagingLocks.count(key)) { TraceLockFailure("already locked",resource,subresource,E_FAIL); return false; }
	ID3D11Device* device = nullptr;
	resource->GetDevice(&device);
	if (!device) return false;
	ID3D11DeviceContext* context = nullptr;
	device->GetImmediateContext(&context);
	D3D11_RESOURCE_DIMENSION dimension;
	resource->GetType(&dimension);
	ID3D11Resource* staging = nullptr;
	HRESULT hr = E_FAIL;
	bool directWrite = false;
	D3D11_MAP mode = D3D11_MAP_READ_WRITE;
	UINT width = 0, height = 1, depth = 1;
	if (dimension == D3D11_RESOURCE_DIMENSION_BUFFER)
	{
		D3D11_BUFFER_DESC desc;
		static_cast<ID3D11Buffer*>(resource)->GetDesc(&desc);
		// [Blur1PCDX11] Retail DynamicVertexBuffer discards at offset zero and
		// appends with NOOVERWRITE. Honor those promises without a GPU readback.
		if (write && desc.Usage == D3D11_USAGE_DYNAMIC &&
			(flags & (D3DLOCK_DISCARD | D3DLOCK_NOOVERWRITE)))
		{
			directWrite = true;
			mode = flags & D3DLOCK_DISCARD ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
			staging = resource;
			staging->AddRef();
			hr = S_OK;
		}
		else
		{
			desc.Usage = D3D11_USAGE_STAGING;
			desc.BindFlags = 0;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			ID3D11Buffer* buffer = nullptr;
			hr = device->CreateBuffer(&desc, nullptr, &buffer);
			staging = buffer;
		}
	}
	else if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
	{
		D3D11_TEXTURE2D_DESC desc;
		static_cast<ID3D11Texture2D*>(resource)->GetDesc(&desc);
		const UINT mip = subresource % desc.MipLevels;
		desc.Width = std::max(1U, desc.Width >> mip);
		desc.Height = std::max(1U, desc.Height >> mip);
		width = desc.Width; height = desc.Height;
		const Display2::E_TextureFormat pcFormat = Display2::D3D11PCFormat(desc.Format);
		if (pcFormat == Display2::E_TextureFormat_DXT1 || pcFormat == Display2::E_TextureFormat_DXT3 ||
			pcFormat == Display2::E_TextureFormat_DXT5)
		{
			// [Blur1PCDX11] A standalone staging mip needs whole BC blocks.
			desc.Width = (desc.Width + 3) & ~3U;
			desc.Height = (desc.Height + 3) & ~3U;
		}
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		if (desc.SampleDesc.Count > 1)
		{
			// Multisampled surfaces are read through a single-sample resolve.
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
		}
		ID3D11Texture2D* texture = nullptr;
		hr = device->CreateTexture2D(&desc, nullptr, &texture);
		staging = texture;
	}
	else if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
	{
		D3D11_TEXTURE3D_DESC desc;
		static_cast<ID3D11Texture3D*>(resource)->GetDesc(&desc);
		desc.Width = std::max(1U, desc.Width >> subresource);
		desc.Height = std::max(1U, desc.Height >> subresource);
		desc.Depth = std::max(1U, desc.Depth >> subresource);
		width = desc.Width; height = desc.Height; depth = desc.Depth;
		desc.MipLevels = 1;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		ID3D11Texture3D* texture = nullptr;
		hr = device->CreateTexture3D(&desc, nullptr, &texture);
		staging = texture;
	}
	if (SUCCEEDED(hr) && staging)
	{
		if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
		{
			D3D11_TEXTURE2D_DESC desc;
			static_cast<ID3D11Texture2D*>(resource)->GetDesc(&desc);
			if (desc.SampleDesc.Count > 1)
				context->ResolveSubresource(staging, 0, resource, subresource, desc.Format);
			else
				context->CopySubresourceRegion(staging, 0, 0, 0, 0, resource, subresource, nullptr);
		}
		else if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
			context->CopySubresourceRegion(staging, 0, 0, 0, 0, resource, subresource, nullptr);
		else if (!directWrite)
			context->CopyResource(staging, resource);
		hr = TryMapStaging(context, staging, mapped, mode);
		if (SUCCEEDED(hr))
		{
			S_StagingLock& held = g_stagingLocks[key];
			held = {};
			held.staging = staging; held.context = context; held.subresource = subresource;
			// [Blur1PCDX11] A directly mapped buffer is already the GPU resource;
			// unlock unmaps it and must not copy it onto itself.
			held.write = write && !directWrite;
			held.pcFormat = Display2::D3D11TextureFormats::GetPCFormat(resource);
			held.width = width; held.height = height; held.depth = depth; held.nativeMap = mapped;
			DXGI_FORMAT storage;
			unsigned pcBytes, nativeBytes;
			if (Display2::D3D11TextureFormats::LuminanceLayout(held.pcFormat,&storage,&pcBytes,&nativeBytes))
			{
				const UINT row = width*pcBytes, slice = row*height;
				held.pcBytes.resize(static_cast<size_t>(slice)*depth);
				Display2::D3D11TextureFormats::ConvertLuminance(held.pcFormat,width,height,depth,
					mapped.pData,mapped.RowPitch,mapped.DepthPitch,held.pcBytes.data(),row,slice,false);
				mapped.pData = held.pcBytes.data(); mapped.RowPitch = row; mapped.DepthPitch = slice;
			}
		}
		else
		{
			TraceLockFailure("staging map",resource,subresource,hr);
			static unsigned traced = 0;
			if (traced++ < 3)
			{
				Blur2PCAutomation::LogF("DX11 staging map failed hr=%08x resource=%p sub=%u write=%u",
					static_cast<unsigned>(hr), resource, subresource, write ? 1u : 0u);
				DumpNativeInfoQueue(device, "staging map");
			}
			staging->Release();
		}
	}
	else TraceLockFailure("staging allocation",resource,subresource,hr);
	context->Release();
	device->Release();
	return SUCCEEDED(hr);
}

void UnlockStagingResource(ID3D11Resource* resource, UINT subresource)
{
	std::lock_guard<std::mutex> guard(g_stagingMutex);
	auto it = g_stagingLocks.find(S_StagingKey(resource, subresource));
	if (it == g_stagingLocks.end()) return;
	const S_StagingLock& lock = it->second;
	if (lock.write && !lock.pcBytes.empty())
	{
		const UINT row = static_cast<UINT>(lock.pcBytes.size())/lock.height/lock.depth;
		Display2::D3D11TextureFormats::ConvertLuminance(lock.pcFormat,lock.width,lock.height,lock.depth,
			lock.pcBytes.data(),row,row*lock.height,lock.nativeMap.pData,
			lock.nativeMap.RowPitch,lock.nativeMap.DepthPitch,true);
	}
	lock.context->Unmap(lock.staging, 0);
	if (lock.write)
	{
		D3D11_RESOURCE_DIMENSION dimension;
		resource->GetType(&dimension);
		if (dimension == D3D11_RESOURCE_DIMENSION_BUFFER)
			lock.context->CopyResource(resource, lock.staging);
		else
			lock.context->CopySubresourceRegion(resource, lock.subresource, 0, 0, 0,
			                                   lock.staging, 0, nullptr);
	}
	lock.staging->Release();
	g_stagingLocks.erase(it);
}
}

namespace Display2
{
E_SurfaceFormat GetSurfaceFormatFromTexture(const E_TextureFormat format)
{
	return (E_SurfaceFormat)format;
}

U32 GetSurfacePitchFromWidth(U32 width, const U32 flags, const E_SurfaceFormat format, const E_Multisample multisample)
{
	U32 pitch = 0;

	switch(format)
	{
	case E_SurfaceFormat_L8:
	case E_SurfaceFormat_A8:
		pitch = width * 1;
		break;
	case E_SurfaceFormat_D16:
		pitch = width * 2;
		break;
	case E_SurfaceFormat_A8R8G8B8:
	case E_SurfaceFormat_D24S8:
	case E_SurfaceFormat_R32F:
		pitch = width * 4;
		break;
	case E_SurfaceFormat_A16B16G16R16F:
		pitch = width * 8;
		break;
	case E_SurfaceFormat_A32B32G32R32F:
		pitch = width * 16;
		break;
	default:
		assert(0 && "Unsupported surface format");
		break;
	}

	switch(multisample)
	{
	case E_Multisample_None:
		break;
	case E_Multisample_2:
	case E_Multisample_4:
		pitch = pitch * 2;
		break;
	default:
		assert(0 && "Unsupported surface multisample");
		break;
	}

	return pitch;
}

U32 GetSurfaceSizeFromHeightAndPitch(U32 height, const U32 pitch, const U32 flags, const E_SurfaceFormat format, const E_Multisample multisample)
{
	switch(multisample)
	{
	case E_Multisample_None:
	case E_Multisample_2:
		break;
	case E_Multisample_4:
		height = height * 2;
		break;
	default:
		assert(0 && "Unsupported surface multisample");
		break;
	}

	U32 size = pitch * height;

	return size;
}

U32 GetSurfaceSize(const U32 width, const U32 height, const U32 flags, const E_SurfaceFormat format, const E_Multisample multisample)
{
#if PLATFORM_XENON
	if(!(flags & E_Flags_Raw))
		return XGSurfaceSize(width, height, (D3DFORMAT)format, (D3DMULTISAMPLE_TYPE)multisample);
#endif

	const U32 pitch = GetSurfacePitchFromWidth(width, flags, format, multisample);
	return GetSurfaceSizeFromHeightAndPitch(height, pitch, flags, format, multisample);
}

void GetSurfaceSizeAndPitch(const U32 width, const U32 height, const U32 flags, const E_SurfaceFormat format, const E_Multisample multisample, U32& size, U32& pitch)
{
	pitch = GetSurfacePitchFromWidth(width, flags, format, multisample);
	size = GetSurfaceSize(width, height, flags, format, multisample);
}

U32 GetTexturePitchFromWidth(U32 width, const U32 flags, const E_TextureFormat format)
{
	U32 pitch = 0;

	switch(format)
	{
	case E_TextureFormat_DXT1:
		pitch = ((width + 3) / 4) * 8;
		break;
	case E_TextureFormat_DXT3:
	case E_TextureFormat_DXT5:
		pitch = ((width + 3) / 4) * 16;	
		break;
	case E_TextureFormat_L8:
	case E_TextureFormat_A8:
		pitch = width * 1;
		break;
	case E_TextureFormat_D16:
		pitch = width * 2;
		break;
	case E_TextureFormat_A8R8G8B8:
	case E_TextureFormat_X8R8G8B8:
	case E_TextureFormat_D24S8:
	case E_TextureFormat_R32F:
		pitch = width * 4;
		break;
	case E_TextureFormat_A16B16G16R16F:
		return width * 8;
	case E_TextureFormat_A32B32G32R32F:
		return width * 16;
	default:
		assert(0 && "Unsupported texture format");
		break;
	}

	return pitch;
}

U32 GetTextureSizeFromHeightAndPitch(U32 height, const U32 pitch, const U32 flags, const E_TextureFormat format)
{
	switch(format)
	{
	case E_TextureFormat_DXT1:
	case E_TextureFormat_DXT3:
	case E_TextureFormat_DXT5:
		height = ((height + 3) / 4);
		break;
	default:
		break;
	}

	U32 size = pitch * height;

	return size;
}


U32 GetTexturePitch(U32 width, const U32 flags, const E_TextureFormat format)
{
	return GetTexturePitchFromWidth(width, flags, format);
}

U32 GetTextureRowPitch(U32 width, const U32 flags, const E_TextureFormat format)
{
	return GetTexturePitchFromWidth(width, flags, format);
}

U32 GetTextureSlicePitch(U32 width, U32 height, U32 levels, const U32 flags, const E_TextureFormat format)
{
	U32 pitch = GetTexturePitchFromWidth(width, flags, format);
	U32 size = GetTextureSizeFromHeightAndPitch(height, pitch, flags, format);
	return size * levels;
}

U32 GetTexture2DSize(const U32 width, const U32 height, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	if(!(flags & E_Flags_Raw))
		return XGSetTextureHeader(width, height, 1, 0, (D3DFORMAT)format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, NULL, NULL, NULL);
#endif

	const U32 pitch = GetTexturePitchFromWidth(width, flags, format);
	return GetTextureSizeFromHeightAndPitch(height, pitch, flags, format);
}

U32 GetTexture2DSize(U32 width, U32 height, const U32 levels, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	if(!(flags & E_Flags_Raw))
		return XGSetTextureHeader(width, height, levels, 0, (D3DFORMAT)format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, NULL, NULL, NULL);
#endif

	U32 size = 0;
	for(U32 level=0; level<levels; level++)
	{
		size += GetTexture2DSize(width, height, flags, format);
		if(width > 1)
			width >>= 1;
		if(height > 1)
			height >>= 1;
	}
	return size;
}

void GetTexture2DSizeAndPitch(const U32 width, const U32 height, const U32 levels, const U32 flags, const E_TextureFormat format, U32& size, U32& pitch)
{
	pitch = GetTexturePitchFromWidth(width, flags, format);
	size = GetTexture2DSize(width, height, levels, flags, format);
}

U32 GetTexture3DSize(const U32 width, const U32 height, const U32 depth, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	if(!(flags & E_Flags_Raw))
		return XGSetVolumeTextureHeader(width, height, depth, 1, 0, (D3DFORMAT)format, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, NULL, NULL, NULL);
#endif

	return GetTexture2DSize(width, height, flags, format) * depth;
}

U32 GetTexture3DSize(U32 width, U32 height, const U32 depth, const U32 levels, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	if(!(flags & E_Flags_Raw))
		return XGSetVolumeTextureHeader(width, height, depth, 1, 0, (D3DFORMAT)format, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, NULL, NULL, NULL);
#endif

	U32 size = 0;
	for(U32 level = 0; level < levels; level++)
	{
		size += GetTexture3DSize(width, height, depth, flags, format);
		if(width > 1)
			width >>= 1;
		if(height > 1)
			height >>= 1;
	}
	return size;
}

void GetTexture3DSizeAndPitch(const U32 width, const U32 height, const U32 depth, const U32 levels, const U32 flags, const E_TextureFormat format, U32& size, U32& pitch)
{
	pitch = GetTexturePitchFromWidth(width, flags, format);
	size = GetTexture3DSize(width, height, depth, levels, flags, format);
}

U32 GetTextureCubeSize(const U32 width, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	if(!(flags & E_Flags_Raw))
		return XGSetCubeTextureHeader(width, 1, 0, (D3DFORMAT)format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, NULL, NULL, NULL);
#endif

	U32 size = 0;
	for(U32 i=0; i<6; i++)
	{
		size += GetTexture2DSize(width, width, flags, format);
	}
	return size;
}

U32 GetTextureCubeSize(U32 width, const U32 levels, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	if(!(flags & E_Flags_Raw))
		return XGSetCubeTextureHeader(width, levels, 0, (D3DFORMAT)format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, NULL, NULL, NULL);
#endif

	U32 size = 0;
	for(U32 i=0; i<6; i++)
	{
		size += GetTexture2DSize(width, width, levels, flags, format);
	}
	return size;
}

void GetTextureCubeSizeAndPitch(const U32 width, const U32 levels, const U32 flags, const E_TextureFormat format, U32& size, U32& pitch)
{
	pitch = GetTexturePitchFromWidth(width, flags, format);
	size = GetTextureCubeSize(width, levels, flags, format);
}

U32 GetTexture2DLevelOffset(const U32 level, U32 width, U32 height, const U32 levels, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	BIZ_ASSERT_FAIL("TODO: Implement me!");
#endif

	U32 offset = 0;

	for(U32 i=0; i<level; i++)
	{
		if(i >= levels)
			return offset;

		const U32 size = GetTexture2DSize(width, height, flags, format);

		offset += size;

		if(width > 1)
			width /= 2;
		if(height > 1)
			height /= 2;
	}

	return offset;
}

U32 GetTexture3DLevelOffset(const U32 level, U32 width, U32 height, U32 depth, const U32 levels, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	BIZ_ASSERT_FAIL("TODO: Implement me!");
#endif

	U32 offset = 0;

	for(U32 i=0; i<level; i++)
	{
		if(i >= levels)
			return offset;

		const U32 size = GetTexture3DSize(width, height, depth, flags, format);

		offset += size;

		if(width > 1)
			width /= 2;
		if(height > 1)
			height /= 2;
		if(depth > 1)
			depth /= 2;
	}

	return offset;
}

U32 GetTextureCubeLevelOffset(const E_CubeMapFace face, const U32 level, U32 width, const U32 levels, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	BIZ_ASSERT_FAIL("TODO: Implement me!");
#endif

	U32 offset = 0;

	for(S32 f=0; f<face; f++)
	{
		const U32 size = GetTexture2DSize(width, width, levels, flags, format);

		offset += size;
	}

	for(U32 l=0; l<level; l++)
	{
		if(l >= levels)
			return offset;

		const U32 size = GetTextureCubeSize(width, flags, format);

		offset += size;

		if(width > 1)
			width /= 2;
	}

	return offset;
}

#if PLATFORM_XENON

U32 SetTexture2DHeader(const U32 width, const U32 height, const U32 levels, const U32 flags, const E_TextureFormat format, const U32 pitch, GPUTexture2D* pTexture)
{
	BIZ_ASSERT(pTexture);
	
	DWORD d3dUsage = 0;
	if(!(flags & E_Flags_Dynamic))
		d3dUsage = D3DUSAGE_CPU_CACHED_MEMORY;

	#if !CFG_SPEED
	d3dUsage |= D3DUSAGE_RUNCOMMANDBUFFER_TIMESTAMP;
	#endif
	
	D3DFORMAT d3dFormat = (D3DFORMAT)format;
	if(flags & E_Flags_Linear)
		d3dFormat = (D3DFORMAT)MAKELINFMT(d3dFormat);
	
	U32 size = XGSetTextureHeader(width, height, levels, d3dUsage, d3dFormat, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, pTexture, NULL, NULL);

	U32 intRefCount = GPUCOMMON_DEBUG_INTREFCOUNT(pTexture->Common);
	BIZ_ASSERT_MSG_SPEED(intRefCount == 0, "intRefCount %i", intRefCount);

	return size;
}

U32 SetTexture3DHeader(const U32 width, const U32 height, const U32 depth,const U32 levels, const U32 flags, const E_TextureFormat format, const U32 pitch, GPUTexture3D* pTexture)
{
	BIZ_ASSERT(pTexture);

	DWORD d3dUsage = 0;
	if(!(flags & E_Flags_Dynamic))
		d3dUsage = D3DUSAGE_CPU_CACHED_MEMORY;

	#if !CFG_SPEED
	d3dUsage |= D3DUSAGE_RUNCOMMANDBUFFER_TIMESTAMP;
	#endif

	D3DFORMAT d3dFormat = (D3DFORMAT)format;
	if(flags & E_Flags_Linear)
		d3dFormat = (D3DFORMAT)MAKELINFMT(d3dFormat);
	
	return XGSetVolumeTextureHeader(width, height, depth, levels, d3dUsage, d3dFormat, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, pTexture, NULL, NULL);
}

U32 SetTextureCubeHeader(const U32 width, const U32 levels, const U32 flags, const E_TextureFormat format, const U32 pitch, GPUTextureCube* pTexture)
{
	BIZ_ASSERT(pTexture);

	DWORD d3dUsage = 0;
	if(!(flags & E_Flags_Dynamic))
		d3dUsage = D3DUSAGE_CPU_CACHED_MEMORY;

	#if !CFG_SPEED
	d3dUsage |= D3DUSAGE_RUNCOMMANDBUFFER_TIMESTAMP;
	#endif

	D3DFORMAT d3dFormat = (D3DFORMAT)format;
	if(flags & E_Flags_Linear)
		d3dFormat = (D3DFORMAT)MAKELINFMT(d3dFormat);

	return XGSetCubeTextureHeader(width, levels, d3dUsage, d3dFormat, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, pTexture, NULL, NULL);
}

#endif

bool DebugCheckGPUTexture(const GPUTexture* pgpuTexture)
{
	if(!pgpuTexture)
		return true;

#if DEVICE_XB2
	D3DBaseTexture* pBaseTexture = (D3DBaseTexture*)pgpuTexture;
	if(pBaseTexture)
	{
		BIZ_ASSERT((pBaseTexture->Format.BaseAddress << GPU_TEXTURE_ADDRESS_SHIFT) != 0);
		BIZ_ASSERT(pBaseTexture->GetLevelCount() > 0);
		
		if(pBaseTexture->GetType() == D3DRTYPE_TEXTURE)
		{
			D3DTexture* pTexture = (D3DTexture*)pBaseTexture;

			D3DSURFACE_DESC surfaceDesc;
			pTexture->GetLevelDesc(0, &surfaceDesc);
			
			BIZ_ASSERT(surfaceDesc.Type == D3DRTYPE_TEXTURE);
			BIZ_ASSERT(surfaceDesc.Width > 0);
			BIZ_ASSERT(surfaceDesc.Height > 0);
		}
		else
		if(pBaseTexture->GetType() == D3DRTYPE_VOLUMETEXTURE)
		{
			D3DVolumeTexture* pVolumeTexture = (D3DVolumeTexture*)pBaseTexture;
			
			D3DVOLUME_DESC volumeDesc;
			pVolumeTexture->GetLevelDesc(0, &volumeDesc);

			BIZ_ASSERT(volumeDesc.Type == D3DRTYPE_VOLUMETEXTURE);
			BIZ_ASSERT(volumeDesc.Width > 0);
			BIZ_ASSERT(volumeDesc.Height > 0);
			BIZ_ASSERT(volumeDesc.Depth > 0);
		}
		else
		if(pBaseTexture->GetType() == D3DRTYPE_CUBETEXTURE)
		{
			D3DCubeTexture* pCubeTexture = (D3DCubeTexture*)pBaseTexture;
			
			D3DSURFACE_DESC surfaceDesc;
			pCubeTexture->GetLevelDesc(0, &surfaceDesc);
			BIZ_ASSERT(surfaceDesc.Type == D3DRTYPE_CUBETEXTURE);
			BIZ_ASSERT(surfaceDesc.Width > 0);
		}
		else
		{
			BIZ_ASSERT_FAIL("Unsupported d3d resource type %i", pBaseTexture->GetType());
		}
	}
#endif
	
	return true;
}

bool DebugCheckTexture(const C_Texture* pTexture)
{
	if(!pTexture)
		return true;
	if(!pTexture->IsCreated())
		return true;
	return DebugCheckGPUTexture((GPUTexture*)pTexture->GetGPUTexture());
}

const Util::C_String GetD3DFormatAsString(const D3DFORMAT format)
{
	switch(format)
	{
	case D3DFMT_A8R8G8B8:
		return "A8R8G8B8";
	case D3DFMT_X8R8G8B8:
		return "X8R8G8B8";
	case D3DFMT_D24S8:
		return "D24S8";
	case D3DFMT_D24FS8:
		return "D24FS8";
	default:
		return "UNKNOWN";
	}
}

const Util::C_String GetD3DMultisampleTypeAsString(const D3DMULTISAMPLE_TYPE multisampleType)
{
	switch(multisampleType)
	{
	case D3DMULTISAMPLE_NONE:
		return "NONE";
	#ifdef PLATFORM_PC
    case D3DMULTISAMPLE_NONMASKABLE:
		return "NONMASKABLE";
	#endif
    case D3DMULTISAMPLE_2_SAMPLES:
		return "2_SAMPLES";
    #ifdef PLATFORM_PC
    case D3DMULTISAMPLE_3_SAMPLES:
		return "3_SAMPLES";
	#endif
    case D3DMULTISAMPLE_4_SAMPLES:
		return "4_SAMPLES";
    #ifdef PLATFORM_PC
	case D3DMULTISAMPLE_5_SAMPLES:
		return "5_SAMPLES";
	case D3DMULTISAMPLE_6_SAMPLES:
		return "6_SAMPLES";
    case D3DMULTISAMPLE_7_SAMPLES:
		return "7_SAMPLES";
    case D3DMULTISAMPLE_8_SAMPLES:
		return "8_SAMPLES";
    case D3DMULTISAMPLE_9_SAMPLES:
		return "9_SAMPLES";
    case D3DMULTISAMPLE_10_SAMPLES:
		return "10_SAMPLES";
    case D3DMULTISAMPLE_11_SAMPLES:
		return "11_SAMPLES";
    case D3DMULTISAMPLE_12_SAMPLES:
		return "12_SAMPLES";
    case D3DMULTISAMPLE_13_SAMPLES:
		return "13_SAMPLES";
    case D3DMULTISAMPLE_14_SAMPLES:
		return "14_SAMPLES";
    case D3DMULTISAMPLE_15_SAMPLES:
		return "15_SAMPLES";
    case D3DMULTISAMPLE_16_SAMPLES:
		return "16_SAMPLES";
	#endif
    default:
		return "UNKNOWN";
	}
}

const Util::C_String GetD3DSwapEffectAsString(const D3DSWAPEFFECT swapEffect)
{
	switch(swapEffect)
	{
	case D3DSWAPEFFECT_DISCARD:
		return "DISCARD";
	case D3DSWAPEFFECT_FLIP:
		return "FLIP";
	case D3DSWAPEFFECT_COPY:
		return "COPY";
	default:
		return "UNKNOWN";
	}
}

const Util::C_String GetD3DPresentFlagsAsString(const DWORD presentFlags)
{
	Util::C_String temp = "";

	#ifdef PLATFORM_PC
	if(presentFlags & D3DPRESENTFLAG_DEVICECLIP)
		temp += "DEVICECLIP/";
	#endif
	if(presentFlags & D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL)
		temp += "DISCARD_DEPTHSTENCIL/";
	#ifdef PLATFORM_PC
	if(presentFlags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER)
		temp += "LOCKABLE_BACKBUFFER/";
	if(presentFlags & D3DPRESENTFLAG_NOAUTOROTATE)
		temp += "NOAUTOROTATE/";
	if(presentFlags & D3DPRESENTFLAG_UNPRUNEDMODE)
		temp += "UNPRUNEDMODE/";
	if(presentFlags & D3DPRESENTFLAG_VIDEO)
		temp += "VIDEO/";
	#endif

	return temp;
}

const Util::C_String GetD3DPresentIntervalAsString(const UINT presentInterval)
{
	switch(presentInterval)
	{
	//case D3DPRESENT_DONOTFLIP:
		//return "DONOTFLIP";
	//case D3DPRESENT_DONOTWAIT:
		//return "DONOTWAIT";
	//case D3DPRESENT_FLIPRESTART:
		//return "FLIPRESTART";
	case D3DPRESENT_INTERVAL_DEFAULT:
		return "INTERVAL_DEFAULT";
	case D3DPRESENT_INTERVAL_ONE:
		return "INTERVAL_ONE";
	case D3DPRESENT_INTERVAL_TWO:
		return "INTERVAL_TWO";
	case D3DPRESENT_INTERVAL_THREE:
		return "INTERVAL_THREE";
	case D3DPRESENT_INTERVAL_FOUR:
		return "INTERVAL_FOUR";
	case D3DPRESENT_INTERVAL_IMMEDIATE:
		return "INTERVAL_IMMEDIATE";
	//case D3DPRESENT_LINEAR_CONTENT:
		//return "LINEAR_CONTENT";
	#ifdef PLATFORM_PC
	case D3DPRESENT_VIDEO_RESTRICT_TO_MONITOR:
		return "VIDEO_RESTRICT_TO_MONITOR";
	#endif
	default:
		return "UNKNOWN";
	}
}

const Util::C_String GetFlagsAsString(const U32 flags)
{
	Util::C_String str = "";
	
	#define ADD_FLAG_TO_STR(_str, _flags, _flag, _name) if(_flags & _flag) { if(_str != "") _str += "|"; _str += _name; }
	
	ADD_FLAG_TO_STR(str, flags, E_Flags_Video,			"Video");
	ADD_FLAG_TO_STR(str, flags, E_Flags_System,			"System");
	ADD_FLAG_TO_STR(str, flags, E_Flags_Indirect,		"Indirect");
	ADD_FLAG_TO_STR(str, flags, E_Flags_Dynamic,		"Dynamic");
	ADD_FLAG_TO_STR(str, flags, E_Flags_Colour,			"Colour");
	ADD_FLAG_TO_STR(str, flags, E_Flags_DepthStencil,	"DepthStencil");
	ADD_FLAG_TO_STR(str, flags, E_Flags_Raw,			"Raw");
	ADD_FLAG_TO_STR(str, flags, E_Flags_Tiled,			"Tiled");
	ADD_FLAG_TO_STR(str, flags, E_Flags_Linear,			"Linear");
	ADD_FLAG_TO_STR(str, flags, E_Flags_SRGB,			"SRGB");
	ADD_FLAG_TO_STR(str, flags, E_Flags_DoNotTrack,		"DoNotTrack");
	ADD_FLAG_TO_STR(str, flags, E_Flags_DoNotReload,	"DoNotReload");

	return str;
}

const Util::C_String GetIndexFormatAsString(const E_IndexFormat indexFormat)
{
	switch(indexFormat)
	{
	case E_IndexFormat_16:						return "16";	
	case E_IndexFormat_32:						return "32";		
	default:									return "Unknown";
	}
}

const Util::C_String GetMultisampleAsString(const E_Multisample multisample)
{
	switch(multisample)
	{
	case E_Multisample_None:					return "1xAA";	
	case E_Multisample_2:						return "2xAA";		
	case E_Multisample_4:						return "4xAA";		
	default:									return "Unknown";
	}
}

const Util::C_String GetSurfaceFormatAsString(const E_SurfaceFormat format)
{
	switch(format)
	{
	// 8bpp:
	case E_SurfaceFormat_A8: 					return "A8";
	case E_SurfaceFormat_L8: 					return "L8";

	// 16bpp:
	case E_SurfaceFormat_R5G6B5: 				return "R5G6B5";
	case E_SurfaceFormat_L6V5U5: 				return "L6V5U5";
	case E_SurfaceFormat_X1R5G5B5: 				return "X1R5G5B5";
	case E_SurfaceFormat_A1R5G5B5: 				return "A1R5G5B5";
	case E_SurfaceFormat_A4R4G4B4: 				return "A4R4G4B4";
	case E_SurfaceFormat_X4R4G4B4: 				return "X4R4G4B4";
	case E_SurfaceFormat_A8L8: 					return "A8L8";
	case E_SurfaceFormat_V8U8: 					return "V8U8";
	case E_SurfaceFormat_D16: 					return "D16";
	case E_SurfaceFormat_L16: 					return "L16";
	case E_SurfaceFormat_R16F: 					return "R16F";
	case E_SurfaceFormat_UYVY: 					return "UYVY";
	case E_SurfaceFormat_G8R8_G8B8: 			return "G8R8_G8B8";
	case E_SurfaceFormat_R8G8_B8G8: 			return "R8G8_B8G8";
	case E_SurfaceFormat_YUY2:					return "YUY2";
	#if DEVICE_XB2
	case E_SurfaceFormat_R6G5B5:				return "R6G5B5";
	case E_SurfaceFormat_Q4W4V4U4:				return "Q4W4V4U4";
	case E_SurfaceFormat_G8R8:					return "G8R8";
	case E_SurfaceFormat_R16F_EXPAND: 			return "R16F_EXPAND";
	case E_SurfaceFormat_LE_LIN_UYVY: 			return "LE_UYVY";
	case E_SurfaceFormat_LE_LIN_YUY2: 			return "LE_YUY2";
	#endif

	// 32bpp:
	case E_SurfaceFormat_A8R8G8B8: 				return "A8R8G8B8";
	case E_SurfaceFormat_X8R8G8B8: 				return "X8R8G8B8";
	case E_SurfaceFormat_A8B8G8R8: 				return "A8B8G8R8";
	case E_SurfaceFormat_X8B8G8R8: 				return "X8B8G8R8";
	case E_SurfaceFormat_X8L8V8U8: 				return "X8L8V8U8";
	case E_SurfaceFormat_Q8W8V8U8: 				return "Q8W8V8U8";
	case E_SurfaceFormat_A2R10G10B10: 			return "A2R10G10B10";
	case E_SurfaceFormat_A2B10G10R10: 			return "A2B10G10R10";
	case E_SurfaceFormat_A2W10V10U10: 			return "A2W10V10U10";
	case E_SurfaceFormat_G16R16: 				return "G16R16";
	case E_SurfaceFormat_V16U16: 				return "V16U16";
	case E_SurfaceFormat_G16R16F:				return "G16R16F";
	case E_SurfaceFormat_R32F:					return "R32F";
	case E_SurfaceFormat_D24S8:					return "D24S8";
	case E_SurfaceFormat_D24X8:					return "D24X8";
	case E_SurfaceFormat_D24FS8:				return "D24FS8";
	case E_SurfaceFormat_D32:					return "D32";
	#if DEVICE_XB2
	case E_SurfaceFormat_X2R10G10B10:			return "X2R10G10B10";
	case E_SurfaceFormat_A16L16:				return "A16L16";
	case E_SurfaceFormat_R10G11B11: 			return "R10G11B11";
	case E_SurfaceFormat_R11G11B10: 			return "R11G11B10";
	case E_SurfaceFormat_W10V11U11: 			return "W10V11U11";
	case E_SurfaceFormat_W11V11U10: 			return "W11V11U10";
	case E_SurfaceFormat_G16R16F_EXPAND:		return "G16R16F_EXPAND";
	case E_SurfaceFormat_L32:					return "L32";
	#endif

	// 64bpp:
	case E_SurfaceFormat_A16B16G16R16: 			return "A16B16G16R16";
	case E_SurfaceFormat_Q16W16V16U16: 			return "Q16W16V16U16";
	case E_SurfaceFormat_A16B16G16R16F:			return "A16B16G16R16F";
	case E_SurfaceFormat_G32R32F:				return "G32R32F";
	#if DEVICE_XB2
	case E_SurfaceFormat_A16B16G16R16F_EXPAND:	return "A16B16G16R16F_EXPAND";
	case E_SurfaceFormat_A32L32: 				return "A32L32";
	case E_SurfaceFormat_G32R32: 				return "G32R32";
	case E_SurfaceFormat_V32U32: 				return "V32U32";
	#endif

	// 128bpp:
	case E_SurfaceFormat_A32B32G32R32F:			return "A32B32G32R32F";
	#if DEVICE_XB2
	case E_SurfaceFormat_A32B32G32R32: 			return "A32B32G32R32";
	case E_SurfaceFormat_Q32W32V32U32: 			return "Q32W32V32U32";
	#endif
	
	// EDRAM only:
	#if DEVICE_XB2
	case E_SurfaceFormat_A2B10G10R10F_EDRAM:	return "A2B10G10R10F_EDRAM";
	case E_SurfaceFormat_G16R16_EDRAM:			return "G16R16_EDRAM";
	case E_SurfaceFormat_A16B16G16R16_EDRAM:	return "A16B16G16R16_EDRAM";
	#endif

	// Front buffer formats have to be little endian:
	#if DEVICE_XB2
	case E_SurfaceFormat_LE_X8R8G8B8: 			return "LE_X8R8G8B8";
	case E_SurfaceFormat_LE_A8R8G8B8: 			return "LE_A8R8G8B8";
	case E_SurfaceFormat_LE_X2R10G10B10: 		return "LE_X2R10G10B10";
	case E_SurfaceFormat_LE_A2R10G10B10: 		return "LE_A2R10G10B10";
	#endif

	default:									return "Unknown";
	}
}

const Util::C_String GetTextureFormatAsString(const E_TextureFormat format)
{
	switch(format)
	{
	// DXT:
	case E_TextureFormat_DXT1: 					return "DXT1";
	case E_TextureFormat_DXT3: 					return "DXT3";
	case E_TextureFormat_DXT5: 					return "DXT5";
	#if DEVICE_XB2
	case E_TextureFormat_DXT3A:					return "DXT3A";
	case E_TextureFormat_DXT3A_1111:			return "DXT3A_1111";
	case E_TextureFormat_DXT5A:					return "DXT5A";
	case E_TextureFormat_DXN:					return "DXN";
	case E_TextureFormat_CTX1:					return "CTX1";
	#endif

	// 8bpp:
	case E_TextureFormat_A8: 					return "A8";
	case E_TextureFormat_L8: 					return "L8";

	// 16bpp:
	case E_TextureFormat_R5G6B5: 				return "R5G6B5";
	case E_TextureFormat_L6V5U5: 				return "L6V5U5";
	case E_TextureFormat_X1R5G5B5: 				return "X1R5G5B5";
	case E_TextureFormat_A1R5G5B5: 				return "A1R5G5B5";
	case E_TextureFormat_A4R4G4B4: 				return "A4R4G4B4";
	case E_TextureFormat_X4R4G4B4: 				return "X4R4G4B4";
	case E_TextureFormat_A8L8: 					return "A8L8";
	case E_TextureFormat_V8U8: 					return "V8U8";
	case E_TextureFormat_D16: 					return "D16";
	case E_TextureFormat_L16: 					return "L16";
	case E_TextureFormat_R16F: 					return "R16F";
	case E_TextureFormat_UYVY: 					return "UYVY";
	case E_TextureFormat_G8R8_G8B8: 			return "G8R8_G8B8";
	case E_TextureFormat_R8G8_B8G8: 			return "R8G8_B8G8";
	case E_TextureFormat_YUY2:					return "YUY2";
	#if DEVICE_XB2
	case E_TextureFormat_R6G5B5:				return "R6G5B5";
	case E_TextureFormat_Q4W4V4U4:				return "Q4W4V4U4";
	case E_TextureFormat_G8R8:					return "G8R8";
	case E_TextureFormat_R16F_EXPAND: 			return "R16F_EXPAND";
	case E_TextureFormat_LE_LIN_UYVY: 			return "LE_UYVY";
	case E_TextureFormat_LE_LIN_YUY2: 			return "LE_YUY2";
	#endif

	// 32bpp:
	case E_TextureFormat_A8R8G8B8: 				return "A8R8G8B8";
	case E_TextureFormat_X8R8G8B8: 				return "X8R8G8B8";
	case E_TextureFormat_A8B8G8R8: 				return "A8B8G8R8";
	case E_TextureFormat_X8B8G8R8: 				return "X8B8G8R8";
	case E_TextureFormat_X8L8V8U8: 				return "X8L8V8U8";
	case E_TextureFormat_Q8W8V8U8: 				return "Q8W8V8U8";
	case E_TextureFormat_A2R10G10B10: 			return "A2R10G10B10";
	case E_TextureFormat_A2B10G10R10: 			return "A2B10G10R10";
	case E_TextureFormat_A2W10V10U10: 			return "A2W10V10U10";
	case E_TextureFormat_G16R16: 				return "G16R16";
	case E_TextureFormat_V16U16: 				return "V16U16";
	case E_TextureFormat_G16R16F:				return "G16R16F";
	case E_TextureFormat_R32F:					return "R32F";
	case E_TextureFormat_D24S8:					return "D24S8";
	case E_TextureFormat_D24X8:					return "D24X8";
	case E_TextureFormat_D24FS8:				return "D24FS8";
	case E_TextureFormat_D32:					return "D32";
	#if DEVICE_XB2
	case E_TextureFormat_X2R10G10B10:			return "X2R10G10B10";
	case E_TextureFormat_A16L16:				return "A16L16";
	case E_TextureFormat_R10G11B11: 			return "R10G11B11";
	case E_TextureFormat_R11G11B10: 			return "R11G11B10";
	case E_TextureFormat_W10V11U11: 			return "W10V11U11";
	case E_TextureFormat_W11V11U10: 			return "W11V11U10";
	case E_TextureFormat_G16R16F_EXPAND:		return "G16R16F_EXPAND";
	case E_TextureFormat_L32:					return "L32";
	#endif

	// 64bpp:
	case E_TextureFormat_A16B16G16R16: 			return "A16B16G16R16";
	case E_TextureFormat_Q16W16V16U16: 			return "Q16W16V16U16";
	case E_TextureFormat_A16B16G16R16F:			return "A16B16G16R16F";
	case E_TextureFormat_G32R32F:				return "G32R32F";
	#if DEVICE_XB2
	case E_TextureFormat_A16B16G16R16F_EXPAND:	return "A16B16G16R16F_EXPAND";
	case E_TextureFormat_A16B16G16R16_UINT:		return "A16B16G16R16_UINT";
	case E_TextureFormat_A32L32: 				return "A32L32";
	case E_TextureFormat_G32R32: 				return "G32R32";
	case E_TextureFormat_V32U32: 				return "V32U32";
	#endif

	// 128bpp:
	case E_TextureFormat_A32B32G32R32F:			return "A32B32G32R32F";
	#if DEVICE_XB2
	case E_TextureFormat_A32B32G32R32: 			return "A32B32G32R32";
	case E_TextureFormat_Q32W32V32U32: 			return "Q32W32V32U32";
	#endif
	
	// EDRAM only:
	#if DEVICE_XB2
	case E_TextureFormat_A2B10G10R10F_EDRAM:	return "A2B10G10R10F_EDRAM";
	case E_TextureFormat_G16R16_EDRAM:			return "G16R16_EDRAM";
	case E_TextureFormat_A16B16G16R16_EDRAM:	return "A16B16G16R16_EDRAM";
	#endif

	// Front buffer formats have to be little endian:
	#if DEVICE_XB2
	case E_TextureFormat_LE_X8R8G8B8: 			return "LE_X8R8G8B8";
	case E_TextureFormat_LE_A8R8G8B8: 			return "LE_A8R8G8B8";
	case E_TextureFormat_LE_X2R10G10B10: 		return "LE_X2R10G10B10";
	case E_TextureFormat_LE_A2R10G10B10: 		return "LE_A2R10G10B10";
	#endif

	default:									return "Unknown";
	}
}

const Util::C_String GetTextureTypeAsString(const E_TextureType type)
{
	switch(type)
	{
	case E_TextureType_2D:						return "2D";
	case E_TextureType_3D:						return "3D";	
	case E_TextureType_Cube:					return "Cube";
	default:									return "Unknown";
	}
}

C_Viewport::C_Viewport()
{
	TopLeftX		= 0;
	TopLeftY		= 0;
	Width	= 1;
	Height	= 1;
	MinDepth	= 0.0f;
	MaxDepth	= 1.0f;
}

C_Viewport::C_Viewport(const U32 x, const U32 y, const U32 width, const U32 height)
{
	TopLeftX		= x;
	TopLeftY		= y;
	Width	= width;
	Height	= height;
	MinDepth	= 0.0f;
	MaxDepth	= 1.0f;
}

C_Viewport::C_Viewport(const U32 x, const U32 y, const U32 width, const U32 height, const F32 minZ, const F32 maxZ)
{
	TopLeftX		= x;
	TopLeftY		= y;
	Width	= width;
	Height	= height;
	MinDepth	= minZ;
	MaxDepth	= maxZ;
}

// C_VertexElement

C_VertexElement::C_VertexElement()
{
	Stream		= 0xFF;
	Offset		= 0;
	Type		= D3DDECLTYPE_UNUSED;
	Method		= 0;
	Usage		= 0;
	UsageIndex	= 0;
}

C_VertexElement::C_VertexElement(const U16 stream, const U32 offset, const U32 type, const U8 usage, const U8 usageIndex)
{
	Stream		= stream;
	Offset		= offset;
	Type		= type;
	Method		= 0;
	Usage		= usage;
	UsageIndex	= usageIndex;
}

void C_VertexElement::SetStream(const U16 stream)
{
	Stream = stream;
}

void C_VertexElement::SetOffset(const U32 offset)
{
	Offset = offset;
}

void C_VertexElement::SetType(const U32 type)
{
	Type = type;
}

void C_VertexElement::SetUsage(const U8 usage)
{
	Usage = usage;
}

void C_VertexElement::SetUsageIndex(const U8 usageIndex)
{
	UsageIndex = usageIndex;
}

// C_VertexDeclaration

C_VertexDeclaration::C_VertexDeclaration() : m_pd3dVertexDeclaration(nullptr), m_ElementCount(0) {}
C_VertexDeclaration::~C_VertexDeclaration() {}
S32 C_VertexDeclaration::GetElementCount() const { return static_cast<S32>(m_ElementCount); }
bool C_VertexDeclaration::GetElements(U32 count, C_VertexElement* elements) const
{
    if (!elements || count != m_ElementCount) return false;
    for (U32 i = 0; i < count; ++i) elements[i] = m_Elements[i];
    return true;
}
U32 C_VertexDeclaration::GetStride(U32 stream) const
{
    U32 stride = 0;
    for (U32 i = 0; i < m_ElementCount; ++i)
        if (m_Elements[i].GetStream() == stream)
            stride = std::max(stride, m_Elements[i].GetOffset() + m_Elements[i].GetSize());
    return stride;
}

// C_VertexBuffer

C_VertexBuffer::C_VertexBuffer() : m_Flags(0)
{
#if DEVICE_XB2
	ZeroMemory(&m_d3dVertexBuffer, sizeof(GPUVertexBuffer));
#else
	m_pd3dVertexBuffer = NULL;
#endif
}

C_VertexBuffer::~C_VertexBuffer()
{
}

void C_VertexBuffer::SetSize(const U32 size)
{
#if DEVICE_XB2
	m_d3dVertexBuffer.Format.Size = size;
#endif
}

void C_VertexBuffer::SetFlags(const U32 flags)
{
	m_Flags = flags;
}

void C_VertexBuffer::SetAddress(void* pAddress)
{
#if DEVICE_XB2
	m_d3dVertexBuffer.Format.BaseAddress = (U32)pAddress;
#endif
}

void* C_VertexBuffer::Lock(const U32 offset, const U32 size, const U32 flags)
{
	BIZ_ASSERT(IsCreated());
	D3D11_MAPPED_SUBRESOURCE mapped = {};
	const bool ok = LockResource(m_pd3dVertexBuffer, 0, true, mapped, flags);
	BIZ_ASSERT_MSG(ok, "Failed to lock the d3d11 vertex buffer");
	return ok ? static_cast<U8*>(mapped.pData) + offset : nullptr;
}

void* C_VertexBuffer::Lock()
{
	return Lock(0, 0, 0);
}

void C_VertexBuffer::Unlock()
{
	BIZ_ASSERT(IsCreated());
	UnlockStagingResource(m_pd3dVertexBuffer, 0);
}

// C_IndexBuffer

C_IndexBuffer::C_IndexBuffer() : m_Flags(0)
{
#if DEVICE_XB2
	ZeroMemory(&m_d3dIndexBuffer, sizeof(GPUIndexBuffer));
#else
	m_pd3dIndexBuffer = NULL;
#endif
}

C_IndexBuffer::~C_IndexBuffer()
{
}

void C_IndexBuffer::SetSize(const U32 size)
{
#if DEVICE_XB2
	m_d3dIndexBuffer.Size = size;
#endif
}

void C_IndexBuffer::SetFlags(const U32 flags)
{
	m_Flags = flags;
}

void C_IndexBuffer::SetAddress(void* pAddress)
{
#if DEVICE_XB2
	m_d3dIndexBuffer.Address = (U32)pAddress;
#endif
}

void* C_IndexBuffer::Lock(const U32 offset, const U32 size, const U32 flags)
{
	BIZ_ASSERT(IsCreated());
	D3D11_MAPPED_SUBRESOURCE mapped = {};
	const bool ok = LockResource(m_pd3dIndexBuffer, 0, true, mapped, flags);
	BIZ_ASSERT_MSG(ok, "Failed to lock the d3d11 index buffer");
	return ok ? static_cast<U8*>(mapped.pData) + offset : nullptr;
}

void* C_IndexBuffer::Lock()
{
	return Lock(0, 0, 0);
}

void C_IndexBuffer::Unlock()
{
	BIZ_ASSERT(IsCreated());
	UnlockStagingResource(m_pd3dIndexBuffer, 0);
}

// C_ShaderSemantic

C_ShaderSemantic::C_ShaderSemantic() :
	m_Usage(UCHAR_MAX),
	m_UsageIndex(UCHAR_MAX)
{
}

// C_ShaderParam

C_ShaderParam::C_ShaderParam() :
	m_RegisterIndex(UINT_MAX),
	m_RegisterCount(0)
{
}

C_ShaderParam::C_ShaderParam(const U32 registerIndex, const U32 registerCount) :
	m_RegisterIndex(registerIndex),
	m_RegisterCount(registerCount)
{
}

void C_ShaderParam::SetupFromFunc(const char* name, const U8* func, U32 size)
{
    if (!name || !func || !size) return;
    ID3D11ShaderReflection* reflection = nullptr;
    if (FAILED(D3DReflect(func, size, IID_ID3D11ShaderReflection, (void**)&reflection))) return;
    D3D11_SHADER_DESC shader = {};
    if (SUCCEEDED(reflection->GetDesc(&shader)))
        for (UINT bufferIndex = 0; bufferIndex < shader.ConstantBuffers; ++bufferIndex)
        {
            ID3D11ShaderReflectionConstantBuffer* buffer = reflection->GetConstantBufferByIndex(bufferIndex);
            D3D11_SHADER_BUFFER_DESC bufferDesc = {};
            if (FAILED(buffer->GetDesc(&bufferDesc))) continue;
            for (UINT variableIndex = 0; variableIndex < bufferDesc.Variables; ++variableIndex)
            {
                ID3D11ShaderReflectionVariable* variable = buffer->GetVariableByIndex(variableIndex);
                D3D11_SHADER_VARIABLE_DESC desc = {};
                if (SUCCEEDED(variable->GetDesc(&desc)) && strcmp(desc.Name, name) == 0)
                {
                    m_RegisterIndex = desc.StartOffset / 16;
                    m_RegisterCount = (desc.Size + 15) / 16;
                    reflection->Release();
                    return;
                }
            }
        }
    reflection->Release();
}

// C_VertexShader

C_VertexShader::C_VertexShader() : m_pd3dVertexShader(NULL), m_Flags(0)
{
}

C_VertexShader::~C_VertexShader()
{
}

void C_VertexShader::SetFlags(const U32 flags)
{
	m_Flags = flags;
}

const C_ShaderParam C_VertexShader::GetParamFromName(const char* name) const
{
    C_ShaderParam param;
    if (!m_Bytecode.empty()) param.SetupFromFunc(name, m_Bytecode.data(), m_Bytecode.size());
    return param;
}

const C_ShaderParam C_VertexShader::GetParameter(const S32 index, const char* name) const
{
	return GetParamFromName(name);
}

bool C_VertexShader::GetSemantics(Btl::Vector<C_ShaderSemantic>& semantics)
{
    if (m_Bytecode.empty()) return false;
    ID3D11ShaderReflection* reflection = nullptr;
    if (FAILED(D3DReflect(m_Bytecode.data(), m_Bytecode.size(),
                          IID_ID3D11ShaderReflection, (void**)&reflection))) return false;
    D3D11_SHADER_DESC shader = {};
    const bool valid = SUCCEEDED(reflection->GetDesc(&shader));
    if (valid)
        for (UINT i = 0; i < shader.InputParameters; ++i)
        {
            D3D11_SIGNATURE_PARAMETER_DESC desc = {};
            if (SUCCEEDED(reflection->GetInputParameterDesc(i, &desc)))
            {
                C_ShaderSemantic semantic;
                semantic.m_Usage = static_cast<U8>(desc.SystemValueType == D3D_NAME_POSITION ?
                    D3DDECLUSAGE_POSITION : D3DDECLUSAGE_TEXCOORD);
                semantic.m_UsageIndex = static_cast<U8>(desc.SemanticIndex);
                semantics.PushBack(semantic);
            }
        }
    reflection->Release();
    return valid;
}

// C_PixelShader

C_PixelShader::C_PixelShader() : m_pd3dPixelShader(NULL), m_Flags(0)
{
#if DEVICE_XB2
	m_pAddress = NULL;
#endif
}

C_PixelShader::~C_PixelShader()
{
}

void C_PixelShader::SetFlags(const U32 flags)
{
	m_Flags = flags;
}

void C_PixelShader::SetAddress(U8* pAddress)
{
#if DEVICE_XB2
	m_pAddress = pAddress;
#endif
}

const C_ShaderParam C_PixelShader::GetParamFromName(const char* name) const
{
    C_ShaderParam param;
    if (!m_Bytecode.empty()) param.SetupFromFunc(name, m_Bytecode.data(), m_Bytecode.size());
    return param;
}

const C_ShaderParam C_PixelShader::GetParameter(const S32 index, const char* name) const
{
	return GetParamFromName(name);
}

const C_ShaderParam C_PixelShader::GetParameterArray(const S32 index, const char* name, const U32 count) const
{
	return GetParamFromName(name);
}

// C_LockedRect

C_LockedRect::C_LockedRect()
{
	pBits = NULL;
	pitch = 0;
}

// C_ConstLockedRect

C_ConstLockedRect::C_ConstLockedRect()
{
	pBits = NULL;
	pitch = 0;
}

// C_LockedBox

C_LockedBox::C_LockedBox()
{
	pBits = NULL;
	rowPitch = 0;
	slicePitch = 0;
}

// C_ConstLockedBox

C_ConstLockedBox::C_ConstLockedBox()
{
	pBits = NULL;
	rowPitch = 0;
	slicePitch = 0;
}

// C_SurfaceParams

C_SurfaceParams::C_SurfaceParams()
{
#if DEVICE_XB2
	Base				= 0;
#if 1
	HierarchicalZBase	= 0; // This needs to be zero or it don't work!
#else
	HierarchicalZBase	= 0xFFFFFFFF;
#endif
	ColorExpBias		= 0;
	HiZFunc				= D3DHIZFUNC_DEFAULT;
#endif
}

C_SurfaceParams::~C_SurfaceParams()
{
}

void C_SurfaceParams::OverSurface(const C_Surface* pSurface)
{
#if DEVICE_XB2
	BIZ_ASSERT(pSurface && pSurface->IsCreated());
	
	Base = pSurface->m_d3dSurface.ColorInfo.ColorBase;
#endif
}

void C_SurfaceParams::AfterSurface(const C_Surface* pSurface)
{
#if DEVICE_XB2
	BIZ_ASSERT(pSurface && pSurface->IsCreated());
	
	Base = pSurface->m_d3dSurface.ColorInfo.ColorBase + (pSurface->m_d3dSurface.Size / GPU_EDRAM_TILE_SIZE);
#endif
}

void C_SurfaceParams::BeforeSurface(const C_Surface* pSurface)
{
#if DEVICE_XB2
	BIZ_ASSERT(pSurface && pSurface->IsCreated());
	
	Base = pSurface->m_d3dSurface.ColorInfo.ColorBase - (pSurface->m_d3dSurface.Size / GPU_EDRAM_TILE_SIZE);
#endif
}

// C_Surface

C_Surface::C_Surface() : m_Flags(0), m_Subresource(0)
{
#if DEVICE_XB2
	ZeroMemory(&m_d3dSurface, sizeof(GPUSurface));
#else
	m_pd3dSurface = NULL;
#endif
}

C_Surface::~C_Surface()
{
}

void C_Surface::SetSize(const U32 size)
{
#if DEVICE_XB2
	m_d3dSurface.Size = size;
#endif
}

void C_Surface::SetFlags(const U32 flags)
{
	m_Flags = flags;
}

void C_Surface::SetAddress(U8* pAddress)
{
}

C_LockedRect C_Surface::LockRect(const U32 flags)
{
    BIZ_ASSERT(IsCreated());
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    const bool ok = LockResource(m_pd3dSurface, m_Subresource, true, mapped);
    BIZ_ASSERT_MSG(ok, "Failed to lock the d3d11 surface");
    C_LockedRect rect = {};
    if (ok) { rect.pBits = static_cast<U8*>(mapped.pData); rect.pitch = mapped.RowPitch; }
    return rect;
}

void C_Surface::UnlockRect()
{
    BIZ_ASSERT(IsCreated());
    UnlockStagingResource(m_pd3dSurface, m_Subresource);
}

// C_Texture

C_Texture::C_Texture() : m_Size(0), m_Flags(0)
{
#if DEVICE_XB2
	ZeroMemory(&m_d3dTexture, sizeof(GPUTexture));
#else
	m_pd3dTexture = NULL;
#endif
}

C_Texture::~C_Texture()
{
}

void C_Texture::SetSize(const U32 size)
{
	m_Size = size;
}

void C_Texture::SetFlags(const U32 flags)
{
	m_Flags = flags;
}

void C_Texture::SetAddress(U8* pAddress)
{
#if DEVICE_XB2
	m_d3dTexture.Format.BaseAddress = (U32)pAddress >> GPU_TEXTURE_ADDRESS_SHIFT;
#endif
}

void C_Texture::SetSRGB(const bool enable)
{
	if(enable)
	{
		m_Flags |= E_Flags_SRGB;

		#if DEVICE_XB2
		m_d3dTexture.Format.SignX = GPUSIGN_GAMMA;
		m_d3dTexture.Format.SignY = GPUSIGN_GAMMA;
		m_d3dTexture.Format.SignZ = GPUSIGN_GAMMA;
		#endif
	}
	else
	{
		m_Flags &= ~E_Flags_SRGB;

		#if DEVICE_XB2
		BIZ_ASSERT(m_d3dTexture.Format.SignX != GPUSIGN_GAMMA);
		BIZ_ASSERT(m_d3dTexture.Format.SignY != GPUSIGN_GAMMA);
		BIZ_ASSERT(m_d3dTexture.Format.SignZ != GPUSIGN_GAMMA);
		#endif
	}
}

C_LockedRect C_Texture::LockRect2D(const U32 level, const U32 flags)
{
    BIZ_ASSERT(IsCreated() && GetType() == E_TextureType_2D);
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    const bool ok = LockResource(GetGPUTexture2D(), level, true, mapped);
    BIZ_ASSERT_MSG(ok, "Failed to lock d3d11 texture 2d");
    C_LockedRect rect = {};
    if (ok) { rect.pBits = static_cast<U8*>(mapped.pData); rect.pitch = mapped.RowPitch; }
    return rect;
}

C_ConstLockedRect C_Texture::LockRect2D(const U32 level, const U32 flags) const
{
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    const bool ok = LockResource(const_cast<GPUTexture2D*>(GetGPUTexture2D()), level, false, mapped);
    BIZ_ASSERT_MSG(ok, "Failed to lock d3d11 texture 2d");
    C_ConstLockedRect rect = {};
    if (ok) { rect.pBits = static_cast<const U8*>(mapped.pData); rect.pitch = mapped.RowPitch; }
    return rect;
}

void C_Texture::UnlockRect2D(const U32 level) const
{
    UnlockStagingResource(const_cast<GPUTexture2D*>(GetGPUTexture2D()), level);
}

C_LockedBox C_Texture::LockBox3D(const U32 level, const U32 flags)
{
    BIZ_ASSERT(IsCreated() && GetType() == E_TextureType_3D);
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    const bool ok = LockResource(GetGPUTexture3D(), level, true, mapped);
    BIZ_ASSERT_MSG(ok, "Failed to lock d3d11 texture 3d");
    C_LockedBox box = {};
    if (ok) { box.pBits = static_cast<U8*>(mapped.pData); box.rowPitch = mapped.RowPitch; box.slicePitch = mapped.DepthPitch; }
    return box;
}

C_ConstLockedBox C_Texture::LockBox3D(const U32 level, const U32 flags) const
{
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    const bool ok = LockResource(const_cast<GPUTexture3D*>(GetGPUTexture3D()), level, false, mapped);
    BIZ_ASSERT_MSG(ok, "Failed to lock d3d11 texture 3d");
    C_ConstLockedBox box = {};
    if (ok) { box.pBits = static_cast<const U8*>(mapped.pData); box.rowPitch = mapped.RowPitch; box.slicePitch = mapped.DepthPitch; }
    return box;
}

void C_Texture::UnlockBox3D(const U32 level) const
{
    UnlockStagingResource(const_cast<GPUTexture3D*>(GetGPUTexture3D()), level);
}

C_LockedRect C_Texture::LockRectCube(const U32 face, const U32 level, const U32 flags)
{
    BIZ_ASSERT(IsCreated() && GetType() == E_TextureType_Cube);
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    const bool ok = LockResource(GetGPUTextureCube(), face * GetLevels() + level, true, mapped);
    BIZ_ASSERT_MSG(ok, "Failed to lock d3d11 cube texture");
    C_LockedRect rect = {};
    if (ok) { rect.pBits = static_cast<U8*>(mapped.pData); rect.pitch = mapped.RowPitch; }
    return rect;
}

C_ConstLockedRect C_Texture::LockRectCube(const U32 face, const U32 level, const U32 flags) const
{
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    const bool ok = LockResource(const_cast<GPUTextureCube*>(GetGPUTextureCube()), face * GetLevels() + level, false, mapped);
    BIZ_ASSERT_MSG(ok, "Failed to lock d3d11 cube texture");
    C_ConstLockedRect rect = {};
    if (ok) { rect.pBits = static_cast<const U8*>(mapped.pData); rect.pitch = mapped.RowPitch; }
    return rect;
}

void C_Texture::UnlockRectCube(const U32 face, const U32 level) const
{
    UnlockStagingResource(const_cast<GPUTextureCube*>(GetGPUTextureCube()), face * GetLevels() + level);
}

GPUTexture* C_Texture::Get()
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dTexture : NULL;
#else
	return m_pd3dTexture;
#endif

}
const Util::C_String C_Texture::GetFileName() const
{
	return "";
}

C_LockedRect C_Texture::Lock(const U32 level)
{
	BIZ_ASSERT(GetType() == E_ResourceType_Texture2D);

	return LockRect2D(level);
}

C_ConstLockedRect C_Texture::Lock(const U32 level) const
{
	BIZ_ASSERT(GetType() == E_ResourceType_Texture2D);

	return LockRect2D(level);
}

void C_Texture::Unlock(const U32 level) const
{
	BIZ_ASSERT(GetType() == E_ResourceType_Texture2D);
	
	UnlockRect2D(level);
}

// C_RenderTexture2D

C_RenderTexture2D::C_RenderTexture2D() : m_Flags(0)
{
#if DEVICE_D3D9_PC || DEVICE_D3D11_PC // [Blur1PCDX11] The native layout carries these slots too.
	m_TextureIndex = 0;
	m_SurfaceIndex = 0;
	m_Multisample = D3DMULTISAMPLE_NONE;	// [Blur2PC]
#endif
}

C_RenderTexture2D::~C_RenderTexture2D()
{
}

// C_RenderTextureCube

C_RenderTextureCube::C_RenderTextureCube() : m_Flags(0)
{
}

C_RenderTextureCube::~C_RenderTextureCube()
{
}

// C_ContextBase

C_ContextBase::C_ContextBase() :
	m_pd3dDevice(NULL)
{
}

C_ContextBase::C_ContextBase(GPUDevice* pd3dDevice) :
	m_pd3dDevice(pd3dDevice)
{
}

// C_Context

C_Context::C_Context() :
	m_pState(NULL)
{
}

C_Context::C_Context(GPUDevice* pd3dDevice) :
	C_ContextBase(pd3dDevice),
	m_pState(NULL)
{
}

// C_ContextNoCheck

C_ContextNoCheck::C_ContextNoCheck() :
	m_pState(NULL)
{
}

C_ContextNoCheck::C_ContextNoCheck(GPUDevice* pd3dDevice) :
	C_ContextBase(pd3dDevice),
	m_pState(NULL)
{
}

// C_ContextNoCheckNoTrack

C_ContextNoCheckNoTrack::C_ContextNoCheckNoTrack()
{
}

C_ContextNoCheckNoTrack::C_ContextNoCheckNoTrack(GPUDevice* pd3dDevice) :
	C_ContextBase(pd3dDevice)
{
}


} // namespace Display2

