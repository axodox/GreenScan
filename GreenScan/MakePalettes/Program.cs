using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;

namespace MakePalettes
{
    class Program
    {
        struct RGBA
        {
            byte R, G, B, A;
            
            public RGBA(byte r, byte g, byte b, byte a)
            {
                R = r;
                G = g;
                B = b;
                A = a;
            }

            static byte Scale(double x)
            {
                return (byte)Math.Round(x * 255d);
            }

            public RGBA(double r, double g, double b, double a)
            {
                R = Scale(r);
                G = Scale(g);
                B = Scale(b);
                A = Scale(a);
            }

            public RGBA(double i)
            {
                R = G = B = Scale(i);
                A = 255;
            }
        }

        static RGBA HSLToRGB(double h, double s, double l, double a)
        {
            double v;
            double r = 1d, g = 1d, b = 1d;
            if (h >= 1) h = 0.999999;
            v = (l <= 0.5) ? (l * (1d + s)) : (l + s - l * s);
            if (v >= 0d)
            {
                double m;
                double sv;
                double fract, vsf, mid1, mid2;
                m = l + l - v;
                sv = (v - m) / v;
                h *= 6d;
                fract = h - Math.Truncate(h);
                vsf = v * sv * fract;
                mid1 = m + vsf;
                mid2 = v - vsf;
                if (h < 1d)
                {
                    r = v;
                    g = mid1;
                    b = m;
                }
                else if (h < 2d)
                {
                    r = mid2;
                    g = v;
                    b = m;
                }
                else if (h < 3d)
                {
                    r = m;
                    g = v;
                    b = mid1;
                }
                else if (h < 4d)
                {
                    r = m;
                    g = mid2;
                    b = v;
                }
                else if (h < 5d)
                {
                    r = mid1;
                    g = m;
                    b = v;
                }
                else
                {
                    r = v;
                    g = m;
                    b = mid2;
                }
            }
            return new RGBA(r, g, b, a);
        }

        delegate RGBA ScaleFunc(double x, double y);

        static void MakeScaleImage(string path, int width, int height, ScaleFunc func)
        {
            Bitmap bitmap = new Bitmap(width, height, PixelFormat.Format32bppArgb);
            Rectangle lockRect = new Rectangle(0, 0, width, height);
            BitmapData bitmapData = bitmap.LockBits(lockRect, ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
            double divWidth = width - 1;
            double divHeight = height - 1;
            unsafe
            {
                RGBA* pData;
                for (int row = 0; row < height; row++)
                {
                    pData = (RGBA*)(bitmapData.Scan0 + bitmapData.Stride * row);
                    for (int col = 0; col < width; col++)
                    {
                        *pData = func(col / divWidth, row / divHeight);
                        pData++;
                    }
                }
            }
            bitmap.UnlockBits(bitmapData);
            bitmap.Save(path);
            bitmap.Dispose();
        }        

        static RGBA SinFunc(double x, double y)
        {
            return new RGBA(Math.Sin(x * Math.PI * 2d) / 2d + 0.5d);
        }

        static RGBA HueFunc(double x, double y)
        {
            return HSLToRGB(x, 1d, 0.5d, 1d);
        }

        static RGBA HSLFunc(double x, double y)
        {
            return HSLToRGB(x, 1d, y, 1d);
        }

        static void Main(string[] args)
        {
            MakeScaleImage("sinMap.png", 512, 1, SinFunc);
            MakeScaleImage("hueMap.png", 512, 1, HueFunc);
            MakeScaleImage("hslMap.png", 512, 256, HSLFunc);                
        }
    }
}
