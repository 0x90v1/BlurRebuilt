#include "stdafx.h"
#include "Movie/XB2/Bink/binktextures.h"

#ifdef CFG_ALLOW_OPTIMIZE_OFF_RENDER_MOVIE
#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF_RENDER_MOVIE
#endif

//
// pointers to our local vertex and pixel shader
//

static Display::C_VertexShader	m_PositionAndTexCoordPassThrough;
static Display::C_PixelShader	m_YCrCbToRGBNoPixelAlpha;
static Display::C_PixelShader	m_YCrCbAToRGBA;

//
// structure and definition for uploading our texture verts
//

#define POS_TC_VERTEX_FVF ( D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE4( 0 ) )
typedef struct POS_TC_VERTEX 
{
  F32 sx, sy, sz;			// Screen coordinates
  F32 tu0, tv0, tu1, tv1;	// Texture coordinates 
} POS_TC_VERTEX;

EXTERN_SHADER(vsBink);
EXTERN_SHADER(psBink);
EXTERN_SHADER(psBinkNoAlpha);

//
// simple pass through vertex shader
//

//static CONST CHAR StrPositionAndTexCoordPassThrough[] = 
//" struct VS_DATA                                        "
//" {                                                     "
//"     float4 Pos : POSITION;                            "
//"     float4 T0: TEXCOORD0;                             "
//" };                                                    "
//"                                                       "
//" VS_DATA main( VS_DATA In )                            "
//" {                                                     "
//"     return In;                                        "
//" }                                                     ";

//
// simple pixel shader to apply the yuvtorgb matrix
//

//static const char StrYCrCbToRGBNoPixelAlpha[] =
//" sampler tex0   : register( s0 );      "
//" sampler tex1   : register( s1 );      "
//" sampler tex2   : register( s2 );      "
//" float4  tor    : register( c0 );      "
//" float4  tog    : register( c1 );      "
//" float4  tob    : register( c2 );      "
//" float4  consts : register( c3 );      "
//"                                       "
//" struct VS_OUT                         "
//" {                                     "
//"     float2 T0: TEXCOORD0;             "
//" };                                    "
//"                                       "
//" float4 main( VS_OUT In ) : COLOR      "
//" {                                     "
//"   float4 c;                           "
//"   float4 p;                           "
//"   c.x = tex2D( tex0, In.T0 ).x;       "
//"   c.y = tex2D( tex1, In.T0 ).x;       "
//"   c.z = tex2D( tex2, In.T0 ).x;       "
//"   c.w = consts.x;                     "
//"   p.x = dot( tor, c );                "
//"   p.y = dot( tog, c );                "
//"   p.z = dot( tob, c );                "
//"   p.w = consts.w;                     "
//"   return p;                           "
//" }                                     ";

//
// simple pixel shader to apply the yuvtorgb matrix with alpha
//

//static const char StrYCrCbAToRGBA[] =
//" sampler tex0   : register( s0 );      "
//" sampler tex1   : register( s1 );      "
//" sampler tex2   : register( s2 );      "
//" sampler tex3   : register( s3 );      "
//" float4  tor    : register( c0 );      "
//" float4  tog    : register( c1 );      "
//" float4  tob    : register( c2 );      "
//" float4  consts : register( c3 );      "
//"                                       "
//" struct VS_OUT                         "
//" {                                     "
//"     float2 T0: TEXCOORD0;             "
//" };                                    "
//"                                       "
//" float4 main( VS_OUT In ) : COLOR      "
//" {                                     "
//"   float4 c;                           "
//"   float4 p;                           "
//"   c.x = tex2D( tex0, In.T0 ).x;       "
//"   c.y = tex2D( tex1, In.T0 ).x;       "
//"   c.z = tex2D( tex2, In.T0 ).x;       "
//"   c.w = consts.x;                     "
//"   p.w = tex2D( tex3, In.T0 ).x;       "
//"   p.x = dot( tor, c );                "
//"   p.y = dot( tog, c );                "
//"   p.z = dot( tob, c );                "
//"   p.w *= consts.w;                    "
//"   return p;                           "
//" }                                     ";


//
// matrix to convert yuv to rgb
// not const - we change the final value to reflect global alpha
//

static float yuvtorgb[] = 
{
   1.164123535f,  1.595794678f,  0.0f,         -0.87065506f,
   1.164123535f, -0.813476563f, -0.391448975f,  0.529705048f,
   1.164123535f,  0.0f,          2.017822266f, -1.081668854f,
   1.0f, 0.0f, 0.0f, 0.0f
};


//############################################################################
//##                                                                        ##
//## Free the shaders that we use.                                          ##
//##                                                                        ##
//############################################################################

void Free_Bink_shaders( void )
{
	if (m_YCrCbToRGBNoPixelAlpha.IsCreated())
		Display::Device()->Destroy(&m_YCrCbToRGBNoPixelAlpha);

	if (m_YCrCbAToRGBA.IsCreated())
		Display::Device()->Destroy(&m_YCrCbAToRGBA);

	if (m_PositionAndTexCoordPassThrough.IsCreated())
		Display::Device()->Destroy(&m_PositionAndTexCoordPassThrough);
}


//############################################################################
//##                                                                        ##
//## Create the three shaders that we use.                                  ##
//##                                                                        ##
//############################################################################

S32 Create_Bink_shaders( LPDIRECT3DDEVICE9 d3d_device )
{
	// Create a pixel shader that goes from YcRcB to RGB (without alpha)
	if (!m_YCrCbToRGBNoPixelAlpha.IsCreated())
	{
		if (!Display::CreatePixelShaderFromFunc("DontShip/Shaders/psBinkNoAlpha", 0, &m_YCrCbToRGBNoPixelAlpha, REF_SHADER(psBinkNoAlpha)))
		{
			Free_Bink_shaders( );
			return 0;
		}
	}

	// Create a pixel shader that goes from YcRcB to RGB with an alpha plane
	if (!m_YCrCbAToRGBA.IsCreated())
	{
		if (!Display::CreatePixelShaderFromFunc("DontShip/Shaders/psBink", 0, &m_YCrCbAToRGBA, REF_SHADER(psBink)))
		{
			Free_Bink_shaders();
			return 0;
		}
	}

	// Create a vertex shader that just passes the vertices straight through
	if (!m_PositionAndTexCoordPassThrough.IsCreated())
	{
		if (!Display::CreateVertexShaderFromFunc("DontShip/Shaders/vsBink", 0, &m_PositionAndTexCoordPassThrough, REF_SHADER(vsBink)))
		{
			Free_Bink_shaders();
			return 0;
		}
	}

	return( 1 );
}


//############################################################################
//##                                                                        ##
//## Free the textures that we allocated.                                   ##
//##                                                                        ##
//############################################################################

void Free_Bink_textures( LPDIRECT3DDEVICE9 d3d_device,
                         BINKTEXTURESET * set_textures )
{
  int i;
  BINKFRAMETEXTURES * bt;
    
  // free the texture memory and then the textures directly
  bt = set_textures->textures;
  for( i = 0 ; i < set_textures->bink_buffers.TotalFrames ; i++ )
  {
    if ( bt->Ytexture )
    {
	  BizPhysicalFree( set_textures->bink_buffers.Frames[ i ].YPlane.Buffer );
	  delete bt->Ytexture;
    }
    if ( bt->cRtexture )
    {
	  BizPhysicalFree( set_textures->bink_buffers.Frames[ i ].cRPlane.Buffer );
      delete bt->cRtexture;
    }
    if ( bt->cBtexture )
    {
	  BizPhysicalFree( set_textures->bink_buffers.Frames[ i ].cBPlane.Buffer );
      delete bt->cBtexture;
    }
    if ( bt->Atexture )
    {
	  BizPhysicalFree( set_textures->bink_buffers.Frames[ i ].APlane.Buffer );
      delete bt->Atexture;
    }
    ++bt;
  }
}


//############################################################################
//##                                                                        ##
//## Create a texture while allocating the memory ourselves.                ##
//##                                                                        ##
//############################################################################

static S32 make_texture( U32 width, U32 height, D3DFORMAT format, 
                         LPDIRECT3DTEXTURE9 * out_texture, void ** out_ptr, U32 * out_pitch, U32 * out_size )
{
  DWORD size;
  IDirect3DTexture9 * texture;
  void * ptr;
  
  texture = new IDirect3DTexture9;
  if ( texture == 0 )
    return( 0 );

  size = XGSetTextureHeader( width, height, 1, D3DUSAGE_CPU_CACHED_MEMORY, format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, texture, 0, 0 );
  
  size = ( ( size + GPU_TEXTURE_ALIGNMENT - 1 ) / GPU_TEXTURE_ALIGNMENT ) * GPU_TEXTURE_ALIGNMENT;
  
  //
  // Bink textures must be allocated in read-write cacheable memory!
  //
  
  ptr = BizPhysicalAlloc( size, MAXULONG_PTR, GPU_TEXTURE_ALIGNMENT, PAGE_READWRITE );
  if ( ptr == 0 )
  {
    delete texture;
    return( 0 );
  }
  
  XGOffsetResourceAddress( texture, ptr );
  
  *out_pitch = width;
  *out_texture = texture;
  *out_ptr = ptr;
  *out_size = size;

  //
  // use lockrect to get the d3d pitch
  //
    
  D3DLOCKED_RECT lr;

  if ( SUCCEEDED( texture->LockRect( 0, &lr, 0, 0 ) ) )
  {
    *out_pitch = lr.Pitch;
     texture->UnlockRect( 0 );
  }
  
  return( 1 );
}


//############################################################################
//##                                                                        ##
//## Create 2 sets of textures for Bink to decompress into...               ##
//##                                                                        ##
//############################################################################

S32 Create_Bink_textures( LPDIRECT3DDEVICE9 d3d_device,
                          BINKTEXTURESET * set_textures )
{
  int i;
  
  //
  // Create our textures
  //

  for( i = 0 ; i < set_textures->bink_buffers.TotalFrames ; i++ )
  {
    set_textures->textures[ i ].Ytexture = 0;
    set_textures->textures[ i ].cBtexture = 0;
    set_textures->textures[ i ].cRtexture = 0;
    set_textures->textures[ i ].Atexture = 0;

    if ( set_textures->bink_buffers.Frames[ i ].YPlane.Allocate )
    {
      // create Y plane
      if ( ! make_texture( set_textures->bink_buffers.YABufferWidth, 
                           set_textures->bink_buffers.YABufferHeight,
                           D3DFMT_LIN_L8,
                           &set_textures->textures[ i ].Ytexture, 
                           &set_textures->bink_buffers.Frames[ i ].YPlane.Buffer, 
                           &set_textures->bink_buffers.Frames[ i ].YPlane.BufferPitch, 
                           &set_textures->textures[ i ].Ysize ) )
        goto fail;
    }
                                              
    if ( set_textures->bink_buffers.Frames[ i ].cRPlane.Allocate )
    {
      // create cR plane
      if ( ! make_texture( set_textures->bink_buffers.cRcBBufferWidth, 
                           set_textures->bink_buffers.cRcBBufferHeight,
                           D3DFMT_LIN_L8,
                           &set_textures->textures[ i ].cRtexture, 
                           &set_textures->bink_buffers.Frames[ i ].cRPlane.Buffer, 
                           &set_textures->bink_buffers.Frames[ i ].cRPlane.BufferPitch, 
                           &set_textures->textures[ i ].cRsize ) )
        goto fail;
    }

    if ( set_textures->bink_buffers.Frames[ i ].cBPlane.Allocate )
    {
      // create cR plane
      if ( ! make_texture( set_textures->bink_buffers.cRcBBufferWidth, 
                           set_textures->bink_buffers.cRcBBufferHeight,
                           D3DFMT_LIN_L8,
                           &set_textures->textures[ i ].cBtexture, 
                           &set_textures->bink_buffers.Frames[ i ].cBPlane.Buffer, 
                           &set_textures->bink_buffers.Frames[ i ].cBPlane.BufferPitch, 
                           &set_textures->textures[ i ].cBsize ) )
        goto fail;
    }
    
    if ( set_textures->bink_buffers.Frames[ i ].APlane.Allocate )
    {
      // create alpha plane
      if ( ! make_texture( set_textures->bink_buffers.YABufferWidth, 
                           set_textures->bink_buffers.YABufferHeight,
                           D3DFMT_LIN_L8,
                           &set_textures->textures[ i ].Atexture, 
                           &set_textures->bink_buffers.Frames[ i ].APlane.Buffer, 
                           &set_textures->bink_buffers.Frames[ i ].APlane.BufferPitch, 
                           &set_textures->textures[ i ].Asize ) )
        goto fail;
    }  
  }
  
  return( 1 );
  
 fail:

  Free_Bink_textures( d3d_device, set_textures );
  return( 0 );
}


//############################################################################
//##                                                                        ##
//## Draw our textures onto the screen with our vertex and pixel shaders.   ##
//##                                                                        ##
//############################################################################

void Draw_Bink_textures( LPDIRECT3DDEVICE9 d3d_device,
                         BINKTEXTURESET * set_textures,
                         U32 width,
                         U32 height,
                         F32 x_offset,
                         F32 y_offset,
                         F32 x_scale,
                         F32 y_scale,
                         F32 alpha_level,
						 F32 uEnd1,
						 F32 vEnd1 )
{
  POS_TC_VERTEX vertices[ 4 ];
  int i;

  //
  // Turn on texture filtering and texture clamping
  //

  for( i = 0 ; i < 4 ; i++ )
  {
    d3d_device->SetSamplerState( i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    d3d_device->SetSamplerState( i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    d3d_device->SetSamplerState( i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    d3d_device->SetSamplerState( i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    d3d_device->SetSamplerState( i, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP );
  }

  //
  // Select the texture.
  //

  d3d_device->SetTexture( 0, set_textures->textures[ set_textures->bink_buffers.FrameNum ].Ytexture );
  d3d_device->SetTexture( 1, set_textures->textures[ set_textures->bink_buffers.FrameNum ].cRtexture );
  d3d_device->SetTexture( 2, set_textures->textures[ set_textures->bink_buffers.FrameNum ].cBtexture );

  //
  // turn off Z buffering, culling, and projection (since we are drawing orthographically)
  //
  
  d3d_device->SetRenderState( D3DRS_ZENABLE, FALSE );
  d3d_device->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
  d3d_device->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
  d3d_device->SetRenderState( D3DRS_VIEWPORTENABLE, 0 );

  
  //
  // upload the YUV to RGB matrix
  //
  
  yuvtorgb[15] = alpha_level;
  d3d_device->SetPixelShaderConstantF( 0, yuvtorgb, 4 );
  
  
  //
  // set the vertex format and turn on our vertex shader
  //
  
  d3d_device->SetFVF( POS_TC_VERTEX_FVF );
  d3d_device->SetVertexShader(m_PositionAndTexCoordPassThrough.GetGPUVertexShader());

  
  //
  // are we using an alpha plane? if so, turn on the 4th texture and set our pixel shader
  //
  
  if ( set_textures->textures[ set_textures->bink_buffers.FrameNum ].Atexture )
  {
    //
    // set the alpha texture
    //
    
    d3d_device->SetTexture( 3, set_textures->textures[ set_textures->bink_buffers.FrameNum ].Atexture );

    //
    // turn on our pixel shader
    //

	d3d_device->SetPixelShader(m_YCrCbAToRGBA.GetGPUPixelShader());

    goto do_alpha;
  }
  else
  {
    //
    // turn on our pixel shader
    //

	  d3d_device->SetPixelShader(m_YCrCbToRGBNoPixelAlpha.GetGPUPixelShader());
  }

  //
  // are we completely opaque or somewhat transparent?
  //
  
  if ( alpha_level >= 0.999f )
  {
    d3d_device->SetRenderState( D3DRS_ALPHABLENDENABLE, 0 );
  }
  else
  {
   do_alpha:
    d3d_device->SetRenderState( D3DRS_ALPHABLENDENABLE, 1 );
    d3d_device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    d3d_device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  }
  
  //
  // Setup up the vertices.
  //

  vertices[ 0 ].sx = x_offset;
  vertices[ 0 ].sy = y_offset;
  vertices[ 0 ].sz = 0.0F;
  vertices[ 0 ].tu0 = 0.f;
  vertices[ 0 ].tv0 = 0.f;
  vertices[ 0 ].tu1 = 0.f;
  vertices[ 0 ].tv1 = 0.f;
  vertices[ 1 ] = vertices[ 0 ];
  vertices[ 1 ].sx = x_offset + ( ( (F32)(S32) width ) * x_scale );
  vertices[ 1 ].tu0 = 1.f;
  vertices[ 1 ].tu1 = uEnd1;
  vertices[ 2 ] = vertices[0];
  vertices[ 2 ].sy = y_offset + ( ( (F32)(S32) height ) * y_scale );
  vertices[ 2 ].tv0 = 1.f;
  vertices[ 2 ].tv1 = vEnd1;
  vertices[ 3 ] = vertices[ 1 ];
  vertices[ 3 ].sy = vertices[ 2 ].sy;
  vertices[ 3 ].tv0 = 1.f;
  vertices[ 3 ].tv1 = vEnd1;

  //
  // Draw the vertices.
  //

  d3d_device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, vertices, sizeof( vertices[ 0 ] ) );

  //
  // clear the textures
  //
  
  d3d_device->SetTexture( 0, 0 );
  d3d_device->SetTexture( 1, 0 );
  d3d_device->SetTexture( 2, 0 );

  if ( set_textures->textures[ set_textures->bink_buffers.FrameNum ].Atexture )
  {
    d3d_device->SetTexture( 3, 0 );
  }

  d3d_device->SetRenderState( D3DRS_VIEWPORTENABLE, 1 );
}


//
// simple routine to store the cache of a memory range
//

#include <ppcintrinsics.h>

static void store_cache( void * ptr, U32 size )
{
  U32 i;
  
  for ( i = 0 ; i < size ; i += 128 )
  {
    __dcbst( 0, ptr );
    ptr = ( (U8*) ptr ) + 128;
  }
}
    

// sync the textures out to main memory, so that the GPU can see them (this is fast)
RADDEFFUNC void Sync_Bink_textures( BINKTEXTURESET * set_textures )
{
  // store out the cache
  int i;

  i = set_textures->bink_buffers.FrameNum;
  if ( set_textures->bink_buffers.Frames[ i ].YPlane.Buffer )
	store_cache( set_textures->bink_buffers.Frames[ i ].YPlane.Buffer, set_textures->textures[ i ].Ysize );
  if ( set_textures->bink_buffers.Frames[ i ].cRPlane.Buffer )
	store_cache( set_textures->bink_buffers.Frames[ i ].cRPlane.Buffer, set_textures->textures[ i ].cRsize );
  if ( set_textures->bink_buffers.Frames[ i ].cBPlane.Buffer )
	store_cache( set_textures->bink_buffers.Frames[ i ].cBPlane.Buffer, set_textures->textures[ i ].cBsize );
  if ( set_textures->bink_buffers.Frames[ i ].APlane.Buffer )
	store_cache( set_textures->bink_buffers.Frames[ i ].APlane.Buffer, set_textures->textures[ i ].Asize );
}


// make sure the GPU is done with the textures that we are about to write info
RADDEFFUNC void Wait_for_Bink_textures( BINKTEXTURESET * set_textures )
{
  S32 next;
  
  next = set_textures->bink_buffers.FrameNum + 1;
  if ( ( next >= set_textures->bink_buffers.TotalFrames ) || ( set_textures->textures[ next ].Ytexture == 0 ) )
    next = 0;

  if ( set_textures->textures[ next ].Ytexture )
  {
    //
    // block until the texture is ready
    //

    set_textures->textures[ next ].Ytexture->BlockUntilNotBusy();
  }
}
