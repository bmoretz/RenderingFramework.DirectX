#pragma once

#include <Windows.h>
#include <ddraw.h>

#define WIN32_LEAN_AND_MEAN

#define DDRAW_INIT_STRUCT( dds ) { ZeroMemory( &dds, sizeof( dds ) ); dds.dwSize = sizeof( dds ); }
#define SafeRelease( x ) if ( x ) { x->Release(); x = NULL; }

namespace DirectDrawWrapper
{
	#pragma unmanaged

	class DirectDrawWrapper
	{
		public:

			DirectDrawWrapper( void );
			~DirectDrawWrapper( void );
			
			void Initialize( HWND );
			void Release( void );

			void CreateDevice();
			void ResetDevice();

			bool Lock( bool );
			void Render();

			int Width, Height, Stride;
			unsigned int BackgroundColor;

			void * VideoMemory;

			char PixelSize;

		private:

			void CreateBackBuffer();
			void DisplayError( HRESULT, LPCTSTR );

			void InitializeRealColorSurface( LPDDSURFACEDESC2 ddsd );
			void InitializeHiColorSurface( LPDDSURFACEDESC2 ddsd );
			void InitializeTrueColorSurface( LPDDSURFACEDESC2 ddsd );

			HWND hWnd;
			RECT clientArea;
			POINT p1, p2;

			LPDIRECTDRAW7 lpDirectDraw;
			LPDIRECTDRAWSURFACE7 lpDDSPrimary;
			LPDIRECTDRAWSURFACE7 lpDDSBack;
			LPDIRECTDRAWCLIPPER lpDDClipper;
			LPDIRECTDRAWPALETTE lpDDPalette;

			PALETTEENTRY paletteEntries[ 256 ];
	};
}