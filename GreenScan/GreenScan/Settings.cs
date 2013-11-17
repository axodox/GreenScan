using System;
using Green.Settings;
using Green.Kinect;
using Green.Graphics;
using System.Windows;
using System.IO;
using GreenResources = GreenScan.Properties.Resources;

namespace Green.Scan
{
    public class ScanSettings : SettingManager
    {
        public SettingGroup KinectProperties { get; private set; }
        public EnumSetting<KinectManager.Modes> KinectMode { get; private set; }
        public BooleanSetting NearModeEnabled { get; private set; }
        public BooleanSetting EmitterEnabled { get; private set; }
        public NumericSetting<int> ElevationAngle { get; private set; }

        public SettingGroup PreprocessingProperties { get; private set; }
        public NumericSetting<int> DepthAveraging { get; private set; }
        public NumericSetting<int> DepthGaussIterations { get; private set; }
        public NumericSetting<float> DepthGaussSigma { get; private set; }

        public SettingGroup CameraProperties { get; private set; }
        public MatrixSetting InfraredIntrinsics { get; private set; }
        public MatrixSetting InfraredDistortion { get; private set; }
        public BooleanSetting InfraredDistortionCorrectionEnabled { get; private set; }
        public MatrixSetting DepthToIRMapping { get; private set; }
        public MatrixSetting DepthCoeffs { get; private set; }
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
        public BooleanSetting SaveNoTimestamp { get; private set; }
        public StringSetting SaveLabel { get; private set; }
        public SizeSetting SaveModelResolution { get; private set; }
        public SizeSetting SaveTextureResolution { get; private set; }
        public PathSetting SaveCalibrationDirectory { get; private set; }
        public NumericSetting<int> SaveScalingPower { get; set; }
        public NumericSetting<int> SaveContinousShootingInterval { get; set; }

        public SettingGroup TurntableProperties { get; private set; }
        public EnumSetting<RotatingScanner.Modes> TurntableMode { get; private set; }
        public MatrixSetting TurntableTransform { get; private set; }
        public NumericSetting<int> TurntablePiSteps { get; private set; }
        public BooleanSetting TurntableHasMirror { get; private set; }
        public RectangleSetting TurntableEllipse { get; private set; }
        public RectangleSetting TurntableRectangleA { get; private set; }
        public RectangleSetting TurntableRectangleB { get; private set; }

        public SettingGroup TurntableAxialProperties { get; private set; }
        public EnumSetting<RotatingScanner.AxialViews> TurntableAxialView { get; private set; }
        public NumericSetting<float> TurntableAxialClippingHeight { get; private set; }
        public NumericSetting<float> TurntableAxialClippingRadius { get; private set; }
        public NumericSetting<float> TurntableAxialCoreX { get; private set; }
        public NumericSetting<float> TurntableAxialCoreY { get; private set; }
        public SizeSetting TurntableAxialModelResolution { get; private set; }
        public SizeSetting TurntableAxialTextureResolution { get; private set; }   

        public SettingGroup TurntableVolumetricProperties { get; private set; }
        public NumericSetting<float> TurntableVolumetricCubeSize { get; private set; }
        public NumericSetting<int> TurntableVolumetricCubeResolution { get; private set; }
        public EnumSetting<RotatingScanner.VolumetricViews> TurntableVolumetricView { get; private set; }
        public NumericSetting<float> TurntableVolumetricSlice { get; private set; }
        public NumericSetting<float> TurntableVolumetricThreshold { get; private set; }
        public NumericSetting<float> TurntableVolumetricGradientLimit { get; private set; }

        public SettingSetter TurntableScanningSetter { get; private set; }

        public string GetRawMetadata()
        {
            return ToString(new SettingGroup[] { CameraProperties });
        }

        public string GetTurntableMetadata()
        {
            switch(TurntableMode.Value)
            {
                case RotatingScanner.Modes.OneAxis:
                case RotatingScanner.Modes.TwoAxis:
                    return ToString(new SettingGroup[] { TurntableAxialProperties });
                case RotatingScanner.Modes.Volumetric:
                    return ToString(new SettingGroup[] { TurntableVolumetricProperties });
                default:
                    return "";
            }
        }

        bool RawMetadataLoaded;
        public void LoadRawMetadata(string metadata)
        {
            if (!RawMetadataLoaded)
            {
                CameraProperties.StoreAllValues();
                RawMetadataLoaded = true;
            }
            FromString(metadata);            
        }

        bool TurntableMetadataLoaded;
        public void LoadTurntableMetadata(string metadata)
        {
            if (!TurntableMetadataLoaded)
            {
                TurntableAxialProperties.StoreAllValues();
                TurntableVolumetricProperties.StoreAllValues();
                TurntableMetadataLoaded = true;
            }
            FromString(metadata);
        }

        bool ImportMetadataLoaded;
        public void LoadImportMetadata()
        {
            if (!ImportMetadataLoaded)
            {
                CameraProperties.StoreAllValues();
                CameraProperties.ResetToDefault();
                ImportMetadataLoaded = true;
            }
        }

        public void RestoreMetadata()
        {
            if (RawMetadataLoaded)
            {
                CameraProperties.RestoreAllValues();
                RawMetadataLoaded = false;
            }
            if (TurntableMetadataLoaded)
            {
                TurntableAxialProperties.RestoreAllValues();
                TurntableVolumetricProperties.RestoreAllValues();
                TurntableMetadataLoaded = false;
            }
            if (ImportMetadataLoaded)
            {
                CameraProperties.RestoreAllValues();
                ImportMetadataLoaded = false;
            }
        }

        public ScanSettings()
            : base()
        {
            TurntableScanningSetter = new SettingSetter();

            //Kinect
            KinectProperties = new SettingGroup("Kinect") { FriendlyName = GreenResources.SettingGroupKinect, Footer = GreenResources.SettingGroupKinectFooter, IsHidden = true };
            SettingGroups.Add(KinectProperties);

            KinectMode = new EnumSetting<KinectManager.Modes>("Mode", KinectManager.Modes.DepthAndColor) { FriendlyName = GreenResources.SettingKinectMode, FriendlyOptions = GreenResources.EnumKinectManagerModes.Split('|') };
            DependentAvailability kinectModeIsDepthAndColor = new DependentAvailability(KinectMode, "DepthAndColor");
            DependentAvailability kinectModeWithDepth = new DependentAvailability(KinectMode, "Depth|DepthAndColor");
            KinectProperties.Settings.Add(KinectMode);

            NearModeEnabled = new BooleanSetting("NearModeEnabled", true) { FriendlyName = GreenResources.SettingKinectNearMode, AvailabilityProvider = kinectModeWithDepth };
            EmitterEnabled = new BooleanSetting("EmitterEnabled", true) { FriendlyName = GreenResources.SettingKinectEmitterEnabled, AvailabilityProvider = kinectModeWithDepth };
            ElevationAngle = new NumericSetting<int>("ElevationAngle", 0, -27, 27) { FriendlyName = GreenResources.SettingElevationAngle };
            KinectProperties.Settings.Add(NearModeEnabled);
            KinectProperties.Settings.Add(EmitterEnabled);
            KinectProperties.Settings.Add(ElevationAngle);

            TurntableScanningSetter.Settings.AddRange(KinectProperties.Settings);

            //Preprocessing
            PreprocessingProperties = new SettingGroup("Preprocessing") { FriendlyName = GreenResources.SettingGroupPreprocessing };
            SettingGroups.Add(PreprocessingProperties);

            DepthAveraging = new NumericSetting<int>("DepthAveraging", 1, 1, 256) { FriendlyName = GreenResources.SettingDepthAveraging, AvailabilityProvider = kinectModeIsDepthAndColor };
            DepthGaussIterations = new NumericSetting<int>("DepthGaussIterations", 0, 0, 4) { FriendlyName = GreenResources.SettingDepthGaussIterations, AvailabilityProvider = kinectModeIsDepthAndColor };
            DepthGaussSigma = new NumericSetting<float>("DepthGaussSigma", 1, 0.1f, 4f, 2) { FriendlyName = GreenResources.SettingDepthGaussSigma, AvailabilityProvider = kinectModeIsDepthAndColor };
            PreprocessingProperties.Settings.Add(DepthAveraging);
            PreprocessingProperties.Settings.Add(DepthGaussIterations);
            PreprocessingProperties.Settings.Add(DepthGaussSigma);

            //Camera
            CameraProperties = new SettingGroup("Camera") { FriendlyName = GreenResources.SettingGroupCamera };
            SettingGroups.Add(CameraProperties);

            InfraredIntrinsics = new MatrixSetting("InfraredIntrinsics", new float[,] { { 582.0813f, 0f, 309.8195f }, { 0f, 582.865f, 234.2108f }, { 0f, 0f, 1f } }) { FriendlyName = GreenResources.SettingInfraredIntrinsics, AvailabilityProvider = kinectModeIsDepthAndColor };
            InfraredDistortion = new MatrixSetting("InfraredDistortion", new float[,] { { 0f, 0f }, { 0f, 0f } }) { FriendlyName = GreenResources.SettingInfraredDistortion, AvailabilityProvider = kinectModeIsDepthAndColor };
            InfraredDistortionCorrectionEnabled = new BooleanSetting("InfraredDistortionCorrectionEnabled", false) { FriendlyName = GreenResources.SettingInfraredDistortionCorrectionEnabled, AvailabilityProvider = kinectModeIsDepthAndColor };
            DepthToIRMapping = new MatrixSetting("DepthToIRMapping", new float[,] { { 1f, 0f, 0f }, { 0f, 1f, 0f }, { 0f, 0f, 1f } }) { FriendlyName = GreenResources.SettingDepthToIRMapping, AvailabilityProvider = kinectModeIsDepthAndColor };
            DepthCoeffs = new MatrixSetting("DepthCoeffs", new float[,] { { 0.125e-3f, 0f } }) { FriendlyName = GreenResources.SettingDepthCoeffs, AvailabilityProvider = kinectModeIsDepthAndColor };
            ColorIntrinsics = new MatrixSetting("ColorIntrinsics", new float[,] { { 523.279f, 0f, 303.9468f }, { 0f, 523.1638f, 239.1431f }, { 0f, 0f, 1f } }) { FriendlyName = GreenResources.SettingColorIntrinsics, AvailabilityProvider = kinectModeIsDepthAndColor };
            ColorRemapping = new MatrixSetting("ColorRemapping", new float[,] { { 1f, 0f, 0f }, { 0f, 1f, 0f }, { 0f, 0f, 1f } }) { FriendlyName = GreenResources.SettingColorRemapping, AvailabilityProvider = kinectModeIsDepthAndColor };
            ColorExtrinsics = new MatrixSetting("ColorExtrinsics", new float[,] { { 0.999967f, 0.003969562f, 0.00709248f, 0.02473051f }, { -0.003896888f, 0.9999401f, -0.01023114f, -0.000453843f }, { -0.007132668f, 0.01020316f, 0.9999225f, 0.003119022f }, { 0f, 0f, 0f, 1f } }) { FriendlyName = GreenResources.SettingColorExtrinsics, AvailabilityProvider = kinectModeIsDepthAndColor };
            CameraProperties.Settings.Add(InfraredIntrinsics);
            CameraProperties.Settings.Add(InfraredDistortion);
            CameraProperties.Settings.Add(InfraredDistortionCorrectionEnabled);
            CameraProperties.Settings.Add(DepthToIRMapping);
            CameraProperties.Settings.Add(DepthCoeffs);
            CameraProperties.Settings.Add(ColorIntrinsics);
            CameraProperties.Settings.Add(ColorRemapping);
            CameraProperties.Settings.Add(ColorExtrinsics);

            ColorDispositionX = new NumericSetting<int>("ColorDispositionX", 0, -32, 32) { FriendlyName = GreenResources.SettingColorDispositionX, AvailabilityProvider = kinectModeIsDepthAndColor };
            ColorDispositionY = new NumericSetting<int>("ColorDispositionY", 0, -32, 32) { FriendlyName = GreenResources.SettingColorDispositionY, AvailabilityProvider = kinectModeIsDepthAndColor };
            ColorScaleX = new NumericSetting<float>("ColorScaleX", 1f, 0.8f, 1.2f, 2) { FriendlyName = GreenResources.SettingColorScaleX, AvailabilityProvider = kinectModeIsDepthAndColor };
            ColorScaleY = new NumericSetting<float>("ColorScaleY", 1f, 0.8f, 1.2f, 2) { FriendlyName = GreenResources.SettingColorScaleY, AvailabilityProvider = kinectModeIsDepthAndColor };
            CameraProperties.Settings.Add(ColorDispositionX);
            CameraProperties.Settings.Add(ColorDispositionY);
            CameraProperties.Settings.Add(ColorScaleX);
            CameraProperties.Settings.Add(ColorScaleY);

            TurntableScanningSetter.Settings.AddRange(CameraProperties.Settings);

            //View
            ViewProperties = new SettingGroup("View") { FriendlyName = GreenResources.SettingGroupView };
            SettingGroups.Add(ViewProperties);

            TranslationX = new NumericSetting<float>("TranslationX", 0f, -1f, 1f, 2) { FriendlyName = GreenResources.SettingTranslationX, AvailabilityProvider = kinectModeIsDepthAndColor };
            TranslationY = new NumericSetting<float>("TranslationY", 0f, -1f, 1f, 2) { FriendlyName = GreenResources.SettingTranslationY, AvailabilityProvider = kinectModeIsDepthAndColor };
            TranslationZ = new NumericSetting<float>("TranslationZ", 1.5f, 0f, 3f, 2) { FriendlyName = GreenResources.SettingTranslationZ, AvailabilityProvider = kinectModeIsDepthAndColor };
            ViewProperties.Settings.Add(TranslationX);
            ViewProperties.Settings.Add(TranslationY);
            ViewProperties.Settings.Add(TranslationZ);

            RotationX = new NumericSetting<float>("RotationX", 0f, -180f, 180f, 2) { FriendlyName = GreenResources.SettingRotationX, AvailabilityProvider = kinectModeIsDepthAndColor };
            RotationY = new NumericSetting<float>("RotationY", 0f, -180f, 180f, 2) { FriendlyName = GreenResources.SettingRotationY, AvailabilityProvider = kinectModeIsDepthAndColor };
            RotationZ = new NumericSetting<float>("RotationZ", 0f, -180f, 180f, 2) { FriendlyName = GreenResources.SettingRotationZ, AvailabilityProvider = kinectModeIsDepthAndColor };
            ViewProperties.Settings.Add(RotationX);
            ViewProperties.Settings.Add(RotationY);
            ViewProperties.Settings.Add(RotationZ);

            Scale = new NumericSetting<float>("Scale", 1f, 0f, 8f, 2) { FriendlyName = GreenResources.SettingScale };
            MoveX = new NumericSetting<float>("MoveX", 0f, -1f, 1f, 2) { FriendlyName = GreenResources.SettingMoveX };
            MoveY = new NumericSetting<float>("MoveY", 0f, -1f, 1f, 2) { FriendlyName = GreenResources.SettingMoveY };
            Rotation = new NumericSetting<int>("Rotation", 0, 0, 3) { FriendlyName = GreenResources.SettingRotation };
            ViewProperties.Settings.Add(Scale);
            ViewProperties.Settings.Add(MoveX);
            ViewProperties.Settings.Add(MoveY);
            ViewProperties.Settings.Add(Rotation);

            //Shading
            ShadingProperties = new SettingGroup("Shading") { FriendlyName = GreenResources.SettingGroupShading };
            SettingGroups.Add(ShadingProperties);

            UseModuleShading = new BooleanSetting("ModuleShading", false) { FriendlyName = GreenResources.SettingUseModuleShading, AvailabilityProvider = kinectModeIsDepthAndColor };
            ShadingMode = new EnumSetting<GraphicsCanvas.ShadingModes>("ShadingMode", GraphicsCanvas.ShadingModes.Rainbow) { FriendlyName = GreenResources.SettingShadingMode, AvailabilityProvider = kinectModeIsDepthAndColor, FriendlyOptions = GreenResources.EnumGraphicsCanvasShadingModes.Split('|') };
            DepthMaximum = new NumericSetting<float>("DepthMaximum", 8f, 0f, 8f, 2) { FriendlyName = GreenResources.SettingDepthMaximum, AvailabilityProvider = kinectModeIsDepthAndColor };
            DepthMinimum = new NumericSetting<float>("DepthMinimum", 0.4f, 0.4f, 8f, 2) { FriendlyName = GreenResources.SettingDepthMinimum, AvailabilityProvider = kinectModeIsDepthAndColor };
            ShadingPeriode = new NumericSetting<float>("ShadingPeriode", 1f, 0.01f, 2f, 2) { FriendlyName = GreenResources.SettingShadingPeriode };
            ShadingPhase = new NumericSetting<float>("ShadingPhase", 0f, 0f, 1f, 2) { FriendlyName = GreenResources.SettingShadingPhase };
            TriangleRemoveLimit = new NumericSetting<float>("TriangleRemoveLimit", 0.0024f, 0.0001f, 0.004f, 4) { FriendlyName = GreenResources.SettingTriangleRemoveLimit, AvailabilityProvider = kinectModeIsDepthAndColor };
            WireframeShading = new BooleanSetting("WireframeShading", false) { FriendlyName = GreenResources.SettingWireframeShading, AvailabilityProvider = kinectModeIsDepthAndColor };
            ShadingProperties.Settings.Add(UseModuleShading);
            ShadingProperties.Settings.Add(ShadingMode);
            ShadingProperties.Settings.Add(DepthMaximum);
            ShadingProperties.Settings.Add(DepthMinimum);
            ShadingProperties.Settings.Add(ShadingPeriode);
            ShadingProperties.Settings.Add(ShadingPhase);
            ShadingProperties.Settings.Add(TriangleRemoveLimit);
            ShadingProperties.Settings.Add(WireframeShading);

            //Performance
            PerformanceProperties = new SettingGroup("Performance") { FriendlyName = GreenResources.SettingGroupPerformance };
            SettingGroups.Add(PerformanceProperties);

            TriangleGridResolution = new SizeSetting("TriangleGridResolution", 640, 480, 16, 12, 640, 480) { FriendlyName = GreenResources.SettingTriangleGridResolution, AvailabilityProvider = kinectModeIsDepthAndColor };
            PerformanceProperties.Settings.Add(TriangleGridResolution);

            //Save
            SaveProperties = new SettingGroup("Save") { FriendlyName = GreenResources.SettingGroupSave };
            SettingGroups.Add(SaveProperties);

            SaveDirectory = new PathSetting("Directory", "") { FriendlyName = GreenResources.SettingSaveDirectory };
            SaveLabel = new StringSetting("Label", "", Path.GetInvalidFileNameChars()) { FriendlyName = GreenResources.SettingSaveLabel };
            SaveNoTimestamp = new BooleanSetting("NoTimestamp", false) { FriendlyName = GreenResources.SettingSaveNoTimestamp };
            SaveModelResolution = new SizeSetting("ModelResolution", 640, 480, 8, 8, 640, 480) { FriendlyName = GreenResources.SettingSaveModelResolution, AvailabilityProvider = kinectModeIsDepthAndColor };
            SaveTextureResolution = new SizeSetting("TextureResolution", 640, 480, 8, 8, 1024, 1024) { FriendlyName = GreenResources.SettingSaveTextureResolution, AvailabilityProvider = kinectModeIsDepthAndColor };
            SaveCalibrationDirectory = new PathSetting("CalibrationDirectory", "") { FriendlyName = GreenResources.SettingSaveCalibrationDirectory, IsHidden = true };
            SaveScalingPower = new NumericSetting<int>("ScalingPower", 0, -3, 6) { FriendlyName = GreenResources.SettingSaveScalingPower, AvailabilityProvider = kinectModeIsDepthAndColor };
            SaveProperties.Settings.Add(SaveDirectory);
            SaveProperties.Settings.Add(SaveLabel);
            SaveProperties.Settings.Add(SaveNoTimestamp);
            SaveProperties.Settings.Add(SaveModelResolution);
            SaveProperties.Settings.Add(SaveTextureResolution);
            SaveProperties.Settings.Add(SaveCalibrationDirectory);
            SaveProperties.Settings.Add(SaveScalingPower);

            SaveContinousShootingInterval = new NumericSetting<int>("SaveContinousShootingInterval", 100, 0, 1000);
            SaveProperties.Settings.Add(SaveContinousShootingInterval);

            //Turntable
            TurntableProperties = new SettingGroup("Turntable") { FriendlyName = GreenResources.SettingGroupTurntable, IsHidden = true };
            SettingGroups.Add(TurntableProperties);

            TurntableMode = new EnumSetting<RotatingScanner.Modes>("Mode", RotatingScanner.Modes.TwoAxis) { FriendlyName = GreenResources.SettingTurntableMode, FriendlyOptions = GreenResources.EnumRotatingScannerModes.Split('|') };
            DependentAvailability turntableModeIsVolumetric = new DependentAvailability(TurntableMode, "Volumetric");
            DependentAvailability turntableModeIsAxial = new DependentAvailability(TurntableMode, "OneAxis|TwoAxis");
            TurntableProperties.Settings.Add(TurntableMode);
            TurntableTransform = new MatrixSetting("TurntableTransform", new float[,] { { 1f, 0f, 0f, 0f }, { 0f, 1f, 0f, 0f }, { 0f, 0f, 1f, 0f }, { 0f, 0f, 0f, 1f } }) { FriendlyName = GreenResources.SettingTurntableTransform, IsHidden = true };
            TurntableProperties.Settings.Add(TurntableTransform);
            TurntablePiSteps = new NumericSetting<int>("PiSteps", 10934, 5000, 20000) { FriendlyName = GreenResources.SettingTurntablePiSteps };
            TurntableProperties.Settings.Add(TurntablePiSteps);
            TurntableHasMirror = new BooleanSetting("HasMirror", true) { FriendlyName = GreenResources.SettingTurntableHasMirror };
            TurntableProperties.Settings.Add(TurntableHasMirror);
            TurntableEllipse = new RectangleSetting("SelectionEllipse", new Rect(0d, 0d, 0d, 0d)) { IsHidden = true };
            TurntableRectangleA = new RectangleSetting("SelectionRectangleA", new Rect(0d, 0d, 0d, 0d)) { IsHidden = true };
            TurntableRectangleB = new RectangleSetting("SelectionRectangleB", new Rect(0d, 0d, 0d, 0d)) { IsHidden = true };
            TurntableProperties.Settings.Add(TurntableEllipse);
            TurntableProperties.Settings.Add(TurntableRectangleA);
            TurntableProperties.Settings.Add(TurntableRectangleB);

            TurntableScanningSetter.Settings.AddRange(new Setting[] { TurntableMode, TurntableHasMirror });

            //Turntable - Axial
            TurntableAxialProperties = new SettingGroup("TurntableAxial") { FriendlyName = GreenResources.SettingGroupTurntableAxial, IsHidden = true };
            SettingGroups.Add(TurntableAxialProperties);

            TurntableAxialView = new EnumSetting<RotatingScanner.AxialViews>("AxialView", RotatingScanner.AxialViews.Overlay) { FriendlyName = GreenResources.SettingTurntableAxialView, AvailabilityProvider = turntableModeIsAxial, FriendlyOptions = GreenResources.EnumRotatingScannerViews.Split('|') };
            TurntableAxialProperties.Settings.Add(TurntableAxialView);
            TurntableAxialClippingHeight = new NumericSetting<float>("ClippingHeight", 0.5f, 0f, 2f, 3) { FriendlyName = GreenResources.SettingTurntableAxialClippingHeight, AvailabilityProvider = turntableModeIsAxial };
            TurntableAxialClippingRadius = new NumericSetting<float>("ClippingRadius", 0.3f, 0f, 2f, 3) { FriendlyName = GreenResources.SettingTurntableAxialClippingRadius, AvailabilityProvider = turntableModeIsAxial };
            TurntableAxialProperties.Settings.Add(TurntableAxialClippingHeight);
            TurntableAxialProperties.Settings.Add(TurntableAxialClippingRadius);
            TurntableAxialCoreX = new NumericSetting<float>("CoreX", 0.11f, 0f, 0.5f, 3) { FriendlyName = GreenResources.SettingTurntableAxialCoreX, AvailabilityProvider = turntableModeIsAxial };
            TurntableAxialCoreY = new NumericSetting<float>("CoreY", -0.11f, -0.5f, 0.5f, 3) { FriendlyName = GreenResources.SettingTurntableAxialCoreY, AvailabilityProvider = turntableModeIsAxial };
            TurntableAxialProperties.Settings.Add(TurntableAxialCoreX);
            TurntableAxialProperties.Settings.Add(TurntableAxialCoreY);
            TurntableAxialModelResolution = new SizeSetting("ModelResolution", 640, 480, 64, 64, 1024, 1024) { FriendlyName = GreenResources.SettingTurntableSaveModelResolution, AvailabilityProvider = turntableModeIsAxial };
            TurntableAxialTextureResolution = new SizeSetting("TextureResolution", 640, 480, 64, 64, 1024, 1024) { FriendlyName = GreenResources.SettingTurntableSaveTextureResolution, AvailabilityProvider = turntableModeIsAxial };
            TurntableAxialProperties.Settings.Add(TurntableAxialModelResolution);
            TurntableAxialProperties.Settings.Add(TurntableAxialTextureResolution);

            TurntableScanningSetter.Settings.AddRange(new Setting[] { TurntableAxialClippingHeight, TurntableAxialClippingRadius, TurntableAxialCoreX, TurntableAxialCoreY, TurntableAxialModelResolution, TurntableAxialTextureResolution });

            //Turntable - Volumetric
            TurntableVolumetricProperties = new SettingGroup("TurntableVolumetric") { FriendlyName = GreenResources.SettingGroupTurntableVolumetric, IsHidden = true };
            SettingGroups.Add(TurntableVolumetricProperties);

            TurntableVolumetricCubeSize = new NumericSetting<float>("CubeSize", 30, 10, 50) { FriendlyName = GreenResources.SettingTurntableVolumetricCubeSize, AvailabilityProvider = turntableModeIsVolumetric };
            TurntableVolumetricCubeResolution = new NumericSetting<int>("CubeResolution", 128, 16, 512) { FriendlyName = GreenResources.SettingTurntableVolumetricCubeResolution, AvailabilityProvider = turntableModeIsVolumetric };
            TurntableVolumetricProperties.Settings.Add(TurntableVolumetricCubeSize);
            TurntableVolumetricProperties.Settings.Add(TurntableVolumetricCubeResolution);
            TurntableVolumetricView = new EnumSetting<RotatingScanner.VolumetricViews>("VolumetricView", RotatingScanner.VolumetricViews.Overlay) { FriendlyName = GreenResources.SettingTurntableVolumetricView, AvailabilityProvider = turntableModeIsVolumetric, FriendlyOptions = GreenResources.EnumRotatingScannerVolumetricViews.Split('|') };
            DependentAvailability turntableVolumetricSliceView = new DependentAvailability(new Setting[] { TurntableMode, TurntableVolumetricView }, new string[] { "Volumetric", "Slice" });
            TurntableVolumetricProperties.Settings.Add(TurntableVolumetricView);
            TurntableVolumetricSlice = new NumericSetting<float>("Slice", 0, 0, 1, 2) { FriendlyName = GreenResources.SettingTurntableVolumetricSlice, AvailabilityProvider = turntableVolumetricSliceView };
            TurntableVolumetricThreshold = new NumericSetting<float>("Threshold", 0.1f, 0, 1, 2) { FriendlyName = GreenResources.SettingTurntableVolumetricThreshold, AvailabilityProvider = turntableModeIsVolumetric };
            TurntableVolumetricProperties.Settings.Add(TurntableVolumetricSlice);
            TurntableVolumetricProperties.Settings.Add(TurntableVolumetricThreshold);
            TurntableVolumetricGradientLimit = new NumericSetting<float>("GradientLimit", 0.05f, 0f, 0.2f, 4) { FriendlyName = GreenResources.SettingTurntableVolumetricGradientLimit, AvailabilityProvider = turntableModeIsVolumetric };
            TurntableVolumetricProperties.Settings.Add(TurntableVolumetricGradientLimit);

            TurntableScanningSetter.Settings.AddRange(new Setting[] { TurntableVolumetricCubeSize, TurntableVolumetricCubeResolution });
        }
    }
}