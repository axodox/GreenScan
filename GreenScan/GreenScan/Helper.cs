using Green.MathExtensions;
using Green.Settings;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;

namespace Green.Helper
{
    class VectorImage : Border
    {
        private double imageWidth, imageHeight;
        public double ImageWidth 
        {
            get
            {
                return imageWidth;
            }
            set
            {
                imageWidth = value;
                SetImageSize();
            }
        }
        public double ImageHeight
        {
            get
            {
                return imageHeight;
            }
            set
            {
                imageHeight = value;
                SetImageSize();
            }
        }
        void SetImageSize()
        {
            Rect bgRect = new Rect(0d, 0d, imageWidth, imageHeight);
            ClipGeometry.Rect = bgRect;
            BackgroundRectangle.Rect = bgRect;
        }

        Image Image;
        DrawingImage DrawingImage;
        DrawingGroup DrawingGroup;
        RectangleGeometry ClipGeometry;
        GeometryDrawing BackgroundDrawing;
        RectangleGeometry BackgroundRectangle;
        public Brush BackgroundBrush 
        {
            get
            {
                return BackgroundDrawing.Brush;
            }
            set
            {
                BackgroundDrawing.Brush = value;
            }
        }
        public VectorImage()
        {
            DrawingGroup = new DrawingGroup();
            ClipGeometry = new RectangleGeometry();
            DrawingGroup.ClipGeometry = ClipGeometry;
            BackgroundRectangle = new RectangleGeometry();
            BackgroundDrawing = new GeometryDrawing(Brushes.Transparent, null, BackgroundRectangle);
            DrawingGroup.Children.Add(BackgroundDrawing);
            DrawingImage = new DrawingImage(DrawingGroup);
            Image = new Image();
            Image.Source = DrawingImage;
            Child = Image;
            LastPen = null;
            LastBrush = null;
            LastGeometryGroup = null;
        }

        Pen LastPen;
        Brush LastBrush;
        GeometryGroup LastGeometryGroup;
        public Pen Pen { get; set; }
        public Brush Brush { get; set; }
        public void AddGeometry(Geometry geometry)
        {
            if (Pen != LastPen || Brush != LastBrush)
            {
                LastGeometryGroup = new GeometryGroup();
                GeometryDrawing geometryDrawing = new GeometryDrawing(Brush, Pen, LastGeometryGroup);
                DrawingGroup.Children.Add(geometryDrawing);
            }
            LastGeometryGroup.Children.Add(geometry);
        }

        public void Clear()
        {
            while (DrawingGroup.Children.Count > 1)
            {
                DrawingGroup.Children.RemoveAt(1);
            }
            LastPen = null;
            LastBrush = null;
            LastGeometryGroup = null;
        }

        Size FittedSize;
        Point FittedPosition;
        protected override Size ArrangeOverride(Size arrangeBounds)
        {
            double actualWidth = arrangeBounds.Width, actualHeight = arrangeBounds.Height;
            if (actualWidth / actualHeight > ImageWidth / ImageHeight)
            {
                FittedSize = new Size(ImageWidth * actualHeight / ImageHeight, actualHeight);
                FittedPosition = new Point((actualWidth - FittedSize.Width) / 2d, 0d);
            }
            else
            {
                FittedSize = new Size(actualWidth, ImageHeight * actualWidth / ImageWidth);
                FittedPosition = new Point(0d, (actualHeight - FittedSize.Height) / 2d);
            }
            return base.ArrangeOverride(arrangeBounds);
        }

        public Point PointToCanvas(Point p)
        {
            return new Point(
                (p.X - FittedPosition.X) / FittedSize.Width * ImageWidth,
                (p.Y - FittedPosition.Y) / FittedSize.Height * ImageHeight);
        }
    }

    class VectorImage3D : VectorImage
    {
        public Matrix4x4 WorldViewProjection { get; set; }
        private Point Project(Vector3 position)
        {
            Vector4 pos = WorldViewProjection * new Vector4(position);
            return new Point(pos.X / pos.Z * ImageWidth, pos.Y / pos.Z * ImageHeight);
        }

        public void DrawPoint(Vector3 position)
        {
            EllipseGeometry eg = new EllipseGeometry();
            eg.Center = Project(position);
            eg.RadiusX = 3d;
            eg.RadiusY = 3d;
            AddGeometry(eg);
        }

        public void DrawLine(Vector3 a, Vector3 b)
        {
            LineGeometry lg = new LineGeometry();
            lg.StartPoint = Project(a);
            lg.EndPoint = Project(b);
            AddGeometry(lg);
        }

        const double Length = 0.1d;
        public void DrawPlane(Plane plane)
        {
            Vector3 u = (Vector3.UnitX * plane.Normal).Direction;
            Vector3 v = (plane.Normal * u).Direction;
            Vector3 o = plane.Origin;
            Vector3 x = plane.Origin + plane.Normal * Length / 2;
            Vector3 a = plane.Origin + (u + v) * Length;
            Vector3 b = plane.Origin + (u - v) * Length;
            Vector3 c = plane.Origin + (-u - v) * Length;
            Vector3 d = plane.Origin + (-u + v) * Length;
            DrawPoint(o);
            DrawLine(o, x);
            DrawLine(a, b);
            DrawLine(b, c);
            DrawLine(c, d);
            DrawLine(d, a);
        }
    }

    public static class Extensions
    {
        public static Matrix4x4 ToMatrix4x4(this MatrixSetting setting)
        {
            if (setting.Rows != 4 || setting.Columns != 4) throw new ArgumentException();
            return new Matrix4x4(
                setting.Value[0, 0], setting.Value[0, 1], setting.Value[0, 2], setting.Value[0, 3],
                setting.Value[1, 0], setting.Value[1, 1], setting.Value[1, 2], setting.Value[1, 3],
                setting.Value[2, 0], setting.Value[2, 1], setting.Value[2, 2], setting.Value[2, 3],
                setting.Value[3, 0], setting.Value[3, 1], setting.Value[3, 2], setting.Value[3, 3]);
        }

        public static Matrix4x4 GrowToMatrix4x4(this MatrixSetting setting)
        {
            if (setting.Rows != 3 || setting.Columns != 3) throw new ArgumentException();
            return new Matrix4x4(
                setting.Value[0, 0], setting.Value[0, 1], setting.Value[0, 2], 0d,
                setting.Value[1, 0], setting.Value[1, 1], setting.Value[1, 2], 0d,
                setting.Value[2, 0], setting.Value[2, 1], setting.Value[2, 2], 0d,
                0d, 0d, 0d, 1d);
        }

        public static Matrix4x4 ExpandToMatrix4x4(this MatrixSetting setting)
        {
            if (setting.Rows != 3 || setting.Columns != 3) throw new ArgumentException();
            return new Matrix4x4(
                setting.Value[0, 0], setting.Value[0, 1], 0d, setting.Value[0, 2],
                setting.Value[1, 0], setting.Value[1, 1], 0d, setting.Value[1, 2],
                setting.Value[2, 0], setting.Value[2, 1], setting.Value[2, 2], 0d,
                0d, 0d, 0d, 1d);
        }
    }
}
