using System.Collections.Generic;
using UnityEngine;

namespace EverySingleDay.Systems
{
    // --- plain data describing a level, produced by the generator and consumed
    //     by the bootstrap. Keeping generation separate from instantiation makes
    //     the layout testable and the bootstrap a dumb renderer. ---

    public struct PlatformSpec
    {
        public float centerX, topY, width;
        public bool moving;
        public Vector2 moveTravel;
    }

    public struct EnemySpec
    {
        public Vector3 pos;
        public bool turret;
        public bool alt; // use the theme's alternate enemy colour
    }

    public struct PickupSpec
    {
        public Vector3 pos;
        public bool gem;
    }

    public struct HazardSpec
    {
        public Vector3 pos;
        public int count;
    }

    /// <summary>The full description of one generated level.</summary>
    public class LevelData
    {
        public int seed;
        public LevelTheme theme;
        public ObjectiveType objective;
        public int quota;            // for CollectQuota
        public float surviveTime;    // for Survive
        public Vector3 playerStart;
        public Vector3 goalPos;
        public Vector3 checkpointPos;
        public Vector2 worldMin, worldMax;

        public readonly List<PlatformSpec> platforms = new();
        public readonly List<EnemySpec> enemies = new();
        public readonly List<PickupSpec> pickups = new();
        public readonly List<HazardSpec> hazards = new();

        public int GemCount
        {
            get { int n = 0; foreach (var p in pickups) if (p.gem) n++; return n; }
        }
    }

    /// <summary>
    /// Deterministic, seed-driven level generator. Given a theme and a seed it
    /// produces a complete, *guaranteed-playable* level: every platform is
    /// reachable from the previous one because gaps and vertical steps are
    /// clamped to what the player can actually jump (derived from the theme's
    /// jump/gravity feel). Enemies, pickups and hazards are scattered according
    /// to the theme's densities, and an objective is chosen from the theme bias.
    ///
    /// Same seed + theme => identical level. Different seed => a different
    /// design every play, including its enemies, objective and feel.
    /// </summary>
    public static class LevelGenerator
    {
        public static LevelData Generate(int seed, LevelTheme theme)
        {
            var rng = new System.Random(seed);
            var data = new LevelData { seed = seed, theme = theme };

            // Reachability budget. The base controller jumps ~3.6u high and the
            // theme scales it; we stay conservatively under the true max so the
            // generated jumps always feel comfortable, not frame-perfect.
            float maxJumpHeight = 3.4f * theme.jumpScale;
            float maxUpStep = Mathf.Min(theme.stepRange.y, maxJumpHeight - 0.6f);
            float maxDownStep = theme.stepRange.x;
            // Horizontal reach grows with speed and (via airtime) lower gravity.
            float maxGap = Mathf.Min(theme.gapRange.y,
                3.0f * theme.playerSpeedScale / Mathf.Sqrt(theme.gravityScale));

            int segments = rng.Next(theme.segmentCountRange.x, theme.segmentCountRange.y + 1);

            float cursorX = 0f;
            float lastTopY = 0f;
            float minY = 0f, maxY = 0f;

            // First platform: a safe, flat spawn pad.
            float firstWidth = 6f;
            data.platforms.Add(new PlatformSpec { centerX = firstWidth / 2f, topY = 0f, width = firstWidth });
            data.playerStart = new Vector3(firstWidth / 2f, 2f, 0f);
            cursorX = firstWidth;
            int checkpointIndex = segments / 2;

            for (int i = 1; i < segments; i++)
            {
                // Gap then a new platform.
                float gap = Mathf.Lerp(theme.gapRange.x, maxGap, (float)rng.NextDouble());
                float step = Mathf.Lerp(maxDownStep, maxUpStep, (float)rng.NextDouble());
                float topY = Mathf.Clamp(lastTopY + step, -2f, 9f);
                float width = Mathf.Lerp(theme.widthRange.x, theme.widthRange.y, (float)rng.NextDouble());

                float centerX = cursorX + gap + width / 2f;

                var spec = new PlatformSpec { centerX = centerX, topY = topY, width = width };

                // Some platforms move (carrying the player) when the theme likes it.
                if (rng.NextDouble() < theme.movingPlatformChance && i != checkpointIndex)
                {
                    spec.moving = true;
                    bool vertical = rng.NextDouble() < 0.4;
                    spec.moveTravel = vertical ? new Vector2(0f, Random(rng, 1.5f, 3f))
                                               : new Vector2(Random(rng, 2f, 4f), 0f);
                }
                data.platforms.Add(spec);

                PopulateSegment(rng, theme, spec, data, i == checkpointIndex);

                // Optional floating bonus platform above the path.
                if (rng.NextDouble() < theme.floatingPlatformChance)
                {
                    float fY = Mathf.Clamp(topY + Random(rng, 2.5f, 3.5f), 0f, 11f);
                    float fW = Random(rng, 1.5f, 3f);
                    data.platforms.Add(new PlatformSpec { centerX = centerX, topY = fY, width = fW });
                    // Reward for the climb.
                    data.pickups.Add(new PickupSpec
                    {
                        pos = new Vector3(centerX, fY + 0.8f, 0f),
                        gem = rng.NextDouble() < 0.6
                    });
                }

                if (i == checkpointIndex)
                    data.checkpointPos = new Vector3(centerX, topY + 1.2f, 0f);

                cursorX = centerX + width / 2f;
                lastTopY = topY;
                minY = Mathf.Min(minY, topY);
                maxY = Mathf.Max(maxY, topY);
            }

            // Goal sits on the last platform.
            var last = data.platforms[data.platforms.Count - 1];
            data.goalPos = new Vector3(last.centerX, last.topY + 1.5f, 0f);

            data.worldMin = new Vector2(-2f, minY - 16f);
            data.worldMax = new Vector2(cursorX + 4f, maxY + 14f);

            ChooseObjective(rng, theme, data);
            return data;
        }

        private static void PopulateSegment(System.Random rng, LevelTheme theme,
            PlatformSpec p, LevelData data, bool isCheckpoint)
        {
            float left = p.centerX - p.width / 2f + 1f;
            float right = p.centerX + p.width / 2f - 1f;
            float surface = p.topY + 0.7f;

            // Don't clutter the spawn-side of a checkpoint platform.
            if (!isCheckpoint && !p.moving)
            {
                // Enemies — ground patrol or a turret.
                if (rng.NextDouble() < theme.enemyDensity && p.width >= 3f)
                {
                    bool turret = rng.NextDouble() < theme.turretChance;
                    data.enemies.Add(new EnemySpec
                    {
                        pos = new Vector3(Mathf.Lerp(left, right, (float)rng.NextDouble()),
                                          p.topY + 0.9f, 0f),
                        turret = turret,
                        alt = rng.NextDouble() < 0.5
                    });
                }

                // Hazards — a short spike row near one edge, only on wide platforms.
                if (rng.NextDouble() < theme.hazardDensity && p.width >= 5f)
                {
                    int count = rng.Next(2, 4);
                    float hx = rng.NextDouble() < 0.5 ? left + 0.5f : right - count * 0.7f;
                    data.hazards.Add(new HazardSpec
                    {
                        pos = new Vector3(hx, p.topY, 0f),
                        count = count
                    });
                }
            }

            // Coins / gems — a little arc floating above the platform.
            if (rng.NextDouble() < theme.coinDensity)
            {
                int n = rng.Next(2, 5);
                float startX = Mathf.Lerp(left, Mathf.Max(left, right - n * 0.8f), (float)rng.NextDouble());
                bool gem = rng.NextDouble() < theme.gemChance;
                for (int k = 0; k < n; k++)
                {
                    float arc = Mathf.Sin((k + 1f) / (n + 1f) * Mathf.PI) * 0.8f;
                    data.pickups.Add(new PickupSpec
                    {
                        pos = new Vector3(startX + k * 0.8f, surface + 0.8f + arc, 0f),
                        gem = gem
                    });
                }
            }
        }

        private static void ChooseObjective(System.Random rng, LevelTheme theme, LevelData data)
        {
            var obj = theme.PickObjective(rng);

            // Guard against degenerate objectives for the rolled level.
            if (obj == ObjectiveType.CollectAllGems && data.GemCount == 0)
                obj = ObjectiveType.ReachGoal;
            if (obj == ObjectiveType.DefeatAllEnemies && data.enemies.Count == 0)
                obj = ObjectiveType.ReachGoal;

            data.objective = obj;

            if (obj == ObjectiveType.CollectQuota)
            {
                int gems = data.GemCount;
                if (gems == 0) { data.objective = ObjectiveType.ReachGoal; }
                else data.quota = Mathf.Max(1, Mathf.CeilToInt(gems * 0.6f));
            }

            if (obj == ObjectiveType.Survive)
                data.surviveTime = Random(rng, 20f, 35f);
        }

        private static float Random(System.Random rng, float a, float b)
            => Mathf.Lerp(a, b, (float)rng.NextDouble());
    }
}
