using System.Collections.Generic;
using UnityEngine;

namespace EverySingleDay.Systems
{
    /// <summary>
    /// The authored catalogue of <see cref="LevelTheme"/>s. Each entry distills
    /// an overall aesthetic + feel into data the generator can build a level
    /// from. These ship in code so the game needs no art or network.
    ///
    /// To add a theme from reference imagery: read its palette, mood, density
    /// and motion, then add a new builder method below and register it in
    /// <see cref="BuildAll"/>. Nothing else needs to change — the generator and
    /// bootstrap consume any theme uniformly.
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
            Verdant(),
            Sunset(),
            Cavern(),
            Frost(),
            Volcanic(),
            Void(),
        };

        // ------------------------------------------------------------- themes
        // These are starter themes. Replace/extend their palettes and feel to
        // match supplied reference imagery — that is the intended pipeline.

        private static LevelTheme Verdant() => new LevelTheme
        {
            id = "verdant",
            displayName = "Verdant Hills",
            mood = "bright, breezy, welcoming",
            skyTop = C(0.30f, 0.55f, 0.85f), skyBottom = C(0.62f, 0.82f, 0.96f),
            ground = C(0.30f, 0.22f, 0.16f), groundCap = C(0.34f, 0.68f, 0.32f),
            platform = C(0.52f, 0.40f, 0.28f), player = C(0.97f, 0.86f, 0.30f),
            enemy = C(0.85f, 0.30f, 0.28f), enemyAlt = C(0.80f, 0.55f, 0.20f),
            coin = C(1f, 0.84f, 0.20f), gem = C(0.35f, 0.85f, 0.95f),
            hazard = C(0.80f, 0.80f, 0.82f), goal = C(0.30f, 0.92f, 0.46f),
            accent = C(1f, 1f, 0.92f),
            backdropTints = new[] { C(0.34f, 0.58f, 0.80f), C(0.40f, 0.64f, 0.50f), C(0.50f, 0.72f, 0.40f) },
            backdropShape = 0,
            enemyDensity = 0.45f, turretChance = 0.15f, hazardDensity = 0.25f,
            coinDensity = 0.65f, gemChance = 0.25f,
            gravityScale = 1f, playerSpeedScale = 1f, jumpScale = 1f,
            musicRootNote = 60, musicMode = 1, musicTempo = 120f,
            objectiveWeights = new[] { 1.0f, 0.8f, 0.7f, 0.6f, 0.3f },
        };

        private static LevelTheme Sunset() => new LevelTheme
        {
            id = "sunset",
            displayName = "Amber Dunes",
            mood = "warm, golden, nostalgic",
            skyTop = C(0.85f, 0.45f, 0.35f), skyBottom = C(0.98f, 0.78f, 0.45f),
            ground = C(0.45f, 0.28f, 0.20f), groundCap = C(0.78f, 0.55f, 0.28f),
            platform = C(0.60f, 0.38f, 0.30f), player = C(0.98f, 0.95f, 0.85f),
            enemy = C(0.55f, 0.25f, 0.40f), enemyAlt = C(0.75f, 0.35f, 0.30f),
            coin = C(1f, 0.88f, 0.40f), gem = C(0.95f, 0.45f, 0.65f),
            hazard = C(0.40f, 0.22f, 0.25f), goal = C(0.95f, 0.85f, 0.40f),
            accent = C(1f, 0.92f, 0.75f),
            backdropTints = new[] { C(0.70f, 0.40f, 0.45f), C(0.85f, 0.55f, 0.40f), C(0.95f, 0.70f, 0.45f) },
            backdropShape = 0,
            enemyDensity = 0.5f, turretChance = 0.2f, hazardDensity = 0.3f,
            coinDensity = 0.7f, gemChance = 0.2f,
            gravityScale = 0.95f, playerSpeedScale = 1.05f, jumpScale = 1.05f,
            musicRootNote = 57, musicMode = 2, musicTempo = 100f,
            objectiveWeights = new[] { 1.0f, 1.0f, 0.5f, 0.8f, 0.3f },
        };

        private static LevelTheme Cavern() => new LevelTheme
        {
            id = "cavern",
            displayName = "Crystal Caverns",
            mood = "dim, mysterious, glittering",
            skyTop = C(0.06f, 0.07f, 0.12f), skyBottom = C(0.12f, 0.14f, 0.22f),
            ground = C(0.16f, 0.15f, 0.20f), groundCap = C(0.30f, 0.32f, 0.45f),
            platform = C(0.24f, 0.22f, 0.34f), player = C(0.60f, 0.95f, 0.95f),
            enemy = C(0.70f, 0.30f, 0.85f), enemyAlt = C(0.40f, 0.30f, 0.80f),
            coin = C(0.70f, 0.90f, 1f), gem = C(0.65f, 0.45f, 0.95f),
            hazard = C(0.55f, 0.85f, 0.85f), goal = C(0.45f, 0.95f, 0.80f),
            accent = C(0.70f, 0.85f, 1f),
            backdropTints = new[] { C(0.10f, 0.12f, 0.20f), C(0.16f, 0.18f, 0.30f), C(0.22f, 0.26f, 0.42f) },
            backdropShape = 1,
            enemyDensity = 0.55f, turretChance = 0.3f, hazardDensity = 0.4f,
            coinDensity = 0.55f, gemChance = 0.4f,
            gravityScale = 1f, playerSpeedScale = 0.95f, jumpScale = 1f,
            musicRootNote = 50, musicMode = 0, musicTempo = 92f,
            objectiveWeights = new[] { 0.8f, 1.0f, 0.7f, 1.0f, 0.4f },
        };

        private static LevelTheme Frost() => new LevelTheme
        {
            id = "frost",
            displayName = "Frozen Reaches",
            mood = "cold, crisp, slippery-bright",
            skyTop = C(0.55f, 0.70f, 0.85f), skyBottom = C(0.82f, 0.90f, 0.96f),
            ground = C(0.55f, 0.62f, 0.72f), groundCap = C(0.85f, 0.92f, 0.98f),
            platform = C(0.62f, 0.74f, 0.85f), player = C(0.95f, 0.55f, 0.35f),
            enemy = C(0.30f, 0.45f, 0.70f), enemyAlt = C(0.45f, 0.60f, 0.80f),
            coin = C(1f, 0.90f, 0.55f), gem = C(0.55f, 0.85f, 1f),
            hazard = C(0.70f, 0.85f, 0.95f), goal = C(0.40f, 0.85f, 0.70f),
            accent = C(0.95f, 0.98f, 1f),
            backdropTints = new[] { C(0.60f, 0.72f, 0.85f), C(0.72f, 0.82f, 0.92f), C(0.82f, 0.90f, 0.96f) },
            backdropShape = 1,
            enemyDensity = 0.45f, turretChance = 0.2f, hazardDensity = 0.35f,
            coinDensity = 0.6f, gemChance = 0.3f,
            gravityScale = 1f, playerSpeedScale = 1.15f, jumpScale = 1.05f, // icy momentum
            musicRootNote = 62, musicMode = 1, musicTempo = 108f,
            objectiveWeights = new[] { 1.0f, 0.7f, 0.8f, 0.7f, 0.6f },
        };

        private static LevelTheme Volcanic() => new LevelTheme
        {
            id = "volcanic",
            displayName = "Ember Depths",
            mood = "hot, dangerous, urgent",
            skyTop = C(0.12f, 0.05f, 0.05f), skyBottom = C(0.35f, 0.10f, 0.06f),
            ground = C(0.18f, 0.10f, 0.08f), groundCap = C(0.55f, 0.18f, 0.10f),
            platform = C(0.28f, 0.14f, 0.10f), player = C(0.95f, 0.92f, 0.80f),
            enemy = C(0.95f, 0.45f, 0.12f), enemyAlt = C(0.80f, 0.20f, 0.10f),
            coin = C(1f, 0.80f, 0.30f), gem = C(1f, 0.40f, 0.20f),
            hazard = C(1f, 0.45f, 0.10f), goal = C(1f, 0.85f, 0.45f),
            accent = C(1f, 0.65f, 0.25f),
            backdropTints = new[] { C(0.25f, 0.08f, 0.06f), C(0.40f, 0.14f, 0.08f), C(0.60f, 0.22f, 0.10f) },
            backdropShape = 1,
            enemyDensity = 0.65f, turretChance = 0.35f, hazardDensity = 0.55f,
            coinDensity = 0.5f, gemChance = 0.25f,
            gravityScale = 1.05f, playerSpeedScale = 1f, jumpScale = 1f,
            musicRootNote = 48, musicMode = 0, musicTempo = 140f,
            objectiveWeights = new[] { 1.0f, 0.5f, 1.0f, 0.5f, 0.8f },
        };

        private static LevelTheme Void() => new LevelTheme
        {
            id = "void",
            displayName = "Neon Void",
            mood = "weightless, electric, surreal",
            skyTop = C(0.04f, 0.02f, 0.10f), skyBottom = C(0.10f, 0.04f, 0.20f),
            ground = C(0.10f, 0.08f, 0.18f), groundCap = C(0.85f, 0.20f, 0.75f),
            platform = C(0.18f, 0.12f, 0.30f), player = C(0.30f, 1f, 0.85f),
            enemy = C(1f, 0.20f, 0.55f), enemyAlt = C(0.55f, 0.20f, 1f),
            coin = C(0.95f, 1f, 0.30f), gem = C(0.30f, 0.95f, 1f),
            hazard = C(1f, 0.25f, 0.55f), goal = C(0.55f, 1f, 0.55f),
            accent = C(0.85f, 0.40f, 1f),
            backdropTints = new[] { C(0.10f, 0.05f, 0.22f), C(0.20f, 0.08f, 0.35f), C(0.35f, 0.12f, 0.50f) },
            backdropShape = 2,
            enemyDensity = 0.5f, turretChance = 0.3f, hazardDensity = 0.3f,
            coinDensity = 0.6f, gemChance = 0.45f,
            gravityScale = 0.6f, playerSpeedScale = 1.1f, jumpScale = 1.25f, // low-g
            musicRootNote = 55, musicMode = 2, musicTempo = 126f,
            objectiveWeights = new[] { 0.8f, 1.0f, 0.8f, 1.0f, 0.7f },
        };

        private static Color C(float r, float g, float b) => new Color(r, g, b);
    }
}
