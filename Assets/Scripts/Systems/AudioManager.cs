using UnityEngine;

namespace EverySingleDay.Systems
{
    /// <summary>
    /// Tiny persistent audio hub. Exposes static <see cref="Play"/> for one-shot
    /// SFX from anywhere (enemies, pickups, the player) without each caller
    /// needing its own AudioSource, plus simple looping background music.
    /// </summary>
    public class AudioManager : MonoBehaviour
    {
        public static AudioManager Instance { get; private set; }

        [Header("Sources")]
        public AudioSource musicSource;
        public AudioSource sfxSource;

        [Header("Music")]
        public AudioClip backgroundMusic;
        [Range(0f, 1f)] public float musicVolume = 0.5f;
        [Range(0f, 1f)] public float sfxVolume = 0.8f;

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;
            DontDestroyOnLoad(gameObject);

            EnsureSources();
            GameSettings.OnChanged += ApplySettings;
        }

        private void OnDestroy()
        {
            GameSettings.OnChanged -= ApplySettings;
        }

        /// <summary>React live to Options tickboxes (mute/unmute music).</summary>
        private void ApplySettings()
        {
            if (musicSource == null) return;
            if (GameSettings.MusicEnabled)
            {
                if (!musicSource.isPlaying && musicSource.clip != null)
                    musicSource.Play();
            }
            else
            {
                musicSource.Stop();
            }
        }

        [Tooltip("If no backgroundMusic clip is assigned, play the synthesized loop.")]
        public bool useSynthesizedMusicFallback = true;

        private void Start()
        {
            if (musicSource == null) return;

            var clip = backgroundMusic;
            if (clip == null && useSynthesizedMusicFallback)
                clip = SfxLibrary.Music;

            if (clip != null)
            {
                musicSource.clip = clip;
                musicSource.loop = true;
                musicSource.volume = musicVolume;
                if (GameSettings.MusicEnabled)
                    musicSource.Play();
            }
        }

        private void EnsureSources()
        {
            if (musicSource == null)
            {
                musicSource = gameObject.AddComponent<AudioSource>();
                musicSource.playOnAwake = false;
            }
            if (sfxSource == null)
            {
                sfxSource = gameObject.AddComponent<AudioSource>();
                sfxSource.playOnAwake = false;
            }
        }

        /// <summary>Play a one-shot sound effect from anywhere.</summary>
        public static void Play(AudioClip clip, float volumeScale = 1f)
        {
            if (clip == null) return;
            if (!GameSettings.SfxEnabled) return;
            if (Instance != null && Instance.sfxSource != null)
                Instance.sfxSource.PlayOneShot(clip, Instance.sfxVolume * volumeScale);
        }

        public static void PlayMusic(AudioClip clip)
        {
            if (Instance == null || Instance.musicSource == null || clip == null) return;
            Instance.musicSource.clip = clip;
            Instance.musicSource.loop = true;
            Instance.musicSource.volume = Instance.musicVolume;
            if (GameSettings.MusicEnabled)
                Instance.musicSource.Play();
        }

        public void SetMusicVolume(float v)
        {
            musicVolume = Mathf.Clamp01(v);
            if (musicSource != null) musicSource.volume = musicVolume;
        }

        public void SetSfxVolume(float v) => sfxVolume = Mathf.Clamp01(v);
    }
}
