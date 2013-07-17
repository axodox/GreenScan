using System;

namespace Green.MathExtensions
{
    public static class Extensions
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

    public struct Vector3
    {
        public double X, Y, Z;

        public Vector3(double x, double y, double z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        public static Vector3 operator +(Vector3 a, Vector3 b)
        {
            return new Vector3() { X = a.X + b.X, Y = a.Y + b.Y, Z = a.Z + b.Z };
        }

        public static Vector3 operator -(Vector3 a, Vector3 b)
        {
            return new Vector3() { X = a.X - b.X, Y = a.Y - b.Y, Z = a.Z - b.Z };
        }

        public static Vector3 operator *(Vector3 a, Vector3 b)
        {
            return new Vector3() { X = a.Y * b.Z - a.Z * b.Y, Y = a.Z * b.X - a.X * b.Z, Z = a.X * b.Y - a.Y * b.X };
        }

        public static Vector3 operator *(Vector3 a, double b)
        {
            return new Vector3() { X = a.X * b, Y = a.Y * b, Z = a.Z * b };
        }

        public static Vector3 operator *(double b, Vector3 a)
        {
            return a * b;
        }

        public static Vector3 operator /(Vector3 a, double b)
        {
            return new Vector3() { X = a.X / b, Y = a.Y / b, Z = a.Z / b };
        }

        public static Vector3 operator /(double a, Vector3 b)
        {
            return new Vector3() { X = a / b.X, Y = a / b.Y, Z = a / b.Z };
        }

        public static double operator %(Vector3 a, Vector3 b)
        {
            return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
        }

        public static bool operator ==(Vector3 a, Vector3 b)
        {
            return a.X == b.X && a.Y == b.Y && a.Z == b.Z;
        }

        public static bool operator !=(Vector3 a, Vector3 b)
        {
            return a.X != b.X || a.Y != b.Y || a.Z != b.Z;
        }

        public static Vector3 operator -(Vector3 a)
        {
            return new Vector3() { X = -a.X, Y = -a.Y, Z = -a.Z };
        }

        public double Length
        {
            get
            {
                return Math.Sqrt(X * X + Y * Y + Z * Z);
            }
        }

        public void Normalize()
        {
            double length = Length;
            if (length != 0d)
            {
                X /= length;
                Y /= length;
                Z /= length;
            }
        }

        public Vector3 Direction
        {
            get
            {
                double length = Length;
                if (length == 0d)
                    return Vector3.Zero;
                else
                    return new Vector3(X / length, Y / length, Z / length);
            }
        }

        public static double AngleOf(Vector3 a, Vector3 b)
        {
            return Math.Asin((a * b).Length / (a.Length * b.Length));
        }

        public static Vector3 Interpolate(Vector3 a, Vector3 b, double d)
        {
            return a * (1 - d) + b * d;
        }

        public static Vector3 DistanceBetweenLines(Vector3 p1, Vector3 v1, Vector3 p2, Vector3 v2)
        {
            Vector3 b = (v1 * v2).Direction;
            Vector3 c = b * ((p2 - p1) % b);
            return c;
        }

        public static double VectorPlaneAgnle(Vector3 p1, Vector3 v1, Vector3 p2, Vector3 v2)
        {
            return Math.PI / 2d - AngleOf((p2 - p1) * v1, v2);
        }

        public override string ToString()
        {
            return String.Format("{0}, {1}, {2}", new object[] { X, Y, Z });
        }

        public static readonly Vector3 Zero = new Vector3();
        public static readonly Vector3 UnitX = new Vector3() { X = 1d };
        public static readonly Vector3 UnitY = new Vector3() { Y = 1d };
        public static readonly Vector3 UnitZ = new Vector3() { Z = 1d };
        public bool IsZero
        {
            get
            {
                return X == 0d && Y == 0d && Z == 0d;
            }
        }
    }

    public struct Vector4
    {
        public double X, Y, Z, W;

        public Vector4(Vector3 v)
        {
            X = v.X;
            Y = v.Y;
            Z = v.Z;
            W = 1d;
        }

        public Vector4(double x, double y, double z, double w = 1d)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }

        public static Vector4 operator +(Vector4 a, Vector4 b)
        {
            return new Vector4() { X = a.X + b.X, Y = a.Y + b.Y, Z = a.Z + b.Z, W = a.W + b.W };
        }

        public static Vector4 operator -(Vector4 a, Vector4 b)
        {
            return new Vector4() { X = a.X - b.X, Y = a.Y - b.Y, Z = a.Z - b.Z, W = a.W - b.W };
        }

        public static Vector4 operator *(Vector4 a, Vector4 b)
        {
            return new Vector4() { X = a.Y * b.Z - a.Z * b.Y, Y = a.Z * b.X - a.X * b.Z, Z = a.X * b.Y - a.Y * b.X, W = 1d };
        }

        public static Vector4 operator *(Vector4 a, double b)
        {
            return new Vector4() { X = a.X * b, Y = a.Y * b, Z = a.Z * b, W = a.W * b };
        }

        public static Vector4 operator *(double b, Vector4 a)
        {
            return a * b;
        }

        public static Vector4 operator /(Vector4 a, double b)
        {
            return new Vector4() { X = a.X / b, Y = a.Y / b, Z = a.Z / b, W = a.W / b };
        }

        public static Vector4 operator /(double a, Vector4 b)
        {
            return new Vector4() { X = a / b.X, Y = a / b.Y, Z = a / b.Z, W = a / b.W };
        }

        public static double operator %(Vector4 a, Vector4 b)
        {
            return a.X * b.X + a.Y * b.Y + a.Z * b.Z + a.W * b.W;
        }

        public static bool operator ==(Vector4 a, Vector4 b)
        {
            return a.X == b.X && a.Y == b.Y && a.Z == b.Z && a.W == b.W;
        }

        public static bool operator !=(Vector4 a, Vector4 b)
        {
            return a.X != b.X || a.Y != b.Y || a.Z != b.Z || a.W != b.W;
        }

        public static Vector4 operator -(Vector4 a)
        {
            return new Vector4() { X = -a.X, Y = -a.Y, Z = -a.Z, W = -a.W };
        }

        public double Length
        {
            get
            {
                return Math.Sqrt(X * X + Y * Y + Z * Z + W * W);
            }
        }

        public void Normalize()
        {
            double length = Length;
            if (length != 0d)
            {
                X /= length;
                Y /= length;
                Z /= length;
                W /= length;
            }
        }

        public Vector4 Direction
        {
            get
            {
                double length = Length;
                if (length == 0d)
                    return Vector4.Zero;
                else
                    return new Vector4(X / length, Y / length, Z / length, W / length);
            }
        }

        public static Vector4 Interpolate(Vector4 a, Vector4 b, double d)
        {
            return a * (1 - d) + b * d;
        }

        public override string ToString()
        {
            return String.Format("{0}, {1}, {2}, {3}", new object[] { X, Y, Z, W });
        }

        public static readonly Vector4 Zero = new Vector4();
        public static readonly Vector4 UnitX = new Vector4() { X = 1d };
        public static readonly Vector4 UnitY = new Vector4() { Y = 1d };
        public static readonly Vector4 UnitZ = new Vector4() { Z = 1d };
        public bool IsZero
        {
            get
            {
                return X == 0d && Y == 0d && Z == 0d && W == 0d;
            }
        }
    }

    public struct Matrix4x4
    {
        public double M11, M12, M13, M14, M21, M22, M23, M24, M31, M32, M33, M34, M41, M42, M43, M44;
        public Matrix4x4(
            double m11, double m12, double m13, double m14, 
            double m21, double m22, double m23, double m24, 
            double m31, double m32, double m33, double m34, 
            double m41, double m42, double m43, double m44)
        {
            M11 = m11;
            M12 = m12;
            M13 = m13;
            M14 = m14;
            
            M21 = m21;
            M22 = m22;
            M23 = m23;
            M24 = m24;

            M31 = m31;
            M32 = m32;
            M33 = m33;
            M34 = m34;

            M41 = m41;
            M42 = m42;
            M43 = m43;
            M44 = m44;
        }

        public static Matrix4x4 Scale(double x, double y, double z)
        {
            return new Matrix4x4(x, 0d, 0d, 0d, 0d, y, 0d, 0d, 0d, 0d, z, 0d, 0d, 0d, 0d, 1d);
        }

        public double Determinant
        {
            get
            {
                return
                    M14 * M23 * M32 * M41 - M13 * M24 * M32 * M41 - M14 * M22 * M33 * M41 + M12 * M24 * M33 * M41 +
                    M13 * M22 * M34 * M41 - M12 * M23 * M34 * M41 - M14 * M23 * M31 * M42 + M13 * M24 * M31 * M42 +
                    M14 * M21 * M33 * M42 - M11 * M24 * M33 * M42 - M13 * M21 * M34 * M42 + M11 * M23 * M34 * M42 +
                    M14 * M22 * M31 * M43 - M12 * M24 * M31 * M43 - M14 * M21 * M32 * M43 + M11 * M24 * M32 * M43 +
                    M12 * M21 * M34 * M43 - M11 * M22 * M34 * M43 - M13 * M22 * M31 * M44 + M12 * M23 * M31 * M44 +
                    M13 * M21 * M32 * M44 - M11 * M23 * M32 * M44 - M12 * M21 * M33 * M44 + M11 * M22 * M33 * M44;
            }
        }

        public Matrix4x4 Inverse
        {
            get
            {
                double d = Determinant;

                return new Matrix4x4(
                    ((-M24) * M33 * M42 + M23 * M34 * M42 + M24 * M32 * M43 - M22 * M34 * M43 - M23 * M32 * M44 + M22 * M33 * M44) / d,
                    (M14 * M33 * M42 - M13 * M34 * M42 - M14 * M32 * M43 + M12 * M34 * M43 + M13 * M32 * M44 - M12 * M33 * M44) / d,
                    ((-M14) * M23 * M42 + M13 * M24 * M42 + M14 * M22 * M43 - M12 * M24 * M43 - M13 * M22 * M44 + M12 * M23 * M44) / d,
                    (M14 * M23 * M32 - M13 * M24 * M32 - M14 * M22 * M33 + M12 * M24 * M33 + M13 * M22 * M34 - M12 * M23 * M34) / d,
                    (M24 * M33 * M41 - M23 * M34 * M41 - M24 * M31 * M43 + M21 * M34 * M43 + M23 * M31 * M44 - M21 * M33 * M44) / d,
                    ((-M14) * M33 * M41 + M13 * M34 * M41 + M14 * M31 * M43 - M11 * M34 * M43 - M13 * M31 * M44 + M11 * M33 * M44) / d,
                    (M14 * M23 * M41 - M13 * M24 * M41 - M14 * M21 * M43 + M11 * M24 * M43 + M13 * M21 * M44 - M11 * M23 * M44) / d,
                    ((-M14) * M23 * M31 + M13 * M24 * M31 + M14 * M21 * M33 - M11 * M24 * M33 - M13 * M21 * M34 + M11 * M23 * M34) / d,
                    ((-M24) * M32 * M41 + M22 * M34 * M41 + M24 * M31 * M42 - M21 * M34 * M42 - M22 * M31 * M44 + M21 * M32 * M44) / d,
                    (M14 * M32 * M41 - M12 * M34 * M41 - M14 * M31 * M42 + M11 * M34 * M42 + M12 * M31 * M44 - M11 * M32 * M44) / d,
                    ((-M14) * M22 * M41 + M12 * M24 * M41 + M14 * M21 * M42 - M11 * M24 * M42 - M12 * M21 * M44 + M11 * M22 * M44) / d,
                    (M14 * M22 * M31 - M12 * M24 * M31 - M14 * M21 * M32 + M11 * M24 * M32 + M12 * M21 * M34 - M11 * M22 * M34) / d,
                    (M23 * M32 * M41 - M22 * M33 * M41 - M23 * M31 * M42 + M21 * M33 * M42 + M22 * M31 * M43 - M21 * M32 * M43) / d,
                    ((-M13) * M32 * M41 + M12 * M33 * M41 + M13 * M31 * M42 - M11 * M33 * M42 - M12 * M31 * M43 + M11 * M32 * M43) / d,
                    (M13 * M22 * M41 - M12 * M23 * M41 - M13 * M21 * M42 + M11 * M23 * M42 + M12 * M21 * M43 - M11 * M22 * M43) / d,
                    ((-M13) * M22 * M31 + M12 * M23 * M31 + M13 * M21 * M32 - M11 * M23 * M32 - M12 * M21 * M33 + M11 * M22 * M33) / d);
            }
        }

        public static Matrix4x4 operator *(Matrix4x4 a, Matrix4x4 b)
        {
            return new Matrix4x4() { 
                M11 = a.M11 * b.M11 + a.M12 * b.M21 + a.M13 * b.M31 + a.M14 * b.M41,
                M21 = a.M21 * b.M11 + a.M22 * b.M21 + a.M23 * b.M31 + a.M24 * b.M41,
                M31 = a.M31 * b.M11 + a.M32 * b.M21 + a.M33 * b.M31 + a.M34 * b.M41,
                M41 = a.M41 * b.M11 + a.M42 * b.M21 + a.M43 * b.M31 + a.M44 * b.M41,

                M12 = a.M11 * b.M12 + a.M12 * b.M22 + a.M13 * b.M32 + a.M14 * b.M42,
                M22 = a.M21 * b.M12 + a.M22 * b.M22 + a.M23 * b.M32 + a.M24 * b.M42,
                M32 = a.M31 * b.M12 + a.M32 * b.M22 + a.M33 * b.M32 + a.M34 * b.M42,
                M42 = a.M41 * b.M12 + a.M42 * b.M22 + a.M43 * b.M32 + a.M44 * b.M42,

                M13 = a.M11 * b.M13 + a.M12 * b.M23 + a.M13 * b.M33 + a.M14 * b.M43,
                M23 = a.M21 * b.M13 + a.M22 * b.M23 + a.M23 * b.M33 + a.M24 * b.M43,
                M33 = a.M31 * b.M13 + a.M32 * b.M23 + a.M33 * b.M33 + a.M34 * b.M43,
                M43 = a.M41 * b.M13 + a.M42 * b.M23 + a.M43 * b.M33 + a.M44 * b.M43,

                M14 = a.M11 * b.M14 + a.M12 * b.M24 + a.M13 * b.M34 + a.M14 * b.M44,
                M24 = a.M21 * b.M14 + a.M22 * b.M24 + a.M23 * b.M34 + a.M24 * b.M44,
                M34 = a.M31 * b.M14 + a.M32 * b.M24 + a.M33 * b.M34 + a.M34 * b.M44,
                M44 = a.M41 * b.M14 + a.M42 * b.M24 + a.M43 * b.M34 + a.M44 * b.M44
            };
        }

        public static Vector4 operator * (Matrix4x4 a, Vector4 b)
        {
            return new Vector4()
            {
                X = a.M11 * b.X + a.M12 * b.Y + a.M13 * b.Z + a.M14 * b.W,
                Y = a.M21 * b.X + a.M22 * b.Y + a.M23 * b.Z + a.M24 * b.W,
                Z = a.M31 * b.X + a.M32 * b.Y + a.M33 * b.Z + a.M34 * b.W,
                W = a.M41 * b.X + a.M42 * b.Y + a.M43 * b.Z + a.M44 * b.W
            };
        }

        public static Vector4 operator * (Vector4 a, Matrix4x4 b)
        {
            return new Vector4()
            {
                X = a.X * b.M11 + a.Y * b.M21 + a.Z * b.M31 + a.W * b.M41,
                Y = a.X * b.M12 + a.Y * b.M22 + a.Z * b.M32 + a.W * b.M42,
                Z = a.X * b.M13 + a.Y * b.M23 + a.Z * b.M33 + a.W * b.M43,
                W = a.X * b.M14 + a.Y * b.M24 + a.Z * b.M34 + a.W * b.M44
            };
        }

        public override string ToString()
        {
            return String.Format("{0}, {1}, {2}, {3}; {4}, {5}, {6}, {7}; {8}, {9}, {10}, {11}; {12}, {12}, {13}, {14}, {15}",
                new object[] { M11, M12, M13, M14, M21, M22, M23, M24, M31, M32, M33, M34, M41, M42, M43, M44 });
        }
    }

    public struct Line
    {
        public Vector3 Origin, Direction;
        public Line(Vector3 origin, Vector3 direction)
        {
            Origin = origin;
            Direction = direction;
        }
        public static Vector3 DistanceBetweenLines(Line a, Line b)
        {
            Vector3 d = (a.Direction * b.Direction).Direction;
            return d * ((b.Origin - a.Origin) % d);
        }

        public static Vector3 Midpoint(Line a, Line b)
        {
            Vector3 d = Line.DistanceBetweenLines(a, b);
            double t = ((a.Origin.Y + d.Y - b.Origin.Y) * b.Direction.X - (a.Origin.X + d.X - b.Origin.X) * b.Direction.Y) / (a.Direction.X * b.Direction.Y - a.Direction.Y * b.Direction.X);
            return a.Origin + a.Direction * t + d / 2;
        }

        public Vector3 Intersect(Plane p)
        {
            double t = (p.Origin % p.Normal - Origin % p.Normal) / (Direction % p.Normal);
            return Origin + t * Direction;
        }
    }

    public struct Plane
    {
        public Vector3 Origin, Normal;
        
        public Plane(Vector3 origin, Vector3 normal)
        {
            Origin = origin;
            Normal = normal;
        }
        
        public static Line Intersect(Plane a, Plane b)
        {
            Line L = new Line();
            L.Direction = a.Normal * b.Normal;
            Line lA = new Line(a.Origin, L.Direction * a.Normal);
            Line lB = new Line(b.Origin, L.Direction * b.Normal);
            L.Origin = Line.Midpoint(lA, lB);
            return L;
        }
    }

    public struct Float4
    {
        public float X, Y, Z, W;

        public Float4(float x, float y, float z, float w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }

        public static readonly Float4 Zero = new Float4();

        public bool IsZero
        {
            get
            {
                return X == 0f && Y == 0f && Z == 0f && W == 0f;
            }
        }

        public override string ToString()
        {
            return String.Format("{0}, {1}, {2}, {3}", new object[] { X, Y, Z, W });
        }
    }
}
