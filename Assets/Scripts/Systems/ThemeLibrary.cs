using System.Collections.Generic;
using UnityEngine;

namespace EverySingleDay.Systems
{
    /// <summary>
    /// The authored catalogue of <see cref="LevelTheme"/>s. The game's identity
    /// is negative space: dark, atmospheric levels where a lit gameplay plane
    /// reads against shadow, framed by foreground silhouettes, fog and a
    /// vignette (Hollow Knight / Limbo). This carries the mood with little drawn
    /// detail, which is also cheaper to render.
    ///
    /// Two families share one design language:
    ///  - "Hollow" variants: muted colour (deep teal/indigo/amber), lit focal
    ///    elements, fog + silhouettes.
    ///  - "Limbo" variants: near-monochrome, black shapes on grey, one accent.
    ///
    /// All ship as data in code — no art, keys or network at runtime.
    /// </summary>
    public static class ThemeLibrary
    {
        private static List<LevelTheme> _themes;

        public static IReadOnlyList<LevelTheme> All => _themes ??= BuildAll();

        public static int Count => All.Count;

        public static LevelTheme Get(int index) => All[Mathf.Abs(index) % All.Count];

        public static LevelTheme GetById(string id)
        {
            foreach (var t in All) if (t.id == id) return t;
            return All[0];
        }

        /// <summary>Pick a theme for a run from its seed (deterministic per seed).</summary>
        public static LevelTheme PickForSeed(int seed) => Get(new System.Random(seed).Next());

        private static List<LevelTheme> BuildAll() => new List<LevelTheme>
        {
            HollowDepths(),
            AbyssTeal(),
            EmberDark(),
            LimboGrey(),
            LimboBlue(),
        };

        // ---------------------------------------------------------- Hollow family
        // Muted colour on darkness; lit focal elements; fog + silhouettes.

        private static LevelTheme HollowDepths() => new LevelTheme
        {
            id = "hollow_depths",
            displayName = "Hollow Depths",
            mood = "still, hushed, cavernous",
            skyTop = C(0.03f, 0.04f, 0.07f), skyBottom = C(0.06f, 0.08f, 0.13f),
            ground = C(0.05f, 0.06f, 0.10f), groundCap = C(0.16f, 0.22f, 0.34f),
            platform = C(0.09f, 0.10f, 0.16f), player = C(0.90f, 0.95f, 1f),
            enemy = C(0.55f, 0.30f, 0.60f), enemyAlt = C(0.35f, 0.30f, 0.65f),
            coin = C(0.80f, 0.85f, 1f), gem = C(0.55f, 0.80f, 1f),
            hazard = C(0.45f, 0.55f, 0.75f), goal = C(0.60f, 0.90f, 1f),
            accent = C(0.75f, 0.85f, 1f),
            backdropTints = new[] { C(0.05f, 0.06f, 0.11f), C(0.07f, 0.09f, 0.16f), C(0.10f, 0.13f, 0.22f) },
            backdropShape = 1,
            vignette = 0.5f, fogColor = C(0.03f, 0.04f, 0.07f), fogDensity = 0.25f,
            foregroundSilhouettes = true, silhouetteColor = C(0.01f, 0.01f, 0.02f),
            focalGlow = true,
            enemyDensity = 0.5f, turretChance = 0.25f, hazardDensity = 0.35f,
            coinDensity = 0.55f, gemChance = 0.35f,
            gravityScale = 1f, playerSpeedScale = 1f, jumpScale = 1f,
            musicRootNote = 48, musicMode = 0, musicTempo = 84f,
            objectiveWeights = new[] { 0.9f, 1.0f, 0.7f, 0.9f, 0.5f },
        };

        private static LevelTheme AbyssTeal() => new LevelTheme
        {
            id = "abyss_teal",
            displayName = "The Abyss",
            mood = "cold, drowned, luminous",
            skyTop = C(0.02f, 0.05f, 0.06f), skyBottom = C(0.04f, 0.09f, 0.11f),
            ground = C(0.03f, 0.07f, 0.08f), groundCap = C(0.10f, 0.28f, 0.30f),
            platform = C(0.05f, 0.11f, 0.13f), player = C(0.85f, 1f, 0.98f),
            enemy = C(0.20f, 0.55f, 0.55f), enemyAlt = C(0.25f, 0.45f, 0.60f),
            coin = C(0.70f, 1f, 0.95f), gem = C(0.40f, 0.95f, 0.90f),
            hazard = C(0.35f, 0.70f, 0.70f), goal = C(0.50f, 1f, 0.85f),
            accent = C(0.60f, 0.95f, 0.90f),
            backdropTints = new[] { C(0.03f, 0.07f, 0.09f), C(0.04f, 0.11f, 0.13f), C(0.06f, 0.16f, 0.18f) },
            backdropShape = 1,
            vignette = 0.55f, fogColor = C(0.02f, 0.05f, 0.06f), fogDensity = 0.3f,
            foregroundSilhouettes = true, silhouetteColor = C(0.01f, 0.02f, 0.02f),
            focalGlow = true,
            enemyDensity = 0.45f, turretChance = 0.2f, hazardDensity = 0.3f,
            coinDensity = 0.6f, gemChance = 0.4f,
            gravityScale = 0.85f, playerSpeedScale = 0.95f, jumpScale = 1.05f, // sunken, floaty
            musicRootNote = 45, musicMode = 0, musicTempo = 76f,
            objectiveWeights = new[] { 0.8f, 1.0f, 0.6f, 1.0f, 0.6f },
        };

        private static LevelTheme EmberDark() => new LevelTheme
        {
            id = "ember_dark",
            displayName = "Ashen Deep",
            mood = "smouldering, tense, close",
            skyTop = C(0.06f, 0.03f, 0.03f), skyBottom = C(0.12f, 0.05f, 0.04f),
            ground = C(0.07f, 0.04f, 0.03f), groundCap = C(0.35f, 0.14f, 0.07f),
            platform = C(0.11f, 0.06f, 0.05f), player = C(1f, 0.95f, 0.85f),
            enemy = C(0.70f, 0.28f, 0.12f), enemyAlt = C(0.55f, 0.18f, 0.10f),
            coin = C(1f, 0.75f, 0.35f), gem = C(1f, 0.55f, 0.30f),
            hazard = C(0.90f, 0.40f, 0.15f), goal = C(1f, 0.75f, 0.40f),
            accent = C(1f, 0.65f, 0.35f),
            backdropTints = new[] { C(0.08f, 0.04f, 0.03f), C(0.13f, 0.06f, 0.04f), C(0.20f, 0.09f, 0.05f) },
            backdropShape = 1,
            vignette = 0.5f, fogColor = C(0.05f, 0.02f, 0.02f), fogDensity = 0.28f,
            foregroundSilhouettes = true, silhouetteColor = C(0.02f, 0.01f, 0.01f),
            focalGlow = true,
            enemyDensity = 0.6f, turretChance = 0.3f, hazardDensity = 0.5f,
            coinDensity = 0.5f, gemChance = 0.3f,
            gravityScale = 1.05f, playerSpeedScale = 1f, jumpScale = 1f,
            musicRootNote = 43, musicMode = 0, musicTempo = 120f,
            objectiveWeights = new[] { 1.0f, 0.5f, 1.0f, 0.6f, 0.8f },
        };

        // ----------------------------------------------------------- Limbo family
        // Near-monochrome: black silhouettes on grey, a single accent.

        private static LevelTheme LimboGrey() => new LevelTheme
        {
            id = "limbo_grey",
            displayName = "Grey Between",
            mood = "bleak, silent, stark",
            skyTop = C(0.10f, 0.10f, 0.11f), skyBottom = C(0.20f, 0.20f, 0.22f),
            ground = C(0.02f, 0.02f, 0.02f), groundCap = C(0.05f, 0.05f, 0.05f),
            platform = C(0.03f, 0.03f, 0.03f), player = C(0.05f, 0.05f, 0.05f), // dark player, Limbo-style
            enemy = C(0.02f, 0.02f, 0.02f), enemyAlt = C(0.02f, 0.02f, 0.02f),
            coin = C(0.85f, 0.85f, 0.85f), gem = C(1f, 1f, 1f),
            hazard = C(0.01f, 0.01f, 0.01f), goal = C(0.95f, 0.95f, 0.95f),
            accent = C(0.90f, 0.90f, 0.90f),
            backdropTints = new[] { C(0.13f, 0.13f, 0.14f), C(0.17f, 0.17f, 0.18f), C(0.22f, 0.22f, 0.23f) },
            backdropShape = 2,
            vignette = 0.7f, fogColor = C(0.12f, 0.12f, 0.13f), fogDensity = 0.35f,
            foregroundSilhouettes = true, silhouetteColor = C(0f, 0f, 0f),
            focalGlow = false, // stark, no glow
            enemyDensity = 0.5f, turretChance = 0.25f, hazardDensity = 0.45f,
            coinDensity = 0.45f, gemChance = 0.35f,
            gravityScale = 1f, playerSpeedScale = 1f, jumpScale = 1f,
            musicRootNote = 40, musicMode = 0, musicTempo = 70f,
            objectiveWeights = new[] { 1.0f, 0.7f, 0.8f, 0.7f, 0.7f },
        };

        private static LevelTheme LimboBlue() => new LevelTheme
        {
            id = "limbo_blue",
            displayName = "Moonlit Silence",
            mood = "lonely, luminous, monochrome",
            skyTop = C(0.08f, 0.10f, 0.16f), skyBottom = C(0.16f, 0.20f, 0.30f),
            ground = C(0.01f, 0.02f, 0.03f), groundCap = C(0.03f, 0.04f, 0.06f),
            platform = C(0.02f, 0.03f, 0.05f), player = C(0.03f, 0.04f, 0.06f),
            enemy = C(0.01f, 0.02f, 0.03f), enemyAlt = C(0.01f, 0.02f, 0.03f),
            coin = C(0.80f, 0.88f, 1f), gem = C(0.90f, 0.95f, 1f),
            hazard = C(0.01f, 0.01f, 0.02f), goal = C(0.85f, 0.92f, 1f),
            accent = C(0.80f, 0.88f, 1f),
            backdropTints = new[] { C(0.10f, 0.13f, 0.20f), C(0.13f, 0.17f, 0.26f), C(0.17f, 0.22f, 0.33f) },
            backdropShape = 2,
            vignette = 0.68f, fogColor = C(0.08f, 0.10f, 0.16f), fogDensity = 0.35f,
            foregroundSilhouettes = true, silhouetteColor = C(0f, 0f, 0.01f),
            focalGlow = true,
            enemyDensity = 0.45f, turretChance = 0.2f, hazardDensity = 0.4f,
            coinDensity = 0.5f, gemChance = 0.35f,
            gravityScale = 0.9f, playerSpeedScale = 1f, jumpScale = 1.05f,
            musicRootNote = 47, musicMode = 2, musicTempo = 80f,
            objectiveWeights = new[] { 0.9f, 0.9f, 0.7f, 0.9f, 0.7f },
        };

        private static Color C(float r, float g, float b) => new Color(r, g, b);
    }
}
