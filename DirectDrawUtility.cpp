#include "DirectDrawUtility.h"

namespace DirectDrawWrapper
{
	DirectDrawWrapper::DirectDrawWrapper( void ) {}

	DirectDrawWrapper::~DirectDrawWrapper( void ) { }

	void DirectDrawWrapper::Initialize( HWND hwnd )
	{
		hWnd = hwnd;

		HRESULT hResult;

		hResult = DirectDrawCreateEx
		(
			NULL,
			( VOID **) &lpDirectDraw,
			IID_IDirectDraw7,
			NULL
		);

		if( hResult != DD_OK )
			return;

		hResult = lpDirectDraw->SetCooperativeLevel
		(
			hWnd,
			DDSCL_NORMAL
		);

		if( hResult != DD_OK )
			return;
	}

	void DirectDrawWrapper::CreateDevice()
	{
		HRESULT hResult;

		DDSURFACEDESC2 ddsd;

		DDRAW_INIT_STRUCT( ddsd );

		ddsd.dwFlags = DDSD_CAPS;

		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    
		hResult = lpDirectDraw->CreateSurface( &ddsd, &lpDDSPrimary, NULL );

		if( hResult != DD_OK )
		{
			DisplayError( hResult, TEXT( "Error Creating Primary Surface" ) );
			return;
		}

		hResult = lpDirectDraw->CreateClipper( 0, &lpDDClipper, NULL );

		if( hResult != DD_OK )
		{
			DisplayError( hResult, TEXT( "Error Creating Primary Surface Clipper" ) );
			return;
		}

		hResult = lpDDClipper->SetHWnd( 0, hWnd );

		if( hResult != DD_OK )
		{
			DisplayError( hResult, TEXT( "Error Setting Primary Surface Clipper Window" ) );
			return;
		}

		hResult = lpDDSPrimary->SetClipper( lpDDClipper );

		if( hResult != DD_OK )
		{
			DisplayError( hResult, TEXT( "Error Attaching Primary Surface" ) );
			return;
		}

		CreateBackBuffer();
	}

	void DirectDrawWrapper::CreateBackBuffer()
	{
		ZeroMemory( &clientArea, sizeof( clientArea ) );
		GetClientRect( hWnd, &clientArea );

		Width = clientArea.right - clientArea.left;
		Height = clientArea.bottom - clientArea.top;

		if( Width <= 0 || Height <= 0 )
		{
			lpDDSBack = NULL;
			return;
		}

		DDSURFACEDESC2 ddsd;

		DDRAW_INIT_STRUCT( ddsd );

		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
		ddsd.dwWidth = Width;
		ddsd.dwHeight = Height;
		
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

		switch( PixelSize )
		{
			case 1:
				{
					InitializeRealColorSurface( &ddsd );
				} break;

			case 2:
				{
					InitializeHiColorSurface( &ddsd );
				} break;

			case 4:
				{
					InitializeTrueColorSurface( &ddsd );
				} break;
		}

		HRESULT hResult = lpDirectDraw->CreateSurface( &ddsd, &lpDDSBack, NULL );

		if( hResult != DD_OK )
		{
			DisplayError( hResult, TEXT( "Error Creating Secondary Surface" ) );
			return;
		}

		if( PixelSize == 1 )
		{
			hResult = lpDDSBack->SetPalette( lpDDPalette );

			if( hResult != DD_OK )
			{
				DisplayError( hResult, TEXT( "Error Attaching Palette" ) );
			}
		}
	}

	void DirectDrawWrapper::ResetDevice()
	{
		SafeRelease( lpDDSBack );

		CreateBackBuffer();
	}

	bool DirectDrawWrapper::Lock( bool clearFrame )
	{
		if( lpDDSBack == NULL )
			return false;

		DDSURFACEDESC2 LockedSurface;

		DDRAW_INIT_STRUCT( LockedSurface );

		HRESULT hResult;

		if( clearFrame )
		{
			DDBLTFX ddbltfx;
			RECT fillArea;
 
			memset( &ddbltfx, 0, sizeof( DDBLTFX ) );
			ddbltfx.dwSize = sizeof( DDBLTFX );

			ddbltfx.dwFillColor = BackgroundColor;
 
			fillArea.top = 0; fillArea.left = 0;
			fillArea.right = Width; fillArea.bottom = Height;

			hResult = lpDDSBack->Blt( &fillArea, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx );

			if( hResult != DD_OK )
				DisplayError( hResult, TEXT( "Error Clearing Secondary Surface" ) );
		}

		hResult = lpDDSBack->Lock( NULL, &LockedSurface, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_NOSYSLOCK, NULL );

		if( hResult == DD_OK )
		{
			VideoMemory = ( unsigned int * ) LockedSurface.lpSurface;

			Stride = LockedSurface.lPitch >> PixelSize;

			return true;
		}
		else if( hResult == DDERR_SURFACELOST )
		{
			lpDDSBack->Restore();
		}

		return false;
	}

	void DirectDrawWrapper::Render( void )
	{
		lpDDSBack->Unlock( NULL );

		ZeroMemory( &clientArea, sizeof( clientArea ) );
		GetClientRect( hWnd, &clientArea );
		
		p1.x = clientArea.left; p1.y = clientArea.top; 
		p2.x = clientArea.right; p2.y = clientArea.bottom;

		ClientToScreen( hWnd, &p1 );
		ClientToScreen( hWnd, &p2 );

		clientArea.left = p1.x; clientArea.top = p1.y;
		clientArea.right = p2.x; clientArea.bottom = p2.y;

		if( lpDDSPrimary->Blt( &clientArea, lpDDSBack, NULL, DDBLT_WAIT, NULL ) != DD_OK )
			throw;
	}


	void DirectDrawWrapper::InitializeRealColorSurface( LPDDSURFACEDESC2 ddsd )
	{
		ddsd->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;

		ddsd->ddpfPixelFormat.dwSize = sizeof( DDPIXELFORMAT );

		ddsd->ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
		ddsd->ddpfPixelFormat.dwRGBBitCount = 8;

		static bool paletteInitialized = false;

		if( !paletteInitialized )
		{
			int BlueValues[] = { 0, 85, 170, 255 };
			int RedValues[] = { 0, 36, 73, 108, 144, 181, 216, 255 };
			int GreenValues[] = { 0, 36, 73, 108, 144, 181, 216, 255 };
			
			int colorMapIndex = 0;

			for( int redIndex = 0; redIndex < 8; redIndex++ )
			{
				for( int greenIndex = 0; greenIndex < 8; greenIndex++ )
				{
					for( int blueIndex = 0; blueIndex < 4; blueIndex++ )
					{
						paletteEntries[ colorMapIndex ].peRed = RedValues[ redIndex ];
						paletteEntries[ colorMapIndex ].peGreen = RedValues[ greenIndex ];
						paletteEntries[ colorMapIndex ].peBlue = RedValues[ blueIndex ];

						colorMapIndex++;
					}
				}
			}

			HRESULT hResult = lpDirectDraw->CreatePalette( DDPCAPS_8BIT | DDPCAPS_ALLOW256 | DDPCAPS_INITIALIZE, paletteEntries, &lpDDPalette, NULL );

			if( hResult != DD_OK )
			{
				DisplayError( hResult, TEXT( "Error Creating Palette" ) );
			}

			paletteInitialized = true;
		}
	}

	void DirectDrawWrapper::InitializeHiColorSurface( LPDDSURFACEDESC2 ddsd )
	{
		ddsd->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;

		ddsd->ddpfPixelFormat.dwSize = sizeof( DDPIXELFORMAT );

		ddsd->ddpfPixelFormat.dwFlags = DDPF_RGB;
		ddsd->ddpfPixelFormat.dwRGBBitCount = 16;

		ddsd->ddpfPixelFormat.dwRBitMask = 0xf800;
		ddsd->ddpfPixelFormat.dwGBitMask = 0x07e0;
		ddsd->ddpfPixelFormat.dwBBitMask = 0x001f;
	}

	void DirectDrawWrapper::InitializeTrueColorSurface( LPDDSURFACEDESC2 ddsd )
	{
		ddsd->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;

		ddsd->ddpfPixelFormat.dwSize = sizeof( DDPIXELFORMAT );

		ddsd->ddpfPixelFormat.dwFlags = DDPF_RGB;
		ddsd->ddpfPixelFormat.dwRGBBitCount = 32;

		ddsd->ddpfPixelFormat.dwRBitMask = 0x00ff0000;
		ddsd->ddpfPixelFormat.dwGBitMask = 0x0000ff00;
		ddsd->ddpfPixelFormat.dwBBitMask = 0x000000ff;
		ddsd->ddpfPixelFormat.dwRGBAlphaBitMask = 0xff000000;
	}

	void DirectDrawWrapper::Release( void )
	{
		SafeRelease( lpDDPalette );
		SafeRelease( lpDDClipper );
		SafeRelease( lpDDSBack );
		SafeRelease( lpDDSPrimary );
		SafeRelease( lpDirectDraw );
	}

	void DirectDrawWrapper::DisplayError( HRESULT hResult, LPCTSTR szOperation )
	{
		LPCTSTR error;

		switch( hResult )
		{
			case DDERR_ALREADYINITIALIZED:
				{
					error = TEXT( "DDERR_ALREADYINITIALIZED" );
				} break;

			case DDERR_CANNOTATTACHSURFACE:
				{
					error = TEXT( "DDERR_CANNOTATTACHSURFACE" );
				} break;

			case DDERR_CANNOTDETACHSURFACE:
				{
					error = TEXT( "DDERR_CANNOTDETACHSURFACE" ); 
				} break;

			case DDERR_CURRENTLYNOTAVAIL:
				{
					error = TEXT( "DDERR_CURRENTLYNOTAVAIL" ); 
				} break;

			case DDERR_EXCEPTION:
				{
					error = TEXT( "DDERR_EXCEPTION" ); 
				} break;

			case DDERR_GENERIC:
				{
					error = TEXT( "DDERR_GENERIC" );
				} break;
			case DDERR_HEIGHTALIGN:
				{
					error = TEXT( "DDERR_HEIGHTALIGN" );
				} break;

			case DDERR_INCOMPATIBLEPRIMARY:
				{
					error = TEXT( "DDERR_INCOMPATIBLEPRIMARY" );
				} break;

			case DDERR_INVALIDCAPS:
				{
					error = TEXT( "DDERR_INVALIDCAPS" );
				} break;

			case DDERR_INVALIDCLIPLIST:
				{
					error = TEXT( "DDERR_INVALIDCLIPLIST" );
				} break;

			case DDERR_INVALIDMODE:
				{
					error = TEXT( "DDERR_INVALIDMODE" );
				} break;

			case DDERR_INVALIDOBJECT:
				{
					error = TEXT( "DDERR_INVALIDOBJECT" );
				} break;

			case DDERR_INVALIDPARAMS:
				{
					error = TEXT( "DDERR_INVALIDPARAMS" );
				} break;

			case DDERR_INVALIDPIXELFORMAT:
				{
					error = TEXT( "DDERR_INVALIDPIXELFORMAT" );
				} break;

			case DDERR_INVALIDRECT:
				{
					error = TEXT( "DDERR_INVALIDRECT" );
				} break;

			case DDERR_LOCKEDSURFACES:
				{
					error = TEXT( "DDERR_LOCKEDSURFACES" );
				} break;

			case DDERR_NO3D:
				{
					error = TEXT( "DDERR_NO3D" );
				} break;

			case DDERR_NOALPHAHW:
				{
					error = TEXT( "DDERR_NOALPHAHW" ); 
				} break;

			case DDERR_NOCLIPLIST:
				{
					error = TEXT( "DDERR_NOCLIPLIST" );
				} break;

			case DDERR_NOCOLORCONVHW:
				{
					error = TEXT( "DDERR_NOCOLORCONVHW" ); 
				} break;

			case DDERR_NOCOOPERATIVELEVELSET:
				{
					error = TEXT("DDERR_NOCOOPERATIVELEVELSET");
				} break;

			case DDERR_NOCOLORKEY:
				{
					error = TEXT( "DDERR_NOCOLORKEY" );
				} break;

			case DDERR_NOCOLORKEYHW:
				{
					error = TEXT( "DDERR_NOCOLORKEYHW" );
				} break;

			case DDERR_NODIRECTDRAWSUPPORT:
				{
					error = TEXT( "DDERR_NODIRECTDRAWSUPPORT" );
				} break;

			case DDERR_NOEXCLUSIVEMODE:
				{
					error = TEXT( "DDERR_NOEXCLUSIVEMODE" );
				} break;

			case DDERR_NOFLIPHW:
				{
					error = TEXT("DDERR_NOFLIPHW");
				} break;

			case DDERR_NOGDI:
				{
					error = TEXT( "DDERR_NOGDI" );
				} break;

			case DDERR_NOMIRRORHW:
				{
					error = TEXT( "DDERR_NOMIRRORHW" );
				} break;

			case DDERR_NOTFOUND:
				{
					error = TEXT( "DDERR_NOTFOUND" );
				} break;

			case DDERR_NOOVERLAYHW:
				{
					error = TEXT( "DDERR_NOOVERLAYHW" );
				} break;

			case DDERR_NORASTEROPHW:
				{
					error = TEXT( "DDERR_NORASTEROPHW" );
				} break;

			case DDERR_NOROTATIONHW:
				{
					error = TEXT( "DDERR_NOROTATIONHW" );
				} break;

			case DDERR_NOSTRETCHHW:
				{
					error = TEXT( "DDERR_NOSTRETCHHW" );
				} break;

			case DDERR_NOT4BITCOLOR:
				{
					error = TEXT( "DDERR_NOT4BITCOLOR" );
				} break;

			case DDERR_NOT4BITCOLORINDEX:
				{
					error = TEXT( "DDERR_NOT4BITCOLORINDEX" );
				} break;

			case DDERR_NOT8BITCOLOR:
				{
					error = TEXT( "DDERR_NOT8BITCOLOR" );
				} break;

			case DDERR_NOTEXTUREHW:
				{
					error = TEXT( "DDERR_NOTEXTUREHW" );
				} break;

			case DDERR_NOVSYNCHW:
				{
					error = TEXT( "DDERR_NOVSYNCHW" );
				} break;

			case DDERR_NOZBUFFERHW:
				{
					error = TEXT( "DDERR_NOZBUFFERHW" );
				} break;

			case DDERR_NOZOVERLAYHW:
				{
					error = TEXT( "DDERR_NOZOVERLAYHW" );
				} break;

			case DDERR_OUTOFCAPS:
				{
					error = TEXT( "DDERR_OUTOFCAPS" );
				} break;

			case DDERR_OUTOFMEMORY:
				{
					error = TEXT( "DDERR_OUTOFMEMORY" );
				} break;

			case DDERR_OUTOFVIDEOMEMORY:
				{
					error = TEXT( "DDERR_OUTOFVIDEOMEMORY" );
				} break;

			case DDERR_OVERLAYCANTCLIP:
				{
					error = TEXT( "DDERR_OVERLAYCANTCLIP" );
				} break;

			case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
				{
					error = TEXT( "DDERR_OVERLAYCOLORKEYONLYONEACTIVE" );
				} break;

			case DDERR_PALETTEBUSY:
				{
					error = TEXT( "DDERR_PALETTEBUSY" );
				} break;

			case DDERR_COLORKEYNOTSET:
				{
					error = TEXT( "DDERR_COLORKEYNOTSET" );
				} break;

			case DDERR_SURFACEALREADYATTACHED:
				{
					error = TEXT( "DDERR_SURFACEALREADYATTACHED" );
				} break;

			case DDERR_SURFACEALREADYDEPENDENT:
				{
					error = TEXT( "DDERR_SURFACEALREADYDEPENDENT" );
				} break;

			case DDERR_SURFACEBUSY:
				{
					error = TEXT( "DDERR_SURFACEBUSY" );
				} break;

			case DDERR_CANTLOCKSURFACE:
				{
					error = TEXT( "DDERR_CANTLOCKSURFACE" );
				} break;

			case DDERR_SURFACEISOBSCURED:
				{
					error = TEXT( "DDERR_SURFACEISOBSCURED" );
				} break;

			case DDERR_SURFACELOST:
				{
					error = TEXT( "DDERR_SURFACELOST" );
				} break;

			case DDERR_SURFACENOTATTACHED:
				{
					error = TEXT( "DDERR_SURFACENOTATTACHED" );
				} break;

			case DDERR_TOOBIGHEIGHT:
				{
					error = TEXT( "DDERR_TOOBIGHEIGHT" );
				} break;

			case DDERR_TOOBIGSIZE:
				{
					error = TEXT( "DDERR_TOOBIGSIZE" );
				} break;

			case DDERR_TOOBIGWIDTH:
				{
					error = TEXT( "DDERR_TOOBIGWIDTH" );
				} break;

			case DDERR_UNSUPPORTED:
				{
					error = TEXT( "DDERR_UNSUPPORTED" );
				} break;

			case DDERR_UNSUPPORTEDFORMAT:
				{
					error = TEXT( "DDERR_UNSUPPORTEDFORMAT" );
				} break;

			case DDERR_UNSUPPORTEDMASK:
				{
					error = TEXT( "DDERR_UNSUPPORTEDMASK" );
				} break;

			case DDERR_VERTICALBLANKINPROGRESS:
				{
					error = TEXT( "DDERR_VERTICALBLANKINPROGRESS" );
				} break;

			case DDERR_WASSTILLDRAWING:
				{
					error = TEXT( "DDERR_WASSTILLDRAWING" );
				} break;

			case DDERR_XALIGN:
				{
					error = TEXT( "DDERR_XALIGN" );
				} break;

			case DDERR_INVALIDDIRECTDRAWGUID:
				{
					error = TEXT( "DDERR_INVALIDDIRECTDRAWGUID" );
				} break;

			case DDERR_DIRECTDRAWALREADYCREATED:
				{
					error = TEXT( "DDERR_DIRECTDRAWALREADYCREATED" );
				} break;

			case DDERR_NODIRECTDRAWHW:
				{
					error = TEXT( "DDERR_NODIRECTDRAWHW" );
				} break;

			case DDERR_PRIMARYSURFACEALREADYEXISTS:
				{
					error = TEXT( "DDERR_PRIMARYSURFACEALREADYEXISTS" );
				} break;

			case DDERR_NOEMULATION:
				{
					error = TEXT( "DDERR_NOEMULATION" );
				} break;

			case DDERR_REGIONTOOSMALL:
				{
					error = TEXT( "DDERR_REGIONTOOSMALL" );
				} break;

			case DDERR_CLIPPERISUSINGHWND:
				{
					error = TEXT( "DDERR_CLIPPERISUSINGHWND" );
				} break;

			case DDERR_NOCLIPPERATTACHED:
				{
					error = TEXT( "DDERR_NOCLIPPERATTACHED" );
				} break;

			case DDERR_NOHWND:
				{
					error = TEXT( "DDERR_NOHWND" );
				} break;

			case DDERR_HWNDSUBCLASSED:
				{
					error = TEXT( "DDERR_HWNDSUBCLASSED" );
				} break;

			case DDERR_HWNDALREADYSET:
				{
					error = TEXT( "DDERR_HWNDALREADYSET" );
				} break;

			case DDERR_NOPALETTEATTACHED:
				{
					error = TEXT( "DDERR_NOPALETTEATTACHED" );
				} break;

			case DDERR_NOPALETTEHW:
				{
					error = TEXT( "DDERR_NOPALETTEHW" );
				} break;

			case DDERR_BLTFASTCANTCLIP:
				{
					error = TEXT( "DDERR_BLTFASTCANTCLIP" );
				} break;

			case DDERR_NOBLTHW:
				{
					error = TEXT( "DDERR_NOBLTHW" );
				} break;

			case DDERR_NODDROPSHW:
				{
					error = TEXT( "DDERR_NODDROPSHW" );
				} break;

			case DDERR_OVERLAYNOTVISIBLE:
				{
					error = TEXT( "DDERR_OVERLAYNOTVISIBLE" );
				} break;

			case DDERR_NOOVERLAYDEST:
				{
					error = TEXT( "DDERR_NOOVERLAYDEST" );
				} break;

			case DDERR_INVALIDPOSITION:
				{
					error = TEXT( "DDERR_INVALIDPOSITION" );
				} break;

			case DDERR_NOTAOVERLAYSURFACE:
				{
					error = TEXT( "DDERR_NOTAOVERLAYSURFACE" );
				} break;

			case DDERR_EXCLUSIVEMODEALREADYSET:
				{
					error = TEXT( "DDERR_EXCLUSIVEMODEALREADYSET" );
				} break;

			case DDERR_NOTFLIPPABLE:
				{
					 error = TEXT( "DDERR_NOTFLIPPABLE" );
				} break;

			case DDERR_CANTDUPLICATE:
				{
					error = TEXT( "DDERR_CANTDUPLICATE" );
				} break;

			case DDERR_NOTLOCKED:
				{
					error = TEXT( "DDERR_NOTLOCKED" );
				} break;

			case DDERR_CANTCREATEDC:
				{
					error = TEXT( "DDERR_CANTCREATEDC" );
				} break;

			case DDERR_NODC:
				{
					error = TEXT( "DDERR_NODC" );
				} break;

			case DDERR_WRONGMODE:
				{
					error = TEXT( "DDERR_WRONGMODE" );
				} break;

			case DDERR_IMPLICITLYCREATED:
				{
					error = TEXT( "DDERR_IMPLICITLYCREATED" );
				} break;

			case DDERR_NOTPALETTIZED:
				{
					error = TEXT( "DDERR_NOTPALETTIZED" );
				} break;

			case DDERR_UNSUPPORTEDMODE:
				{
					error = TEXT( "DDERR_UNSUPPORTEDMODE" );
				} break;

			case DDERR_NOMIPMAPHW:
				{
					error = TEXT( "DDERR_NOMIPMAPHW" );
				} break;

			case DDERR_INVALIDSURFACETYPE:
				{
					error = TEXT( "DDERR_INVALIDSURFACETYPE" );
				} break;

			case DDERR_DCALREADYCREATED:
				{
					error = TEXT( "DDERR_DCALREADYCREATED" );
				} break;

			case DDERR_CANTPAGELOCK:
				{
					error = TEXT( "DDERR_CANTPAGELOCK" );
				} break;

			case DDERR_CANTPAGEUNLOCK:
				{
					error = TEXT( "DDERR_CANTPAGEUNLOCK" );
				} break;

			case DDERR_NOTPAGELOCKED:
				{
					error = TEXT( "DDERR_NOTPAGELOCKED" ); 
				} break;

			case DDERR_NOTINITIALIZED:
				{
					error = TEXT( "DDERR_NOTINITIALIZED" );
				} break;

			default:
				{
					error = TEXT( "Unknown Error" ); 
				} break;
		}

		MessageBox( NULL, error, szOperation, 0 );
	}
}