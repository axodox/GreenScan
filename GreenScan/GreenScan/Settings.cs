using Green.Settings;
using Green.Kinect;

namespace Green.Scan
{
    class ScanSettings : SettingManager
    {
        public SettingGroup KinectProperties { get; private set; }
        public EnumSetting<KinectManager.Modes> KinectMode { get; private set; }

        public SettingGroup CameraProperties { get; private set; }
        public MatrixSetting DepthToIRMapping { get; private set; }
        public MatrixSetting InfraredCameraMatrix { get; private set; }

        public SettingGroup ViewProperties { get; private set; }
        public NumericSetting<float> TranslationX { get; private set; }
        public NumericSetting<float> TranslationY { get; private set; }
        public NumericSetting<float> TranslationZ { get; private set; }
        public NumericSetting<float> RotationX { get; private set; }
        public NumericSetting<float> RotationY { get; private set; }
        public NumericSetting<float> RotationZ { get; private set; }

        public SettingGroup ShadingProperties { get; private set; }
        public NumericSetting<float> DepthLimit { get; private set; }

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

            InfraredCameraMatrix = new MatrixSetting("InfraredCameraMatrix", new float[,] { { 1161.959596f, 0f, 639.865400f }, { 0f, 1169.649383f, 521.460524f }, { 0f, 0f, 1f } }) { FriendlyName = "Infrared camera matrix" };
            DepthToIRMapping = new MatrixSetting("DepthToIRMapping", new float[,] { { 2f, 0f, 16f }, { 0f, 2f, 17f }, { 0f, 0f, 1f } }) { FriendlyName = "Depth to IR mapping" };
            CameraProperties.Settings.Add(InfraredCameraMatrix);
            CameraProperties.Settings.Add(DepthToIRMapping);

            //View
            ViewProperties = new SettingGroup("View") { FriendlyName = "View" };
            SettingGroups.Add(ViewProperties);

            TranslationX = new NumericSetting<float>("TranslationX", 0f, -1f, 1f, 2) { FriendlyName = "Translation X (meters)" };
            TranslationY = new NumericSetting<float>("TranslationY", 0f, -1f, 1f, 2) { FriendlyName = "Translation Y (meters)" };
            TranslationZ = new NumericSetting<float>("TranslationZ", 1.5f, 0f, 3f, 2) { FriendlyName = "Translation Z (meters)" };
            ViewProperties.Settings.Add(TranslationX);
            ViewProperties.Settings.Add(TranslationY);
            ViewProperties.Settings.Add(TranslationZ); 

            RotationX = new NumericSetting<float>("RotationX", 0f, -45f, 45f, 2) { FriendlyName = "Rotation X (degrees)" };
            RotationY = new NumericSetting<float>("RotationY", 0f, -45f, 45f, 2) { FriendlyName = "Rotation Y (degrees)" };
            RotationZ = new NumericSetting<float>("RotationZ", 0f, -45f, 45f, 2) { FriendlyName = "Rotation Z (degrees)" };
            ViewProperties.Settings.Add(RotationX);
            ViewProperties.Settings.Add(RotationY);
            ViewProperties.Settings.Add(RotationZ);

            //Shading
            ShadingProperties = new SettingGroup("Shading") { FriendlyName = "Shading" };
            SettingGroups.Add(ShadingProperties);

            DepthLimit = new NumericSetting<float>("DepthLimit", 8, 0, 8, 2) { FriendlyName = "Depth limit (meters)" };
            ShadingProperties.Settings.Add(DepthLimit);
        }
    }
}