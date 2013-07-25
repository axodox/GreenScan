using System;
using Green.Settings;
using Green.Kinect;
using Green.Graphics;
using System.Windows;

namespace Green.Scan
{
    public class ScanSettings : SettingManager
    {
        public SettingGroup KinectProperties { get; private set; }
        public EnumSetting<KinectManager.Modes> KinectMode { get; private set; }

        public SettingGroup PreprocessingProperties { get; private set; }
        public NumericSetting<int> DepthAveraging { get; private set; }
        public NumericSetting<int> DepthGaussIterations { get; private set; }
        public NumericSetting<float> DepthGaussSigma { get; private set; }

        public SettingGroup CameraProperties { get; private set; }
        public MatrixSetting InfraredIntrinsics { get; private set; }
        public MatrixSetting InfraredDistortion { get; private set; }
        public BooleanSetting InfraredDistortionCorrectionEnabled { get; private set; }
        public MatrixSetting DepthToIRMapping { get; private set; }
        public MatrixSetting ColorIntrinsics { get; private set; }
        public MatrixSetting ColorRemapping { get; private set; }
        public MatrixSetting ColorExtrinsics { get; private set; }
        public NumericSetting<int> ColorDispositionX { get; private set; }
        public NumericSetting<int> ColorDispositionY { get; private set; }
        public NumericSetting<float> ColorScaleX { get; private set; }
        public NumericSetting<float> ColorScaleY { get; private set; }

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
        public BooleanSetting UseModuleShading { get; private set; }
        public EnumSetting<GraphicsCanvas.ShadingModes> ShadingMode { get; private set; }
        public NumericSetting<float> DepthMaximum { get; private set; }
        public NumericSetting<float> DepthMinimum { get; private set; }
        public NumericSetting<float> ShadingPeriode { get; private set; }
        public NumericSetting<float> ShadingPhase { get; private set; }
        public NumericSetting<float> TriangleRemoveLimit { get; private set; }
        public BooleanSetting WireframeShading { get; private set; }

        public SettingGroup PerformanceProperties { get; private set; }
        public SizeSetting TriangleGridResolution { get; private set; }

        public SettingGroup SaveProperties { get; private set; }
        public PathSetting SaveDirectory { get; private set; }
        public StringSetting SaveLabel { get; private set; }
        public SizeSetting SaveModelResolution { get; private set; }
        public SizeSetting SaveTextureResolution { get; private set; }

        public SettingGroup TurntableProperties { get; private set; }
        public RectangleSetting TurntableEllipse { get; private set; }
        public RectangleSetting TurntableRectangle { get; private set; }
        public NumericSetting<float> TurntableTranslationX { get; private set; }
        public NumericSetting<float> TurntableTranslationY { get; private set; }
        public NumericSetting<float> TurntableTranslationZ { get; private set; }
        public NumericSetting<float> TurntableRotationX { get; private set; }
        public NumericSetting<float> TurntableRotationY { get; private set; }
        public NumericSetting<float> TurntableRotationZ { get; private set; }
        public NumericSetting<float> TurntableClippingHeight { get; private set; }
        public NumericSetting<float> TurntableClippingRadius { get; private set; }
        public NumericSetting<float> TurntableCoreX { get; private set; }
        public NumericSetting<float> TurntableCoreY { get; private set; }
        

        public SizeSetting TurntableModelResolution { get; private set; }
        public SizeSetting TurntableTextureResolution { get; private set; }

        public ScanSettings()
            : base()
        {
            //Kinect
            KinectProperties = new SettingGroup("Kinect") { FriendlyName = "Kinect", IsHidden = true };
            SettingGroups.Add(KinectProperties);

            KinectMode = new EnumSetting<KinectManager.Modes>("Mode", KinectManager.Modes.DepthAndColor) { FriendlyName = "Mode" };
            KinectProperties.Settings.Add(KinectMode);

            //Preprocessing
            PreprocessingProperties = new SettingGroup("Preprocessing") { FriendlyName = "Preprocessing" };
            SettingGroups.Add(PreprocessingProperties);

            DepthAveraging = new NumericSetting<int>("DepthAveraging", 1, 1, 32) { FriendlyName = "Depth averaging buffer size (slots)" };
            DepthGaussIterations = new NumericSetting<int>("DepthGaussIterations", 0, 0, 4) { FriendlyName = "Depth Gauss filtering (iterations)" };
            DepthGaussSigma = new NumericSetting<float>("DepthGaussSigma", 1, 0.1f, 4f, 2) { FriendlyName = "Depth Gauss filtering sigma (units)" };
            PreprocessingProperties.Settings.Add(DepthAveraging);
            PreprocessingProperties.Settings.Add(DepthGaussIterations);
            PreprocessingProperties.Settings.Add(DepthGaussSigma);

            //Camera
            CameraProperties = new SettingGroup("Camera") { FriendlyName = "Camera" };
            SettingGroups.Add(CameraProperties);

            InfraredIntrinsics = new MatrixSetting("InfraredIntrinsics", new float[,] { { 1161.96f, 0f, 639.8654f }, { 0f, 1169.649f, 521.4605f }, { 0f, 0f, 1f } }) { FriendlyName = "Infrared intrinsic matrix" };
            InfraredDistortion = new MatrixSetting("InfraredDistortion", new float[,] { { 0.143542f, -0.244779f }, { 0.003839f, -0.000318f } }) { FriendlyName = "Infrared distortion coefficients {K1, K2; P1, P2}" };
            InfraredDistortionCorrectionEnabled = new BooleanSetting("InfraredDistortionCorrectionEnabled", false) { FriendlyName = "Infrared distortion correction enabled" };
            DepthToIRMapping = new MatrixSetting("DepthToIRMapping", new float[,] { { 2f, 0f, 16f }, { 0f, 2f, 17f }, { 0f, 0f, 1f } }) { FriendlyName = "Depth to IR mapping" };
            ColorIntrinsics = new MatrixSetting("ColorIntrinsics", new float[,] { { 1051.45f, 0f, 641.1544f }, { 0f, 1053.781f, 521.7905f }, { 0f, 0f, 1f } }) { FriendlyName = "Color intrinsic matrix" };
            ColorRemapping = new MatrixSetting("ColorRemapping", new float[,] { { 2f, 0f, 2f }, { 0f, 2f, 0f }, { 0f, 0f, 1f } }) { FriendlyName = "Color remapping" };
            ColorExtrinsics = new MatrixSetting("ColorExtrinsics", new float[,] { { 0.999946f, -0.006657f, 0.00794f, -0.025955f }, { 0.006679f, 0.999974f, -0.002686f, -0.000035f }, { -0.007922f, 0.002739f, 0.999965f, 0.005283f }, { 0f, 0f, 0f, 1f } }) { FriendlyName = "Color extrinsics" };
            CameraProperties.Settings.Add(InfraredIntrinsics);
            CameraProperties.Settings.Add(InfraredDistortion);
            CameraProperties.Settings.Add(InfraredDistortionCorrectionEnabled);
            CameraProperties.Settings.Add(DepthToIRMapping);
            CameraProperties.Settings.Add(ColorIntrinsics);
            CameraProperties.Settings.Add(ColorRemapping);
            CameraProperties.Settings.Add(ColorExtrinsics);

            ColorDispositionX = new NumericSetting<int>("ColorDispositionX", -8, -32, 32) { FriendlyName = "Color X disposition (pixels)" };
            ColorDispositionY = new NumericSetting<int>("ColorDispositionY", 8, -32, 32) { FriendlyName = "Color Y disposition (pixels)" };
            ColorScaleX = new NumericSetting<float>("ColorScaleX", 1.01f, 0.8f, 1.2f, 2) { FriendlyName = "Color X scale (units)" };
            ColorScaleY = new NumericSetting<float>("ColorScaleY", 1f, 0.8f, 1.2f, 2) { FriendlyName = "Color Y scale (units)" };
            CameraProperties.Settings.Add(ColorDispositionX);
            CameraProperties.Settings.Add(ColorDispositionY);
            CameraProperties.Settings.Add(ColorScaleX);
            CameraProperties.Settings.Add(ColorScaleY);

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

            Scale = new NumericSetting<float>("Scale", 1f, 0f, 8f, 2) { FriendlyName = "Scale (units)" };
            MoveX = new NumericSetting<float>("MoveX", 0f, -1f, 1f, 2) { FriendlyName = "Move X (units)" };
            MoveY = new NumericSetting<float>("MoveY", 0f, -1f, 1f, 2) { FriendlyName = "Move Y (units)" };
            Rotation = new NumericSetting<int>("Rotation", 0, 0, 3) { FriendlyName = "Rotation (units)" };
            ViewProperties.Settings.Add(Scale);
            ViewProperties.Settings.Add(MoveX);
            ViewProperties.Settings.Add(MoveY);
            ViewProperties.Settings.Add(Rotation);

            //Shading
            ShadingProperties = new SettingGroup("Shading") { FriendlyName = "Shading" };
            SettingGroups.Add(ShadingProperties);

            UseModuleShading = new BooleanSetting("ModuleShading", false) { FriendlyName = "Use module shading (if available)" };
            ShadingMode = new EnumSetting<GraphicsCanvas.ShadingModes>("ShadingMode", GraphicsCanvas.ShadingModes.Rainbow) { FriendlyName = "Mode" };
            DepthMaximum = new NumericSetting<float>("DepthMaximum", 8f, 0f, 8f, 2) { FriendlyName = "Depth maximum (meters)" };
            DepthMinimum = new NumericSetting<float>("DepthMinimum", 0.8f, 0f, 8f, 2) { FriendlyName = "Depth minimum (meters)" };
            ShadingPeriode = new NumericSetting<float>("ShadingPeriode", 1f, 0.01f, 2f, 2) { FriendlyName = "Shading periode (meters)" };
            ShadingPhase = new NumericSetting<float>("ShadingPhase", 0f, 0f, 1f, 2) { FriendlyName = "Shading phase (radians)" };
            TriangleRemoveLimit = new NumericSetting<float>("TriangleRemoveLimit", 0.0024f, 0.0001f, 0.004f, 4) { FriendlyName = "Triangle remove limit (units)" };
            WireframeShading = new BooleanSetting("WireframeShading", false) { FriendlyName = "Wireframe shading" };
            ShadingProperties.Settings.Add(UseModuleShading);
            ShadingProperties.Settings.Add(ShadingMode);
            ShadingProperties.Settings.Add(DepthMaximum);
            ShadingProperties.Settings.Add(DepthMinimum);
            ShadingProperties.Settings.Add(ShadingPeriode);
            ShadingProperties.Settings.Add(ShadingPhase);
            ShadingProperties.Settings.Add(TriangleRemoveLimit);
            ShadingProperties.Settings.Add(WireframeShading);

            //Performance
            PerformanceProperties = new SettingGroup("Performance") { FriendlyName = "Performance" };
            SettingGroups.Add(PerformanceProperties);

            TriangleGridResolution = new SizeSetting("TriangleGridResolution", 640, 480, 16, 12, 640, 480) { FriendlyName = "Triangle grid resolution" };
            PerformanceProperties.Settings.Add(TriangleGridResolution);

            //Save
            SaveProperties = new SettingGroup("Save") { FriendlyName = "Saving" };
            SettingGroups.Add(SaveProperties);

            SaveDirectory = new PathSetting("Directory", "") { FriendlyName = "Directory" };
            SaveLabel = new StringSetting("Label", "") { FriendlyName = "Label" };
            SaveModelResolution = new SizeSetting("ModelResolution", 640, 480, 8, 8, 640, 480) { FriendlyName = "Model resolution (vertices)" };
            SaveTextureResolution = new SizeSetting("TextureResolution", 640, 480, 8, 8, 1024, 1024) { FriendlyName = "Texture resolution (pixels)" };
            SaveProperties.Settings.Add(SaveDirectory);
            SaveProperties.Settings.Add(SaveLabel);
            SaveProperties.Settings.Add(SaveModelResolution);
            SaveProperties.Settings.Add(SaveTextureResolution);

            //Turntable
            TurntableProperties = new SettingGroup("Turntable") { FriendlyName = "Turntable", IsHidden = true };
            SettingGroups.Add(TurntableProperties);

            TurntableEllipse = new RectangleSetting("SelectionEllipse", new Rect(0d, 0d, 0d, 0d)) { IsHidden = true };
            TurntableRectangle = new RectangleSetting("SelectionRectangle", new Rect(0d, 0d, 0d, 0d)) { IsHidden = true };
            TurntableProperties.Settings.Add(TurntableEllipse);
            TurntableProperties.Settings.Add(TurntableRectangle);

            TurntableTranslationX = new NumericSetting<float>("TranslationX", 0f, -1f, 1f, 3) { FriendlyName = "Translation X (meters)" };
            TurntableTranslationY = new NumericSetting<float>("TranslationY", 0f, -1f, 1f, 3) { FriendlyName = "Translation Y (meters)" };
            TurntableTranslationZ = new NumericSetting<float>("TranslationZ", 1.5f, 0f, 3f, 3) { FriendlyName = "Translation Z (meters)" };
            TurntableProperties.Settings.Add(TurntableTranslationX);
            TurntableProperties.Settings.Add(TurntableTranslationY);
            TurntableProperties.Settings.Add(TurntableTranslationZ);

            TurntableRotationX = new NumericSetting<float>("RotationX", 0f, -90f, 90f, 3) { FriendlyName = "Rotation X (degrees)" };
            TurntableRotationY = new NumericSetting<float>("RotationY", 0f, -90f, 90f, 3) { FriendlyName = "Rotation Y (degrees)" };
            TurntableRotationZ = new NumericSetting<float>("RotationZ", 0f, -180f, 180f, 3) { FriendlyName = "Rotation Z (degrees)" };
            TurntableProperties.Settings.Add(TurntableRotationX);
            TurntableProperties.Settings.Add(TurntableRotationY);
            TurntableProperties.Settings.Add(TurntableRotationZ);

            TurntableModelResolution = new SizeSetting("ModelResolution", 640, 480, 64, 64, 1024, 1024) { FriendlyName = "Model resolution (vertices/leg)" };
            TurntableTextureResolution = new SizeSetting("TextureResolution", 640, 480, 64, 64, 1024, 1024) { FriendlyName = "Texture resolution (pixels/leg)" };
            TurntableProperties.Settings.Add(TurntableModelResolution);
            TurntableProperties.Settings.Add(TurntableTextureResolution);

            TurntableClippingHeight = new NumericSetting<float>("ClippingHeight", 1f, 0f, 2f, 3) { FriendlyName = "Clipping height (meters)" };
            TurntableClippingRadius = new NumericSetting<float>("ClippingRadius", 0.3f, 0f, 2f, 3) { FriendlyName = "Clipping radius (meters)" };
            TurntableProperties.Settings.Add(TurntableClippingHeight);
            TurntableProperties.Settings.Add(TurntableClippingRadius);

            TurntableCoreX = new NumericSetting<float>("CoreX", 0.11f, 0f, 0.5f, 3) { FriendlyName = "Leg distance (meters)" };
            TurntableCoreY = new NumericSetting<float>("CoreY", -0.11f, -0.5f, 0.5f, 3) { FriendlyName = "Leg position (meters)" };
            TurntableProperties.Settings.Add(TurntableCoreX);
            TurntableProperties.Settings.Add(TurntableCoreY);
        }
    }
}