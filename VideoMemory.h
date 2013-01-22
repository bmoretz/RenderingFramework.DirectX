#pragma once

#include "DirectDrawUtility.h"

using namespace System;

namespace DirectDrawWrapper
{
	#pragma managed

	public enum PixelSize { Real = 1, High = 2, True = 4 };

	public ref class VideoMemory
	{
		public:

			VideoMemory( IntPtr );

			void CreateDevice();
			void ResetDevice();
			void Release();

			void SetBackgroundColor( unsigned int );
			void SetColorFormat( PixelSize );

			bool ReadyFrame( bool );
			void RenderFrame();

			IntPtr VideoMemoryPtr;
			int Width, Height, Stride;

		protected:

			~VideoMemory();
			!VideoMemory();

		private:
			DirectDrawWrapper * pDDW;
	};
}