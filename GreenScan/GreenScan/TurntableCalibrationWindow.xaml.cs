using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using Green.Graphics;
using Green.Scan;
using Green.MathExtensions;
using Green.Helper;
using Green.Kinect;
using Line = Green.MathExtensions.Line;
using GreenResources = GreenScan.Properties.Resources;

namespace GreenScan
{
    /// <summary>
    /// Interaction logic for TurntableCalibrationWindow.xaml
    /// </summary>
    public partial class TurntableCalibrationWindow : Window
    {
        GraphicsCanvas Canvas;
        ScanSettings Settings;
        TurntableCalibrator Calibrator;
        private string[] StepDescriptions =
        {
            GreenResources.TurntableCalibrationStep1,
            GreenResources.TurntableCalibrationStep2,
            GreenResources.TurntableCalibrationStep3,
            GreenResources.TurntableCalibrationStep4,
            GreenResources.TurntableCalibrationStep5
        };

        public TurntableCalibrationWindow(GraphicsCanvas canvas, ScanSettings settings)
        {
            InitializeComponent();
            Canvas = canvas;
            Settings = settings;
            Calibrator = new TurntableCalibrator();
            Matrix4x4 mInfraredIntrinsics = settings.InfraredIntrinsics.GrowToMatrix4x4();
            Matrix4x4 mDepthToIRMapping = settings.DepthToIRMapping.ExpandToMatrix4x4();
            Matrix4x4 mDepthIntrinsics = mDepthToIRMapping.Inverse * mInfraredIntrinsics;
            VI.WorldViewProjection = Matrix4x4.Scale(1d / KinectManager.DepthWidth, 1d / KinectManager.DepthHeight, 1d) * mDepthIntrinsics;

            SelectedRectangle = settings.TurntableRectangle.Value;
            SelectedEllipse = settings.TurntableEllipse.Value;
            SetStep(0);
        }

        Rect SelectedRectangle, SelectedEllipse;
        Geometry SelectionGeometry;
        int Step;
        Float4[,] TableDepthData, StandardDepthData;
        void SetStep(int step)
        {
            Step = step;
            BNext.IsEnabled = Step < StepDescriptions.Length - 1;
            BPrevious.IsEnabled = Step > 0;
            TBDescription.Text = StepDescriptions[step];
            PBProgress.Value = Step * 100d / (StepDescriptions.Length - 1);
            VI.Clear();
            Float4[,] vertices;
            switch (Step)
            {
                case 1:
                    if (Canvas.GetVertices(out vertices))
                    {
                        TableDepthData = vertices;
                        IDepth.Source = Vector4ArrayToBitmapSource(TableDepthData, 2f);
                    }
                    VI.Pen = new Pen(Brushes.Blue, 2d);
                    VI.Brush = new SolidColorBrush(Color.FromArgb(128, 0, 0, 255));
                    SelectionGeometry = new EllipseGeometry(SelectedEllipse);
                    VI.AddGeometry(SelectionGeometry);
                    break;
                case 2:
                    if(TableDepthData != null)
                    {
                        Calibrator.CalibrateTable(TableDepthData, SelectedEllipse.TopLeft, SelectedEllipse.BottomRight);
                    }
                    VI.Pen = new Pen(Brushes.Green, 2d);
                    VI.Brush = new SolidColorBrush(Color.FromArgb(128, 0, 255, 0));
                    VI.DrawPlane(Calibrator.Ground);
                    break;
                case 3:
                    if (Canvas.GetVertices(out vertices))
                    {
                        StandardDepthData = vertices;
                        IDepth.Source = Vector4ArrayToBitmapSource(StandardDepthData, 2f);
                    }
                    VI.Pen = new Pen(Brushes.Blue, 2d);
                    VI.Brush = new SolidColorBrush(Color.FromArgb(128, 0, 0, 255));
                    SelectionGeometry = new RectangleGeometry(SelectedRectangle);
                    VI.AddGeometry(SelectionGeometry);
                    BNext.Visibility = Visibility.Visible;
                    BFinish.Visibility = Visibility.Collapsed;
                    break;
                case 4:
                    if (StandardDepthData != null)
                    {
                        Calibrator.CalibrateStandard(StandardDepthData, SelectedRectangle.TopLeft, SelectedRectangle.BottomRight);
                        
                    }
                    VI.Pen = new Pen(Brushes.Green, 2d);
                    VI.Brush = new SolidColorBrush(Color.FromArgb(128, 0, 255, 0));
                    VI.DrawPlane(Calibrator.StandardA);
                    VI.DrawPlane(Calibrator.StandardB);
                    VI.DrawPlane(Calibrator.Table);
                    BNext.Visibility = Visibility.Collapsed;
                    BFinish.Visibility = Visibility.Visible;
                    break;
            }
        }

        private BitmapSource Vector4ArrayToBitmapSource(Float4[,] source, float maxDepth)
        {
            int width = source.GetLength(0), height = source.GetLength(1);
            byte[] target = new byte[width * height];
            unsafe
            {
                fixed (Float4* sSource = &source[0, 0])
                fixed (byte* sTarget = &target[0])
                {
                    Float4* pSource = sSource;
                    byte* pTarget = sTarget, eTarget = sTarget + width * height;
                    float depth;
                    while (pTarget < eTarget)
                    {
                        depth = pSource->Z;
                        if (depth > maxDepth)
                            *pTarget = 255;
                        else if (depth < 0)
                            *pTarget = 0;
                        else
                            *pTarget = (byte)(depth * 255 / maxDepth);
                        pTarget++;
                        pSource++;
                    }
                }
            }
            return BitmapSource.Create(width, height, 96d, 96d, PixelFormats.Gray8, BitmapPalettes.Gray256, target, width);
        }

        private void BNext_Click(object sender, RoutedEventArgs e)
        {
            SetStep(Step + 1);
        }

        private void BPrevious_Click(object sender, RoutedEventArgs e)
        {
            SetStep(Step - 1);
        }

        bool IsMouseOnCanvas
        {
            get
            {
                Point p = Mouse.GetPosition(VI);
                return p.X > 0 && p.Y > 0 && p.X < VI.ActualWidth && p.Y < VI.ActualHeight;
            }
        }

        bool MouseManipulation;
        Point StartPoint, EndPoint;
        private void IDepth_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (!IsMouseOnCanvas) return;
            StartPoint = Mouse.GetPosition(VI);
            switch (Step)
            {
                case 1:
                case 3:
                    Mouse.Capture(IDepth);
                    MouseManipulation = true;
                    break;
            }
        }

        private void IDepth_MouseMove(object sender, MouseEventArgs e)
        {
            if (!MouseManipulation) return;
            if (Mouse.LeftButton == MouseButtonState.Released)
            {
                MouseManipulation = false;
                Mouse.Capture(null);
                Settings.TurntableEllipse.Value = SelectedEllipse;
                Settings.TurntableRectangle.Value = SelectedRectangle;
                return;
            }
            EndPoint = Mouse.GetPosition(VI);
            Point start = VI.PointToCanvas(StartPoint);
            Point end = VI.PointToCanvas(EndPoint);
            if (Mouse.LeftButton == MouseButtonState.Pressed)
            {
                switch (Step)
                {
                    case 1:
                        EllipseGeometry eg = SelectionGeometry as EllipseGeometry;
                        eg.Center = new Point((start.X + end.X) / 2d, (start.Y + end.Y) / 2d);
                        eg.RadiusX = Math.Abs(start.X - end.X) / 2d;
                        eg.RadiusY = Math.Abs(start.Y - end.Y) / 2d;
                        SelectedEllipse = new Rect(start, end);
                        break;
                    case 3:
                        RectangleGeometry rg = SelectionGeometry as RectangleGeometry;
                        SelectedRectangle = rg.Rect = new Rect(start, end);
                        break;
                }
            }
        }

        private void BFinish_Click(object sender, RoutedEventArgs e)
        {
            Settings.TurntableTransform.Value = Calibrator.Table.GetTransform().GetMatrix();
            Close();
        }
    }

    public class TurntableCalibrator
    {
        public abstract class Region
        {
            public int MinX { get; protected set; }
            public int MinY { get; protected set; }
            public int MaxX { get; protected set; }
            public int MaxY { get; protected set; }
            public abstract bool Covered(int x, int y);
        }

        public class EllipseRegion : Region
        {
            double ellipseX, ellipseY, ellipseA, ellipseB;
            public EllipseRegion(Point a, Point b)
            {
                MinX = (int)Math.Min(a.X, b.X);
                MaxX = (int)Math.Max(a.X, b.X);
                MinY = (int)Math.Min(a.Y, b.Y);
                MaxY = (int)Math.Max(a.Y, b.Y);

                ellipseX = (b.X + a.X) / 2d;
                ellipseY = (b.Y + a.Y) / 2d;
                ellipseA = Math.Pow((b.X - a.X) / 2d, 2d);
                ellipseB = Math.Pow((b.Y - a.Y) / 2d, 2d);
            }
            public override bool Covered(int x, int y)
            {
                return Math.Pow(x - ellipseX, 2) / ellipseA + Math.Pow(y - ellipseY, 2) / ellipseB < 1;
            }
        }

        public class RectangleRegion : Region
        {
            public RectangleRegion(Point a, Point b)
            {
                MinX = (int)Math.Min(a.X, b.X);
                MaxX = (int)Math.Max(a.X, b.X);
                MinY = (int)Math.Min(a.Y, b.Y);
                MaxY = (int)Math.Max(a.Y, b.Y);
            }

            public override bool Covered(int x, int y)
            {
                return x >= MinX && x <= MaxX && y >= MinY && y <= MaxY;
            }
        }

        public Plane Ground { get; private set; }
        public Plane StandardA { get; private set; }
        public Plane StandardB { get; private set; }
        public Plane Table { get; private set; }
        public bool IsCalibrated { get; private set; }

        public float TranslationX { get { return (float)Table.Origin.X; } }
        public float TranslationY { get { return (float)-Table.Origin.Y; } }
        public float TranslationZ { get { return (float)Table.Origin.Z; } }
        public float RotationX
        {
            get
            {
                return (float)-Vector3.AngleOf(Vector3.UnitZ, Vector3.UnitX * Table.Normal).ToDegrees();
            }
        }
        public float RotationZ
        {
            get
            {
                return (float)-(Math.Asin(Math.Abs(Table.Normal.X / Table.Normal.Length))).ToDegrees();
            }
        }
        public TurntableCalibrator()
        {
            IsCalibrated = false;
        }

        public void CalibrateTable(Float4[,] data, Point tableA, Point tableB)
        {
            Ground = PlaneFromDepthData(data, new EllipseRegion(tableA, tableB));
        }
        public void CalibrateStandard(Float4[,] data, Point standardA, Point standardB)
        {
            int mX = (int)(standardA.X + standardB.X) / 2;
            Point standardM1 = new Point(mX - 10, standardB.Y);
            Point standardM2 = new Point(mX + 10, standardA.Y);
            Plane a = PlaneFromDepthData(data, new RectangleRegion(standardA, standardM1));
            Plane b = PlaneFromDepthData(data, new RectangleRegion(standardM2, standardB));
            Line axis = Plane.Intersect(a, b);
            Vector3 origin = axis.Intersect(Ground);
            Table = new Plane(origin, axis.Direction);
            StandardA = a;
            StandardB = b;
            IsCalibrated = true;
        }

        public delegate Float4 WorldPositionFunction(int x, int y, ushort depth);
        public static Plane PlaneFromDepthData(Float4[,] Data, Region region)
        {
            int width = Data.GetLength(0);
            int height = Data.GetLength(1);
            Plane P = new Plane();
            //Area selection
            int minX = (region.MinX < 0 ? 0 : region.MinX);
            int maxX = (region.MaxX > width ? width : region.MaxX);
            int minY = (region.MinY < 0 ? 0 : region.MinY);
            int maxY = (region.MaxY > height ? height : region.MaxY);
            int jumpX = width - (maxX - minX);
            int startX = minY * width + minX;
            int count = 0;
            Vector3 pos = Vector3.Zero;
            unsafe
            {
                fixed (Float4* sData = &Data[0, 0])
                {
                    //Calculate origin
                    Float4* pData = sData + startX;
                    for (int y = minY; y < maxY; y++)
                    {
                        for (int x = minX; x < maxX; x++)
                        {
                            pData++;
                            if (region.Covered(x, y) && pData->W != 0f)
                            {
                                pos.X += pData->X;
                                pos.Y += pData->Y;
                                pos.Z += pData->Z;
                                count++;
                            }
                        }
                        pData += jumpX;
                    }
                    pos /= count;
                    P.Origin = new Vector3(pos.X, pos.Y, pos.Z);

                    //Calculate normal
                    double a, b, c, d, e, f;
                    a = b = c = d = e = f = 0;
                    double rx, ry, rz;
                    pData = sData + startX;
                    for (int y = minY; y < maxY; y++)
                    {
                        for (int x = minX; x < maxX; x++)
                        {
                            pData++;
                            if (region.Covered(x, y) && pData->W != 0f)
                            {
                                pos = new Vector3(pData->X, pData->Y, pData->Z);
                                rx = pos.X - P.Origin.X;
                                ry = pos.Y - P.Origin.Y;
                                rz = pos.Z - P.Origin.Z;
                                a += ry * ry;
                                b += rz * ry;
                                c -= rx * ry;
                                d += ry * rz;
                                e += rz * rz;
                                f -= rx * rz;
                            }
                        }
                        pData += jumpX;
                    }
                    double nz = (c * d - a * f) / (b * d - a * e);
                    double ny = (f - e * nz) / d;
                    P.Normal = new Vector3(1d, ny, nz).Direction;
                }
            }
            return P;
        }

    }
}