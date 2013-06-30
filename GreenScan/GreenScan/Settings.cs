using System;
using Green.Settings;
using Green.Kinect;
using Green.Graphics;

namespace Green.Scan
{
    class ScanSettings : SettingManager
    {
        public SettingGroup KinectProperties { get; private set; }
        public EnumSetting<KinectManager.Modes> KinectMode { get; private set; }

        public SettingGroup CameraProperties { get; private set; }
        public MatrixSetting InfraredIntrinsics { get; private set; }
        public MatrixSetting DepthToIRMapping { get; private set; }
        public MatrixSetting ColorIntrinsics { get; private set; }
        public MatrixSetting ColorRemapping { get; private set; }
        public MatrixSetting ColorExtrinsics { get; private set; }
        public NumericSetting<int> DepthDispositionX { get; private set; }
        public NumericSetting<int> DepthDispositionY { get; private set; }
        public NumericSetting<float> DepthScaleX { get; private set; }
        public NumericSetting<float> DepthScaleY { get; private set; }

        public SettingGroup ViewProperties { get; private set; }
        public NumericSetting<float> TranslationX { get; private set; }
        public NumericSetting<float> TranslationY { get; private set; }
        public NumericSetting<float> TranslationZ { get; private set; }
        public NumericSetting<float> RotationX { get; private set; }
        public NumericSetting<float> RotationY { get; private set; }
        public NumericSetting<float> RotationZ { get; private set; }
        public NumericSetting<float> Scale { get; private set; }
        public NumericSetting<float> MoveX { get; private set; }
        public NumericSetting<float> MoveY { get; private set; }
        public NumericSetting<int> Rotation { get; private set; }

        public SettingGroup ShadingProperties { get; private set; }
        public EnumSetting<DirectXCanvas.ShadingModes> ShadingMode { get; private set; }
        public NumericSetting<float> DepthLimit { get; private set; }
        public NumericSetting<float> ShadingPeriode { get; private set; }
        public NumericSetting<float> ShadingPhase { get; private set; }
        public NumericSetting<float> TriangleRemoveLimit { get; private set; }

        public ScanSettings()
            : base()
        {
            //Kinect
            KinectProperties = new SettingGroup("Kinect") { FriendlyName = "Kinect" };
            SettingGroups.Add(KinectProperties);

            KinectMode = new EnumSetting<KinectManager.Modes>("Mode", KinectManager.Modes.DepthAndColor) { FriendlyName = "Mode" };
            KinectProperties.Settings.Add(KinectMode);

            //Camera
            CameraProperties = new SettingGroup("Camera") { FriendlyName = "Camera" };
            SettingGroups.Add(CameraProperties);

            InfraredIntrinsics = new MatrixSetting("InfraredIntrinsics", new float[,] { { 1161.959596f, 0f, 639.865400f }, { 0f, 1169.649383f, 521.460524f }, { 0f, 0f, 1f } }) { FriendlyName = "Infrared intrinsic matrix" };

            DepthToIRMapping = new MatrixSetting("DepthToIRMapping", new float[,] { { 2f, 0f, 16f }, { 0f, 2f, 17f }, { 0f, 0f, 1f } }) { FriendlyName = "Depth to IR mapping" };
            ColorIntrinsics = new MatrixSetting("ColorIntrinsics", new float[,] { { 1051.45007f, 0f, 641.1544f }, { 0f, 1053.781f, 521.790466f }, { 0f, 0f, 1f } }) { FriendlyName = "Color intrinsics" };
            ColorRemapping = new MatrixSetting("ColorRemapping", new float[,] { { 2f, 0f, 2f }, { 0f, 2f, 0f }, { 0f, 0f, 1f } }) { FriendlyName = "Color remapping" };
            ColorExtrinsics = new MatrixSetting("ColorExtrinsics", new float[,] { { 0.999946f, -0.006657f, 0.00794f, -0.025955f }, { 0.006679f, 0.999974f, -0.002686f, -0.000035f }, { -0.007922f, 0.002739f, 0.999965f, 0.005283f }, { 0f, 0f, 0f, 1f } }) { FriendlyName = "Color extrinsics" };
            CameraProperties.Settings.Add(InfraredIntrinsics);
            CameraProperties.Settings.Add(DepthToIRMapping);
            CameraProperties.Settings.Add(ColorIntrinsics);
            CameraProperties.Settings.Add(ColorRemapping);
            CameraProperties.Settings.Add(ColorExtrinsics);

            DepthDispositionX = new NumericSetting<int>("DepthDispositionX", -1, -32, 32) { FriendlyName = "Depth X disposition (pixels)" };
            DepthDispositionY = new NumericSetting<int>("DepthDispositionY", -8, -32, 32) { FriendlyName = "Depth Y disposition (pixels)" };
            DepthScaleX = new NumericSetting<float>("DepthScaleX", 1.02f, 0.8f, 1.2f, 2) { FriendlyName = "Depth X scale (1)" };
            DepthScaleY = new NumericSetting<float>("DepthScaleY", 1f, 0.8f, 1.2f, 2) { FriendlyName = "Depth Y scale (1)" };
            CameraProperties.Settings.Add(DepthDispositionX);
            CameraProperties.Settings.Add(DepthDispositionY);
            CameraProperties.Settings.Add(DepthScaleX);
            CameraProperties.Settings.Add(DepthScaleY);

            //View
            ViewProperties = new SettingGroup("View") { FriendlyName = "View" };
            SettingGroups.Add(ViewProperties);

            TranslationX = new NumericSetting<float>("TranslationX", 0f, -1f, 1f, 2) { FriendlyName = "Translation X (meters)" };
            TranslationY = new NumericSetting<float>("TranslationY", 0f, -1f, 1f, 2) { FriendlyName = "Translation Y (meters)" };
            TranslationZ = new NumericSetting<float>("TranslationZ", 1.5f, 0f, 3f, 2) { FriendlyName = "Translation Z (meters)" };
            ViewProperties.Settings.Add(TranslationX);
            ViewProperties.Settings.Add(TranslationY);
            ViewProperties.Settings.Add(TranslationZ); 

            RotationX = new NumericSetting<float>("RotationX", 0f, -90f, 90f, 2) { FriendlyName = "Rotation X (degrees)" };
            RotationY = new NumericSetting<float>("RotationY", 0f, -90f, 90f, 2) { FriendlyName = "Rotation Y (degrees)" };
            RotationZ = new NumericSetting<float>("RotationZ", 0f, -180f, 180f, 2) { FriendlyName = "Rotation Z (degrees)" };
            ViewProperties.Settings.Add(RotationX);
            ViewProperties.Settings.Add(RotationY);
            ViewProperties.Settings.Add(RotationZ);

            Scale = new NumericSetting<float>("Scale", 1f, 0f, 8f, 2) { FriendlyName = "Scale (1)" };
            MoveX = new NumericSetting<float>("MoveX", 0f, -1f, 1f, 2) { FriendlyName = "Move X (1)" };
            MoveY = new NumericSetting<float>("MoveY", 0f, -1f, 1f, 2) { FriendlyName = "Move Y (1)" };
            Rotation = new NumericSetting<int>("Rotation", 0, 0, 3) { FriendlyName = "Rotation (1)" };
            ViewProperties.Settings.Add(Scale);
            ViewProperties.Settings.Add(MoveX);
            ViewProperties.Settings.Add(MoveY);
            ViewProperties.Settings.Add(Rotation);

            //Shading
            ShadingProperties = new SettingGroup("Shading") { FriendlyName = "Shading" };
            SettingGroups.Add(ShadingProperties);

            ShadingMode = new EnumSetting<DirectXCanvas.ShadingModes>("ShadingMode", DirectXCanvas.ShadingModes.ShadedScale) { FriendlyName = "Mode" };
            DepthLimit = new NumericSetting<float>("DepthLimit", 8f, 0f, 8f, 2) { FriendlyName = "Depth limit (meters)" };
            ShadingPeriode = new NumericSetting<float>("ShadingPeriode", 1f, 0f, 2f, 2) { FriendlyName = "Shading periode (meters)" };
            ShadingPhase = new NumericSetting<float>("ShadingPhase", 0f, 0f, 1f, 2) { FriendlyName = "Shading phase (radians)" };
            TriangleRemoveLimit = new NumericSetting<float>("TriangleRemoveLimit", 0.0024f, 0.0005f, 0.004f, 4) { FriendlyName = "Triangle remove limit (1)" };
            ShadingProperties.Settings.Add(ShadingMode);
            ShadingProperties.Settings.Add(DepthLimit);
            ShadingProperties.Settings.Add(ShadingPeriode);
            ShadingProperties.Settings.Add(ShadingPhase);
            ShadingProperties.Settings.Add(TriangleRemoveLimit);
        }
    }
}