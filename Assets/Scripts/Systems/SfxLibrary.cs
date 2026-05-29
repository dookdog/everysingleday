using UnityEngine;

namespace EverySingleDay.Systems
{
    /// <summary>
    /// Procedurally synthesizes every sound effect and the background music at
    /// runtime, so the game ships with full audio and zero imported .wav/.mp3
    /// files. Clips are generated lazily and cached. Swap any of these for real
    /// audio by assigning clips in the inspector on the relevant components.
    ///
    /// Waveforms: 0 = sine, 1 = square, 2 = triangle, 3 = noise.
    /// </summary>
    public static class SfxLibrary
    {
        private const int SampleRate = 44100;

        private static AudioClip _jump, _doubleJump, _land, _coin, _gem, _stomp,
            _shoot, _hurt, _death, _checkpoint, _goal, _gameOver, _uiClick, _music;

        public static AudioClip Jump        => _jump        ??= BuildJump();
        public static AudioClip DoubleJump  => _doubleJump  ??= BuildDoubleJump();
        public static AudioClip Land        => _land        ??= BuildLand();
        public static AudioClip Coin        => _coin        ??= BuildCoin();
        public static AudioClip Gem         => _gem         ??= BuildGem();
        public static AudioClip Stomp       => _stomp       ??= BuildStomp();
        public static AudioClip Shoot       => _shoot       ??= BuildShoot();
        public static AudioClip Hurt        => _hurt        ??= BuildHurt();
        public static AudioClip Death       => _death       ??= BuildDeath();
        public static AudioClip Checkpoint  => _checkpoint  ??= BuildCheckpoint();
        public static AudioClip Goal        => _goal        ??= BuildGoal();
        public static AudioClip GameOver    => _gameOver    ??= BuildGameOver();
        public static AudioClip UiClick     => _uiClick     ??= BuildUiClick();
        public static AudioClip Music       => _music       ??= BuildMusic();

        // ------------------------------------------------------------ helpers
        /// <summary>MIDI note number -> frequency in Hz (A4 = 69 = 440Hz).</summary>
        private static float Hz(int midi) => 440f * Mathf.Pow(2f, (midi - 69) / 12f);

        private static AudioClip ToClip(string name, float[] data)
        {
            // Clamp to avoid clipping artefacts after summing tones.
            for (int i = 0; i < data.Length; i++)
                data[i] = Mathf.Clamp(data[i], -1f, 1f);

            var clip = AudioClip.Create(name, data.Length, 1, SampleRate, false);
            clip.SetData(data, 0);
            return clip;
        }

        private static float[] Buffer(float seconds) => new float[Mathf.CeilToInt(seconds * SampleRate)];

        private static float Wave(int type, double phase)
        {
            switch (type)
            {
                case 1: return Mathf.Sign(Mathf.Sin((float)phase));                 // square
                case 2: return (2f / Mathf.PI) * Mathf.Asin(Mathf.Sin((float)phase)); // triangle
                case 3: return Random.value * 2f - 1f;                              // noise
                default: return Mathf.Sin((float)phase);                            // sine
            }
        }

        /// <summary>
        /// Adds a tone (optionally frequency-swept) with an attack + exponential
        /// decay envelope into the buffer at the given start time.
        /// </summary>
        private static void AddTone(float[] buf, float start, float dur, float f0,
            float f1, float vol, int waveform, float attack = 0.005f, float decay = 4f)
        {
            int startIdx = Mathf.Clamp((int)(start * SampleRate), 0, buf.Length);
            int len = (int)(dur * SampleRate);
            double phase = 0;
            float aFrac = Mathf.Max(attack / dur, 1e-4f);

            for (int i = 0; i < len; i++)
            {
                int idx = startIdx + i;
                if (idx >= buf.Length) break;

                float t = i / (float)len;
                float freq = Mathf.Lerp(f0, f1, t);
                phase += 2.0 * Mathf.PI * freq / SampleRate;

                float env = t < aFrac ? t / aFrac : Mathf.Exp(-decay * (t - aFrac));
                buf[idx] += Wave(waveform, phase) * vol * env;
            }
        }

        // ------------------------------------------------------------- effects
        private static AudioClip BuildJump()
        {
            var b = Buffer(0.2f);
            AddTone(b, 0f, 0.18f, 240f, 540f, 0.35f, 1, decay: 5f);
            return ToClip("sfx_jump", b);
        }

        private static AudioClip BuildDoubleJump()
        {
            var b = Buffer(0.2f);
            AddTone(b, 0f, 0.18f, 360f, 760f, 0.32f, 1, decay: 5f);
            return ToClip("sfx_double_jump", b);
        }

        private static AudioClip BuildLand()
        {
            var b = Buffer(0.14f);
            AddTone(b, 0f, 0.10f, 150f, 70f, 0.30f, 0, decay: 9f);
            AddTone(b, 0f, 0.06f, 0f, 0f, 0.15f, 3, decay: 14f);
            return ToClip("sfx_land", b);
        }

        private static AudioClip BuildCoin()
        {
            var b = Buffer(0.2f);
            AddTone(b, 0.00f, 0.07f, Hz(83), Hz(83), 0.30f, 1, decay: 6f); // B5
            AddTone(b, 0.06f, 0.12f, Hz(88), Hz(88), 0.30f, 1, decay: 5f); // E6
            return ToClip("sfx_coin", b);
        }

        private static AudioClip BuildGem()
        {
            var b = Buffer(0.3f);
            AddTone(b, 0.00f, 0.07f, Hz(79), Hz(79), 0.25f, 2, decay: 5f);
            AddTone(b, 0.06f, 0.07f, Hz(84), Hz(84), 0.25f, 2, decay: 5f);
            AddTone(b, 0.12f, 0.15f, Hz(91), Hz(91), 0.28f, 2, decay: 4f);
            return ToClip("sfx_gem", b);
        }

        private static AudioClip BuildStomp()
        {
            var b = Buffer(0.18f);
            AddTone(b, 0f, 0.14f, 220f, 50f, 0.35f, 0, decay: 7f);
            AddTone(b, 0f, 0.08f, 0f, 0f, 0.20f, 3, decay: 16f);
            return ToClip("sfx_stomp", b);
        }

        private static AudioClip BuildShoot()
        {
            var b = Buffer(0.16f);
            AddTone(b, 0f, 0.14f, 700f, 180f, 0.22f, 1, decay: 8f);
            return ToClip("sfx_shoot", b);
        }

        private static AudioClip BuildHurt()
        {
            var b = Buffer(0.3f);
            AddTone(b, 0f, 0.26f, 420f, 110f, 0.34f, 1, decay: 4f);
            return ToClip("sfx_hurt", b);
        }

        private static AudioClip BuildDeath()
        {
            var b = Buffer(0.6f);
            AddTone(b, 0.00f, 0.14f, Hz(69), Hz(69), 0.30f, 1, decay: 4f);
            AddTone(b, 0.13f, 0.14f, Hz(64), Hz(64), 0.30f, 1, decay: 4f);
            AddTone(b, 0.26f, 0.28f, Hz(57), Hz(50), 0.32f, 1, decay: 3f);
            return ToClip("sfx_death", b);
        }

        private static AudioClip BuildCheckpoint()
        {
            var b = Buffer(0.3f);
            AddTone(b, 0.00f, 0.10f, Hz(76), Hz(76), 0.28f, 2, decay: 5f);
            AddTone(b, 0.09f, 0.16f, Hz(83), Hz(83), 0.30f, 2, decay: 4f);
            return ToClip("sfx_checkpoint", b);
        }

        private static AudioClip BuildGoal()
        {
            var b = Buffer(0.8f);
            int[] notes = { 72, 76, 79, 84 }; // C5 E5 G5 C6 fanfare
            for (int i = 0; i < notes.Length; i++)
                AddTone(b, i * 0.12f, 0.22f, Hz(notes[i]), Hz(notes[i]), 0.30f, 2, decay: 3f);
            return ToClip("sfx_goal", b);
        }

        private static AudioClip BuildGameOver()
        {
            var b = Buffer(1.1f);
            int[] notes = { 67, 64, 60, 55 }; // descending, somber
            for (int i = 0; i < notes.Length; i++)
                AddTone(b, i * 0.22f, 0.30f, Hz(notes[i]), Hz(notes[i]), 0.30f, 2, decay: 2.5f);
            return ToClip("sfx_game_over", b);
        }

        private static AudioClip BuildUiClick()
        {
            var b = Buffer(0.08f);
            AddTone(b, 0f, 0.06f, 880f, 880f, 0.28f, 1, decay: 10f);
            return ToClip("sfx_ui_click", b);
        }

        // -------------------------------------------------------------- music
        private static AudioClip BuildMusic()
        {
            // Eight-second loop: vi-IV-I-V (Am F C G) arpeggio + soft bass.
            float chordLen = 2f;
            float total = chordLen * 4f;
            var b = Buffer(total);

            int[][] chords =
            {
                new[] { 57, 60, 64, 69 }, // Am
                new[] { 53, 57, 60, 65 }, // F
                new[] { 48, 52, 55, 60 }, // C
                new[] { 55, 59, 62, 67 }, // G
            };

            int[] pattern = { 0, 1, 2, 3, 2, 1, 2, 3 };
            float step = chordLen / pattern.Length; // eighth notes

            for (int c = 0; c < chords.Length; c++)
            {
                float chordStart = c * chordLen;

                // Soft sustained bass (root, one octave down).
                AddTone(b, chordStart, chordLen, Hz(chords[c][0] - 12),
                    Hz(chords[c][0] - 12), 0.12f, 0, attack: 0.05f, decay: 0.6f);

                // Arpeggiated melody.
                for (int s = 0; s < pattern.Length; s++)
                {
                    int midi = chords[c][pattern[s]];
                    AddTone(b, chordStart + s * step, step * 0.95f, Hz(midi), Hz(midi),
                        0.10f, 2, attack: 0.01f, decay: 2.5f);
                }
            }

            return ToClip("music_loop", b);
        }
    }
}
