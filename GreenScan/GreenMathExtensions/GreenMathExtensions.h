// GreenMathExtensions.h

#pragma once

using namespace System;
using namespace System::Runtime::CompilerServices;

namespace Green
{
	namespace MathExtensions
	{
		[ExtensionAttribute]
		public ref class Extensions abstract sealed {
			public:        
				[ExtensionAttribute]
				static double ToDegrees(double angle) {
					return angle / Math::PI * 180.0;
				}

				static double ToRadians(double angle) {
					return angle / 180.0 * Math::PI;
				}
		};

		public static ref class Extensions
		{
			public static double ToDegrees(this double angle)
			{
				return angle / Math.PI * 180d;
			}

			public static double ToRadians(this double angle)
			{
				return angle / 180d * Math.PI;
			}
		}

		public ref class Class1
		{
			// TODO: Add your methods for this class here.
		};

	}
}
