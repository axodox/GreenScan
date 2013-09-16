﻿using System;
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

        public SettingGroup TurntableProperties { get; private set; }
        public RectangleSetting TurntableEllipse { get; private set; }
        public RectangleSetting TurntableRectangle { get; private set; }
        public EnumSetting<RotatingScanner.Modes> TurntableMode { get; private set; }
        public MatrixSetting TurntableTransform { get; private set; }
        public NumericSetting<float> TurntableClippingHeight { get; private set; }
        public NumericSetting<float> TurntableClippingRadius { get; private set; }
        public NumericSetting<float> TurntableCoreX { get; private set; }
        public NumericSetting<float> TurntableCoreY { get; private set; }
        public SizeSetting TurntableModelResolution { get; private set; }
        public SizeSetting TurntableTextureResolution { get; private set; }
        public EnumSetting<RotatingScanner.Views> TurntableAxialView { get; private set; }
        public NumericSetting<int> TurntablePiSteps { get; private set; }
        public NumericSetting<float> TurntableCubeSize { get; private set; }
        public NumericSetting<int> TurntableCubeResolution { get; private set; }
        public EnumSetting<RotatingScanner.VolumetricViews> TurntableVolumetricView { get; private set; }
        public NumericSetting<float> TurntableSlice { get; private set; }
        public NumericSetting<float> TurntableThreshold { get; private set; }
        public NumericSetting<float> TurntableGradientLimit { get; private set; }
        public BooleanSetting TurntableHasMirror { get; private set; }

        public ScanSettings()
            : base()
        {
            //Kinect
            KinectProperties = new SettingGroup("Kinect") { FriendlyName = GreenResources.SettingGroupKinect, Footer = GreenResources.SettingGroupKinectFooter };
            SettingGroups.Add(KinectProperties);

            KinectMode = new EnumSetting<KinectManager.Modes>("Mode", KinectManager.Modes.DepthAndColor) { FriendlyName = GreenResources.SettingKinectMode, FriendlyOptions = GreenResources.EnumKinectManagerModes.Split('|') };
            DependentAvailability kinectModeIsDepthAndColor = new DependentAvailability(KinectMode, "DepthAndColor");
            DependentAvailability kinectModeWithDepth = new DependentAvailability(KinectMode, "Depth|DepthAndColor");
            KinectProperties.Settings.Add(KinectMode);

            NearModeEnabled = new BooleanSetting("NearModeEnabled", true) { FriendlyName = GreenResources.SettingKinectNearMode, AvailabilityProvider = kinectModeWithDepth };
            EmitterEnabled = new BooleanSetting("EmitterEnabled", true) { FriendlyName = GreenResources.SettingKinectEmitterEnabled, AvailabilityProvider = kinectModeWithDepth };
            KinectProperties.Settings.Add(NearModeEnabled);
            KinectProperties.Settings.Add(EmitterEnabled);

            //Preprocessing
            PreprocessingProperties = new SettingGroup("Preprocessing") { FriendlyName = GreenResources.SettingGroupPreprocessing };
            SettingGroups.Add(PreprocessingProperties);

            DepthAveraging = new NumericSetting<int>("DepthAveraging", 1, 1, 32) { FriendlyName = GreenResources.SettingDepthAveraging, AvailabilityProvider = kinectModeIsDepthAndColor };
            DepthGaussIterations = new NumericSetting<int>("DepthGaussIterations", 0, 0, 4) { FriendlyName = GreenResources.SettingDepthGaussIterations, AvailabilityProvider = kinectModeIsDepthAndColor };
            DepthGaussSigma = new NumericSetting<float>("DepthGaussSigma", 1, 0.1f, 4f, 2) { FriendlyName = GreenResources.SettingDepthGaussSigma, AvailabilityProvider = kinectModeIsDepthAndColor };
            PreprocessingProperties.Settings.Add(DepthAveraging);
            PreprocessingProperties.Settings.Add(DepthGaussIterations);
            PreprocessingProperties.Settings.Add(DepthGaussSigma);

            //Camera
            CameraProperties = new SettingGroup("Camera") { FriendlyName = GreenResources.SettingGroupCamera };
            SettingGroups.Add(CameraProperties);

            InfraredIntrinsics = new MatrixSetting("InfraredIntrinsics", new float[,] { { 1161.96f, 0f, 639.8654f }, { 0f, 1169.649f, 521.4605f }, { 0f, 0f, 1f } }) { FriendlyName = GreenResources.SettingInfraredIntrinsics, AvailabilityProvider = kinectModeIsDepthAndColor };
            InfraredDistortion = new MatrixSetting("InfraredDistortion", new float[,] { { 0.143542f, -0.244779f }, { 0.003839f, -0.000318f } }) { FriendlyName = GreenResources.SettingInfraredDistortion, AvailabilityProvider = kinectModeIsDepthAndColor };
            InfraredDistortionCorrectionEnabled = new BooleanSetting("InfraredDistortionCorrectionEnabled", false) { FriendlyName = GreenResources.SettingInfraredDistortionCorrectionEnabled, AvailabilityProvider = kinectModeIsDepthAndColor };
            DepthToIRMapping = new MatrixSetting("DepthToIRMapping", new float[,] { { 2f, 0f, 16f }, { 0f, 2f, 17f }, { 0f, 0f, 1f } }) { FriendlyName = GreenResources.SettingDepthToIRMapping, AvailabilityProvider = kinectModeIsDepthAndColor };
            DepthCoeffs = new MatrixSetting("DepthCoeffs", new float[,] { { 0.125e-3f, 0f } }) { FriendlyName = GreenResources.SettingDepthCoeffs, AvailabilityProvider = kinectModeIsDepthAndColor };
            ColorIntrinsics = new MatrixSetting("ColorIntrinsics", new float[,] { { 1051.45f, 0f, 641.1544f }, { 0f, 1053.781f, 521.7905f }, { 0f, 0f, 1f } }) { FriendlyName = GreenResources.SettingColorIntrinsics, AvailabilityProvider = kinectModeIsDepthAndColor };
            ColorRemapping = new MatrixSetting("ColorRemapping", new float[,] { { 2f, 0f, 2f }, { 0f, 2f, 0f }, { 0f, 0f, 1f } }) { FriendlyName = GreenResources.SettingColorRemapping, AvailabilityProvider = kinectModeIsDepthAndColor };
            ColorExtrinsics = new MatrixSetting("ColorExtrinsics", new float[,] { { 0.999946f, -0.006657f, 0.00794f, -0.025955f }, { 0.006679f, 0.999974f, -0.002686f, -0.000035f }, { -0.007922f, 0.002739f, 0.999965f, 0.005283f }, { 0f, 0f, 0f, 1f } }) { FriendlyName = GreenResources.SettingColorExtrinsics, AvailabilityProvider = kinectModeIsDepthAndColor };
            CameraProperties.Settings.Add(InfraredIntrinsics);
            CameraProperties.Settings.Add(InfraredDistortion);
            CameraProperties.Settings.Add(InfraredDistortionCorrectionEnabled);
            CameraProperties.Settings.Add(DepthToIRMapping);
            CameraProperties.Settings.Add(DepthCoeffs);
            CameraProperties.Settings.Add(ColorIntrinsics);
            CameraProperties.Settings.Add(ColorRemapping);
            CameraProperties.Settings.Add(ColorExtrinsics);

            ColorDispositionX = new NumericSetting<int>("ColorDispositionX", -8, -32, 32) { FriendlyName = GreenResources.SettingColorDispositionX, AvailabilityProvider = kinectModeIsDepthAndColor };
            ColorDispositionY = new NumericSetting<int>("ColorDispositionY", 8, -32, 32) { FriendlyName = GreenResources.SettingColorDispositionY, AvailabilityProvider = kinectModeIsDepthAndColor };
            ColorScaleX = new NumericSetting<float>("ColorScaleX", 1.01f, 0.8f, 1.2f, 2) { FriendlyName = GreenResources.SettingColorScaleX, AvailabilityProvider = kinectModeIsDepthAndColor };
            ColorScaleY = new NumericSetting<float>("ColorScaleY", 1f, 0.8f, 1.2f, 2) { FriendlyName = GreenResources.SettingColorScaleY, AvailabilityProvider = kinectModeIsDepthAndColor };
            CameraProperties.Settings.Add(ColorDispositionX);
            CameraProperties.Settings.Add(ColorDispositionY);
            CameraProperties.Settings.Add(ColorScaleX);
            CameraProperties.Settings.Add(ColorScaleY);

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
            SaveModelResolution = new SizeSetting("ModelResolution", 640, 480, 8, 8, 640, 480) { FriendlyName = GreenResources.SettingSaveModelResolution };
            SaveTextureResolution = new SizeSetting("TextureResolution", 640, 480, 8, 8, 1024, 1024) { FriendlyName = GreenResources.SettingSaveTextureResolution };
            SaveCalibrationDirectory = new PathSetting("CalibrationDirectory", "") { FriendlyName = GreenResources.SettingSaveCalibrationDirectory, IsHidden = true };
            SaveScalingPower = new NumericSetting<int>("ScalingPower", 0, -3, 6) { FriendlyName = GreenResources.SettingSaveScalingPower };
            SaveProperties.Settings.Add(SaveDirectory);
            SaveProperties.Settings.Add(SaveLabel);
            SaveProperties.Settings.Add(SaveNoTimestamp);
            SaveProperties.Settings.Add(SaveModelResolution);
            SaveProperties.Settings.Add(SaveTextureResolution);
            SaveProperties.Settings.Add(SaveCalibrationDirectory);
            SaveProperties.Settings.Add(SaveScalingPower);

            //Turntable
            TurntableProperties = new SettingGroup("Turntable") { FriendlyName = GreenResources.SettingGroupTurntable, IsHidden = true };
            SettingGroups.Add(TurntableProperties);

            TurntableMode = new EnumSetting<RotatingScanner.Modes>("Mode", RotatingScanner.Modes.TwoAxis) { FriendlyName = GreenResources.SettingTurntableMode, FriendlyOptions = GreenResources.EnumRotatingScannerModes.Split('|') };
            DependentAvailability turntableModeIsVolumetric = new DependentAvailability(TurntableMode, "Volumetric");
            DependentAvailability turntableModeIsAxial = new DependentAvailability(TurntableMode, "OneAxis|TwoAxis");
            TurntableProperties.Settings.Add(TurntableMode);

            TurntableVolumetricView = new EnumSetting<RotatingScanner.VolumetricViews>("VolumetricView", RotatingScanner.VolumetricViews.Overlay) { FriendlyName = GreenResources.SettingTurntableVolumetricView, AvailabilityProvider = turntableModeIsVolumetric, FriendlyOptions = GreenResources.EnumRotatingScannerVolumetricViews.Split('|') };
            DependentAvailability turntableVolumetricSliceView = new DependentAvailability(new Setting[] { TurntableMode, TurntableVolumetricView }, new string[] { "Volumetric", "Slice" });
            TurntableSlice = new NumericSetting<float>("Slice", 0, 0, 1, 2) { FriendlyName = GreenResources.SettingTurntableSlice, AvailabilityProvider = turntableVolumetricSliceView };
            TurntableThreshold = new NumericSetting<float>("Threshold", 0, 0, 1, 2) { FriendlyName = GreenResources.SettingTurntableThreshold, AvailabilityProvider = turntableModeIsVolumetric };
            TurntableCubeSize = new NumericSetting<float>("CubeSize", 30, 10, 50) { FriendlyName = GreenResources.SettingTurntableCubeSize, AvailabilityProvider = turntableModeIsVolumetric };
            TurntableCubeResolution = new NumericSetting<int>("CubeResolution", 128, 16, 512) { FriendlyName = GreenResources.SettingTurntableCubeResolution, AvailabilityProvider = turntableModeIsVolumetric };
            TurntableProperties.Settings.Add(TurntableVolumetricView);
            TurntableProperties.Settings.Add(TurntableSlice);
            TurntableProperties.Settings.Add(TurntableThreshold);
            TurntableProperties.Settings.Add(TurntableCubeSize);
            TurntableProperties.Settings.Add(TurntableCubeResolution);

            TurntableAxialView = new EnumSetting<RotatingScanner.Views>("AxialView", RotatingScanner.Views.Overlay) { FriendlyName = GreenResources.SettingTurntableAxialView, AvailabilityProvider = turntableModeIsAxial, FriendlyOptions = GreenResources.EnumRotatingScannerViews.Split('|') };
            TurntableProperties.Settings.Add(TurntableAxialView);

            TurntableCoreX = new NumericSetting<float>("CoreX", 0.11f, 0f, 0.5f, 3) { FriendlyName = GreenResources.SettingTurntableCoreX, AvailabilityProvider = turntableModeIsAxial };
            TurntableCoreY = new NumericSetting<float>("CoreY", -0.11f, -0.5f, 0.5f, 3) { FriendlyName = GreenResources.SettingTurntableCoreY, AvailabilityProvider = turntableModeIsAxial };
            TurntableProperties.Settings.Add(TurntableCoreX);
            TurntableProperties.Settings.Add(TurntableCoreY);

            TurntableClippingHeight = new NumericSetting<float>("ClippingHeight", 0.5f, 0f, 2f, 3) { FriendlyName = GreenResources.SettingTurntableClippingHeight, AvailabilityProvider = turntableModeIsAxial };
            TurntableClippingRadius = new NumericSetting<float>("ClippingRadius", 0.3f, 0f, 2f, 3) { FriendlyName = GreenResources.SettingTurntableClippingRadius, AvailabilityProvider = turntableModeIsAxial };
            TurntableProperties.Settings.Add(TurntableClippingHeight);
            TurntableProperties.Settings.Add(TurntableClippingRadius);

            TurntableModelResolution = new SizeSetting("ModelResolution", 640, 480, 64, 64, 1024, 1024) { FriendlyName = GreenResources.SettingTurntableModelResolution, AvailabilityProvider = turntableModeIsAxial };
            TurntableTextureResolution = new SizeSetting("TextureResolution", 640, 480, 64, 64, 1024, 1024) { FriendlyName = GreenResources.SettingTurntableTextureResolution, AvailabilityProvider = turntableModeIsAxial };
            TurntableProperties.Settings.Add(TurntableModelResolution);
            TurntableProperties.Settings.Add(TurntableTextureResolution);

            TurntableGradientLimit = new NumericSetting<float>("GradientLimit", 0.05f, 0f, 0.2f, 4);
            TurntableHasMirror = new BooleanSetting("HasMirror", true) { FriendlyName = GreenResources.SettingTurntableHasMirror };
            TurntableProperties.Settings.Add(TurntableHasMirror);
            TurntableProperties.Settings.Add(TurntableGradientLimit);

            TurntableTransform = new MatrixSetting("TurntableTransform", new float[,] { { 1f, 0f, 0f, 0f }, { 0f, 1f, 0f, 0f }, { 0f, 0f, 1f, 0f }, { 0f, 0f, 0f, 1f } }) { FriendlyName = GreenResources.SettingTurntableTransform, IsHidden = true };
            TurntableProperties.Settings.Add(TurntableTransform);

            TurntableEllipse = new RectangleSetting("SelectionEllipse", new Rect(0d, 0d, 0d, 0d)) { IsHidden = true };
            TurntableRectangle = new RectangleSetting("SelectionRectangle", new Rect(0d, 0d, 0d, 0d)) { IsHidden = true };
            TurntablePiSteps = new NumericSetting<int>("PiSteps", 10989, 360, 1000000) { IsHidden = true };
            TurntableProperties.Settings.Add(TurntableEllipse);
            TurntableProperties.Settings.Add(TurntableRectangle);
            TurntableProperties.Settings.Add(TurntablePiSteps);
        }
    }
}