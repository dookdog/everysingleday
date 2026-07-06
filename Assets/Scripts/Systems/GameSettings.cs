using UnityEngine;

namespace EverySingleDay.Systems
{
    /// <summary>
    /// Persistent game options, saved via PlayerPrefs. Each option is a simple
    /// on/off toggle surfaced in the Options window as a tickbox. Other systems
    /// read these statics (e.g. AudioManager checks MusicEnabled).
    /// </summary>
    public static class GameSettings
    {
        // Keys
        private const string KMusic = "esd_opt_music";
        private const string KSfx = "esd_opt_sfx";
        private const string KScreenShake = "esd_opt_shake";
        private const string KShowHints = "esd_opt_hints";
        private const string KShowTimer = "esd_opt_timer";
        private const string KHardMode = "esd_opt_hard";

        public static bool MusicEnabled
        {
            get => Get(KMusic, true);
            set { Set(KMusic, value); OnChanged?.Invoke(); }
        }
        public static bool SfxEnabled
        {
            get => Get(KSfx, true);
            set { Set(KSfx, value); OnChanged?.Invoke(); }
        }
        public static bool ScreenShakeEnabled
        {
            get => Get(KScreenShake, true);
            set { Set(KScreenShake, value); OnChanged?.Invoke(); }
        }
        public static bool ShowHints
        {
            get => Get(KShowHints, true);
            set { Set(KShowHints, value); OnChanged?.Invoke(); }
        }
        public static bool ShowTimer
        {
            get => Get(KShowTimer, false);
            set { Set(KShowTimer, value); OnChanged?.Invoke(); }
        }
        public static bool HardMode
        {
            get => Get(KHardMode, false);
            set { Set(KHardMode, value); OnChanged?.Invoke(); }
        }

        /// <summary>Fired whenever any setting changes, so systems can react live.</summary>
        public static System.Action OnChanged;

        private static bool Get(string key, bool dflt)
            => PlayerPrefs.GetInt(key, dflt ? 1 : 0) == 1;

        private static void Set(string key, bool value)
        {
            PlayerPrefs.SetInt(key, value ? 1 : 0);
            PlayerPrefs.Save();
        }
    }
}
