using UnityEngine;

namespace EverySingleDay.Systems
{
    /// <summary>
    /// What the player must do to finish a generated level. The generator picks
    /// one per run (weighted by the active theme) so objectives vary every play.
    /// </summary>
    public enum ObjectiveType
    {
        ReachGoal,        // touch the exit
        CollectAllGems,   // gather every gem, then reach the exit
        DefeatAllEnemies, // clear every enemy, then reach the exit
        CollectQuota,     // gather N gems (a subset), then reach the exit
        Survive           // stay alive until a timer expires, then exit opens
    }

    /// <summary>
    /// A complete aesthetic + gameplay "feel" definition for a level. This is the
    /// unit of content the project is built around: the procedural generator and
    /// the bootstrap both read a LevelTheme to decide colours, density, enemy
    /// mix, hazards, physics feel, music and objective bias.
    ///
    /// Themes are authored in <see cref="ThemeLibrary"/>. The intended workflow
    /// is: provide reference imagery (e.g. generated in an external tool), and a
    /// matching theme is hand-authored here — palette, mood and motifs distilled
    /// into these fields. No art or network access is required at runtime.
    /// </summary>
    [System.Serializable]
    public class LevelTheme
    {
        [Header("Identity")]
        public string id = "default";
        public string displayName = "Verdant Hills";
        [Tooltip("One-line mood description, shown on the level intro card.")]
        public string mood = "bright and breezy";

        [Header("Palette")]
        public Color skyTop = new Color(0.30f, 0.55f, 0.85f);
        public Color skyBottom = new Color(0.55f, 0.78f, 0.95f);
        public Color ground = new Color(0.27f, 0.20f, 0.16f);
        public Color groundCap = new Color(0.32f, 0.65f, 0.30f);
        public Color platform = new Color(0.55f, 0.40f, 0.70f);
        public Color player = new Color(0.95f, 0.85f, 0.30f);
        public Color enemy = new Color(0.85f, 0.27f, 0.27f);
        public Color enemyAlt = new Color(0.55f, 0.25f, 0.55f);
        public Color coin = new Color(1f, 0.82f, 0.18f);
        public Color gem = new Color(0.40f, 0.85f, 0.95f);
        public Color hazard = new Color(0.75f, 0.78f, 0.82f);
        public Color goal = new Color(0.30f, 0.90f, 0.45f);
        public Color accent = new Color(1f, 1f, 1f);

        [Header("Backdrop")]
        [Tooltip("Tints for the parallax hill/shape layers, far to near.")]
        public Color[] backdropTints =
        {
            new Color(0.30f, 0.52f, 0.75f),
            new Color(0.40f, 0.60f, 0.50f),
            new Color(0.50f, 0.68f, 0.42f),
        };
        [Tooltip("0 = rounded hills, 1 = jagged peaks, 2 = floating blobs.")]
        public int backdropShape = 0;

        [Header("Negative space / atmosphere")]
        [Tooltip("Darkened screen-edge frame (0 = none, 1 = heavy). Focuses the " +
                 "eye on the lit gameplay plane, Hollow Knight / Limbo style.")]
        [Range(0f, 1f)] public float vignette = 0.35f;
        [Tooltip("Colour distance fades toward — usually near-black. Distant " +
                 "geometry dissolves into it so it needs no detail.")]
        public Color fogColor = new Color(0.03f, 0.04f, 0.06f);
        [Tooltip("How strongly the fog overlay tints the scene (0 = off).")]
        [Range(0f, 1f)] public float fogDensity = 0f;
        [Tooltip("Add a near, pure-silhouette foreground layer (black shapes " +
                 "that frame the scene and read instantly at zero detail cost).")]
        public bool foregroundSilhouettes = false;
        [Tooltip("Colour of the foreground silhouette layer.")]
        public Color silhouetteColor = new Color(0f, 0f, 0f, 1f);
        [Tooltip("Give the player, pickups and goal a soft emissive halo so they " +
                 "pop against the darkness. Cheap way to guide the eye.")]
        public bool focalGlow = false;

        [Header("Layout feel")]
        [Tooltip("Approx. number of ground segments (level length).")]
        public Vector2Int segmentCountRange = new Vector2Int(10, 16);
        [Tooltip("Horizontal gap between platforms, in units. Kept jumpable.")]
        public Vector2 gapRange = new Vector2(1.5f, 4.5f);
        [Tooltip("Vertical step between platforms, in units. Kept jumpable.")]
        public Vector2 stepRange = new Vector2(-2.5f, 2.5f);
        [Tooltip("Platform width range, in units.")]
        public Vector2 widthRange = new Vector2(3f, 9f);
        [Range(0f, 1f)] public float floatingPlatformChance = 0.35f;
        [Range(0f, 1f)] public float movingPlatformChance = 0.18f;

        [Header("Danger & reward")]
        [Range(0f, 1f)] public float enemyDensity = 0.5f;
        [Range(0f, 1f)] public float turretChance = 0.2f;
        [Range(0f, 1f)] public float hazardDensity = 0.3f;
        [Range(0f, 1f)] public float coinDensity = 0.6f;
        [Range(0f, 1f)] public float gemChance = 0.25f;

        [Header("Physics feel")]
        [Tooltip("Multiplier on world gravity (lower = floatier/moon-like).")]
        public float gravityScale = 1f;
        [Tooltip("Multiplier on player run speed.")]
        public float playerSpeedScale = 1f;
        [Tooltip("Multiplier on player jump height.")]
        public float jumpScale = 1f;

        [Header("Audio feel")]
        [Tooltip("Root MIDI note for the generated music (lower = darker).")]
        public int musicRootNote = 57; // A3
        [Tooltip("0 = minor (tense), 1 = major (bright), 2 = pentatonic (airy).")]
        public int musicMode = 1;
        [Tooltip("Music tempo in BPM.")]
        public float musicTempo = 110f;

        [Header("Objective bias")]
        [Tooltip("Relative weights for each ObjectiveType (index matches enum).")]
        public float[] objectiveWeights = { 1f, 0.8f, 0.8f, 0.6f, 0.4f };

        public Color SkyMid => Color.Lerp(skyBottom, skyTop, 0.5f);

        /// <summary>Pick an objective for this theme using its weights + RNG.</summary>
        public ObjectiveType PickObjective(System.Random rng)
        {
            float total = 0f;
            foreach (var w in objectiveWeights) total += Mathf.Max(0f, w);
            if (total <= 0f) return ObjectiveType.ReachGoal;

            double roll = rng.NextDouble() * total;
            for (int i = 0; i < objectiveWeights.Length; i++)
            {
                roll -= Mathf.Max(0f, objectiveWeights[i]);
                if (roll <= 0) return (ObjectiveType)i;
            }
            return ObjectiveType.ReachGoal;
        }
    }
}
