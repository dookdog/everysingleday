using UnityEngine;
using EverySingleDay.Player;
using EverySingleDay.Enemies;
using EverySingleDay.Collectibles;
using EverySingleDay.Level;
using EverySingleDay.CameraSystem;
using EverySingleDay.UI;

namespace EverySingleDay.Systems
{
    /// <summary>
    /// Builds a complete, playable level at runtime from a seed + theme. Rather
    /// than a hand-authored layout, it asks <see cref="LevelGenerator"/> for a
    /// fresh <see cref="LevelData"/> and renders it: player, camera, HUD, themed
    /// backdrop, platforms, enemies, pickups, hazards, checkpoint and goal.
    ///
    /// Every new play uses a different seed (unless one is pinned), so the
    /// design — layout, enemies, objective and overall feel — changes each game.
    /// The aesthetic comes from the chosen <see cref="LevelTheme"/>; themes are
    /// authored in <see cref="ThemeLibrary"/> from reference imagery, so the look
    /// is something you direct while the structure is generated.
    ///
    /// No prefabs, art or network required — sprites are code-drawn primitives.
    /// </summary>
    public class GameBootstrap : MonoBehaviour
    {
        [Header("Build Options")]
        public bool buildOnStart = true;

        [Tooltip("0 = random seed each play. Set non-zero to replay one design.")]
        public int seed = 0;

        [Tooltip("-1 = pick a theme from the seed. Otherwise force a theme index.")]
        public int forceThemeIndex = -1;

        public static bool AutoSpawnIfMissing = true;

        // Cross-load handoff: the menu / next-level flow can request the next
        // seed and theme so progression feels intentional.
        public static int NextSeed = 0;
        public static int NextThemeIndex = -1;

        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void AutoSpawn()
        {
            if (!AutoSpawnIfMissing) return;
            var sceneName = UnityEngine.SceneManagement.SceneManager.GetActiveScene().name;
            if (sceneName.ToLowerInvariant().Contains("menu")) return; // menu handles itself
            if (FindObjectOfType<GameBootstrap>() != null) return;
            var go = new GameObject("Bootstrap (auto)");
            go.AddComponent<GameBootstrap>();
        }

        // Ground occupies user layer 8 so the player's probes never detect itself.
        private const int GroundLayer = 8;
        private LayerMask GroundMask => 1 << GroundLayer;

        private PhysicsMaterial2D _frictionless;
        private LevelTheme _theme;
        private LevelData _level;

        private void Start()
        {
            if (buildOnStart) Build();
        }

        public void Build()
        {
            // Resolve seed + theme (honouring any cross-load handoff).
            int resolvedSeed = seed != 0 ? seed
                             : NextSeed != 0 ? NextSeed
                             : Random.Range(1, int.MaxValue);
            NextSeed = 0;

            int themeIdx = forceThemeIndex >= 0 ? forceThemeIndex
                         : NextThemeIndex >= 0 ? NextThemeIndex
                         : -1;
            NextThemeIndex = -1;

            _theme = themeIdx >= 0 ? ThemeLibrary.Get(themeIdx)
                                   : ThemeLibrary.PickForSeed(resolvedSeed);

            _level = LevelGenerator.Generate(resolvedSeed, _theme);

            _frictionless = new PhysicsMaterial2D("Slippery") { friction = 0f, bounciness = 0f };

            BuildSystems();
            var player = BuildPlayer(_level.playerStart);
            BuildCamera(player.transform);
            BuildBackdrop();
            RenderLevel();

            // Negative-space atmosphere layered on top of the built level.
            if (_theme.foregroundSilhouettes && Camera.main != null)
                ForegroundSilhouettes.Build(_theme, _level, Camera.main.transform);
            AtmosphereController.Attach(Camera.main, _theme);

            RuntimeHUD.Create();
            ShowIntroCard();
        }

        // ---------------------------------------------------------------- systems
        private void BuildSystems()
        {
            if (GameManager.Instance == null)
                new GameObject("GameManager").AddComponent<GameManager>();

            if (AudioManager.Instance == null)
                new GameObject("AudioManager").AddComponent<AudioManager>();
            // Themed music for this level.
            AudioManager.PlayMusic(SfxLibrary.BuildThemedMusic(
                _theme.musicRootNote, _theme.musicMode, _theme.musicTempo));

            // Objective tracker, configured for the generated objective.
            var omGo = new GameObject("ObjectiveManager");
            var om = omGo.AddComponent<ObjectiveManager>();
            om.Configure(_level);

            if (Object.FindObjectOfType<UnityEngine.EventSystems.EventSystem>() == null)
            {
                var es = new GameObject("EventSystem");
                es.AddComponent<UnityEngine.EventSystems.EventSystem>();
                es.AddComponent<UnityEngine.EventSystems.StandaloneInputModule>();
            }
        }

        // ----------------------------------------------------------------- player
        private GameObject BuildPlayer(Vector3 position)
        {
            var go = new GameObject("Player") { tag = "Player" };
            go.transform.position = position;

            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.SolidSprite(_theme.player);
            sr.drawMode = SpriteDrawMode.Sliced;
            sr.size = new Vector2(1f, 1.4f);
            sr.sortingOrder = 10;

            // Eyes: only draw them when the player body is light enough for them
            // to read. On a dark silhouette player, use lit accent "eyes" so the
            // character still has a focal point in the dark.
            float playerLuma = _theme.player.r * 0.3f + _theme.player.g * 0.6f + _theme.player.b * 0.1f;
            Color eyeColor = playerLuma > 0.4f ? Color.black : _theme.accent;
            AddDecal(go.transform, new Vector2(0.22f, 0.22f), new Vector2(0.18f, 0.2f), eyeColor, 11);
            AddDecal(go.transform, new Vector2(-0.22f, 0.22f), new Vector2(0.18f, 0.2f), eyeColor, 11);

            // Focal glow so the player reads against the darkness.
            if (_theme.focalGlow)
                AtmosphereController.AddFocalGlow(go.transform, _theme.accent, 3.2f);

            var rb = go.AddComponent<Rigidbody2D>();
            rb.gravityScale = 1f;

            var col = go.AddComponent<CapsuleCollider2D>();
            col.size = new Vector2(0.9f, 1.4f);
            col.direction = CapsuleDirection2D.Vertical;
            col.sharedMaterial = _frictionless;

            var groundCheck = new GameObject("GroundCheck").transform;
            groundCheck.SetParent(go.transform, false);
            groundCheck.localPosition = new Vector3(0f, -0.72f, 0f);

            var wallCheck = new GameObject("WallCheck").transform;
            wallCheck.SetParent(go.transform, false);
            wallCheck.localPosition = new Vector3(0.45f, 0f, 0f);

            var controller = go.AddComponent<PlayerController>();
            controller.groundLayer = GroundMask;
            controller.groundCheck = groundCheck;
            controller.wallCheck = wallCheck;
            // Apply the theme's physics feel.
            controller.moveSpeed *= _theme.playerSpeedScale;
            controller.jumpHeight *= _theme.jumpScale;

            var health = go.AddComponent<PlayerHealth>();
            health.spriteToFlash = sr;
            health.SetRespawnPoint(position);

            // Theme gravity: scale the world gravity once (affects all bodies).
            Physics2D.gravity = new Vector2(0f, -30f * _theme.gravityScale);

            return go;
        }

        private void AddDecal(Transform parent, Vector2 pos, Vector2 size, Color color, int order)
        {
            var go = new GameObject("Decal");
            go.transform.SetParent(parent, false);
            go.transform.localPosition = pos;
            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.SolidSprite(color);
            sr.drawMode = SpriteDrawMode.Sliced;
            sr.size = size;
            sr.sortingOrder = order;
        }

        // ----------------------------------------------------------------- camera
        private void BuildCamera(Transform target)
        {
            Camera cam = Camera.main;
            GameObject camGo;
            if (cam == null)
            {
                camGo = new GameObject("Main Camera") { tag = "MainCamera" };
                cam = camGo.AddComponent<Camera>();
            }
            else camGo = cam.gameObject;

            cam.orthographic = true;
            cam.orthographicSize = 6.5f;
            cam.backgroundColor = _theme.skyBottom;
            cam.clearFlags = CameraClearFlags.SolidColor;
            cam.transform.position = new Vector3(target.position.x, target.position.y, -10f);

            var follow = camGo.GetComponent<CameraFollow>() ?? camGo.AddComponent<CameraFollow>();
            follow.target = target;
            follow.offset = new Vector2(0f, 1f);
            follow.useBounds = true;
            follow.minBounds = _level.worldMin;
            follow.maxBounds = _level.worldMax;

            if (camGo.GetComponent<CameraShake>() == null)
                camGo.AddComponent<CameraShake>();

            // A simple two-band sky using a big background quad behind everything.
            BuildSky(cam);
        }

        private void BuildSky(Camera cam)
        {
            var go = new GameObject("Sky");
            go.transform.SetParent(cam.transform, false);
            go.transform.localPosition = new Vector3(0f, 0f, 20f);
            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.SolidSprite(_theme.skyTop);
            sr.drawMode = SpriteDrawMode.Sliced;
            sr.size = new Vector2(80f, 50f);
            sr.color = _theme.skyTop;
            sr.sortingOrder = -100;
        }

        // ------------------------------------------------------------------ level
        private void RenderLevel()
        {
            foreach (var p in _level.platforms)
            {
                if (p.moving) MovingBridge(p);
                else Platform(p);
            }
            foreach (var e in _level.enemies)
            {
                if (e.turret) Turret(e.pos);
                else Enemy(e.pos, e.alt);
            }
            foreach (var pk in _level.pickups) Pickup(pk);
            foreach (var hz in _level.hazards) SpikeRow(hz.pos, hz.count);

            CheckpointFlag(_level.checkpointPos);
            GoalFlag(_level.goalPos);
            DeathPlane(new Vector3((_level.worldMin.x + _level.worldMax.x) / 2f,
                _level.worldMin.y - 2f, 0f), (_level.worldMax.x - _level.worldMin.x) + 40f);
        }

        private GameObject Platform(PlatformSpec p)
        {
            float height = 2f;
            var go = new GameObject("Platform");
            go.layer = GroundLayer;
            go.transform.position = new Vector3(p.centerX, p.topY - height / 2f, 0f);

            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.SolidSprite(_theme.ground);
            sr.drawMode = SpriteDrawMode.Tiled;
            sr.size = new Vector2(p.width, height);
            sr.sortingOrder = 0;

            var col = go.AddComponent<BoxCollider2D>();
            col.size = new Vector2(p.width, height);
            col.sharedMaterial = _frictionless;

            var cap = new GameObject("Cap");
            cap.transform.SetParent(go.transform, false);
            cap.transform.localPosition = new Vector3(0f, height / 2f - 0.15f, 0f);
            var gsr = cap.AddComponent<SpriteRenderer>();
            gsr.sprite = PrimitiveFactory.SolidSprite(_theme.groundCap);
            gsr.drawMode = SpriteDrawMode.Tiled;
            gsr.size = new Vector2(p.width, 0.3f);
            gsr.sortingOrder = 1;

            return go;
        }

        private void MovingBridge(PlatformSpec p)
        {
            var go = new GameObject("MovingPlatform");
            go.layer = GroundLayer;
            go.transform.position = new Vector3(p.centerX, p.topY - 0.25f, 0f);

            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.SolidSprite(_theme.platform);
            sr.drawMode = SpriteDrawMode.Tiled;
            sr.size = new Vector2(Mathf.Max(2.5f, p.width), 0.5f);
            sr.sortingOrder = 2;

            var col = go.AddComponent<BoxCollider2D>();
            col.size = new Vector2(Mathf.Max(2.5f, p.width), 0.5f);
            col.sharedMaterial = _frictionless;

            var mp = go.AddComponent<MovingPlatform>();
            mp.waypoints = new[] { Vector2.zero, p.moveTravel };
            mp.speed = 2.2f;
            mp.waitTime = 0.5f;
        }

        private void Pickup(PickupSpec pk)
        {
            var go = new GameObject(pk.gem ? "Gem" : "Coin");
            go.transform.position = pk.pos;

            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.CircleSprite(pk.gem ? _theme.gem : _theme.coin);
            sr.drawMode = SpriteDrawMode.Sliced;
            sr.size = pk.gem ? new Vector2(0.6f, 0.6f) : new Vector2(0.5f, 0.5f);
            sr.sortingOrder = 5;

            var col = go.AddComponent<CircleCollider2D>();
            col.radius = 0.35f;
            col.isTrigger = true;

            var c = go.AddComponent<Collectible>();
            c.type = pk.gem ? CollectibleType.Gem : CollectibleType.Coin;
            c.value = pk.gem ? 50 : 10;
            c.bob = true;

            // Pickups glow so they draw the eye across the dark negative space.
            if (_theme.focalGlow)
                AtmosphereController.AddFocalGlow(go.transform,
                    pk.gem ? _theme.gem : _theme.coin, pk.gem ? 1.8f : 1.4f);
        }

        private void Enemy(Vector3 pos, bool alt)
        {
            var go = new GameObject("Enemy");
            go.transform.position = pos;

            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.SolidSprite(alt ? _theme.enemyAlt : _theme.enemy);
            sr.drawMode = SpriteDrawMode.Sliced;
            sr.size = new Vector2(0.9f, 0.9f);
            sr.sortingOrder = 6;

            AddDecal(go.transform, new Vector2(0.2f, 0.1f), new Vector2(0.16f, 0.16f), _theme.accent, 7);
            AddDecal(go.transform, new Vector2(-0.2f, 0.1f), new Vector2(0.16f, 0.16f), _theme.accent, 7);

            var rb = go.AddComponent<Rigidbody2D>();
            rb.freezeRotation = true;

            var col = go.AddComponent<BoxCollider2D>();
            col.size = new Vector2(0.9f, 0.9f);

            var groundCheck = new GameObject("GroundCheck").transform;
            groundCheck.SetParent(go.transform, false);
            groundCheck.localPosition = new Vector3(0.45f, -0.45f, 0f);

            var wallCheck = new GameObject("WallCheck").transform;
            wallCheck.SetParent(go.transform, false);
            wallCheck.localPosition = new Vector3(0.45f, 0f, 0f);

            var enemy = go.AddComponent<PatrolEnemy>();
            enemy.groundLayer = GroundMask;
            enemy.groundCheck = groundCheck;
            enemy.wallCheck = wallCheck;
        }

        private void Turret(Vector3 pos)
        {
            var go = new GameObject("Turret");
            go.transform.position = pos;
            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.SolidSprite(_theme.enemyAlt);
            sr.drawMode = SpriteDrawMode.Sliced;
            sr.size = new Vector2(0.9f, 0.9f);
            sr.sortingOrder = 6;

            var firePoint = new GameObject("FirePoint").transform;
            firePoint.SetParent(go.transform, false);
            firePoint.localPosition = new Vector3(0f, 0.1f, 0f);

            var turret = go.AddComponent<TurretEnemy>();
            turret.firePoint = firePoint;
            turret.fireInterval = 2.2f;
            turret.detectionRange = 9f;
            turret.projectilePrefab = BuildProjectilePrefab();
        }

        private Projectile BuildProjectilePrefab()
        {
            var go = new GameObject("ProjectileTemplate");
            go.SetActive(false);
            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.CircleSprite(_theme.hazard);
            sr.drawMode = SpriteDrawMode.Sliced;
            sr.size = new Vector2(0.35f, 0.35f);
            sr.sortingOrder = 8;

            var col = go.AddComponent<CircleCollider2D>();
            col.radius = 0.18f;
            col.isTrigger = true;

            go.AddComponent<Rigidbody2D>().gravityScale = 0f;
            var proj = go.AddComponent<Projectile>();
            proj.groundLayer = GroundMask;
            proj.speed = 7f;
            return proj;
        }

        private void SpikeRow(Vector3 basePos, int count)
        {
            for (int i = 0; i < count; i++)
            {
                var go = new GameObject("Spike");
                go.transform.position = basePos + new Vector3(i * 0.7f, 0.35f, 0f);
                var sr = go.AddComponent<SpriteRenderer>();
                sr.sprite = PrimitiveFactory.SpikeSprite(_theme.hazard);
                sr.drawMode = SpriteDrawMode.Sliced;
                sr.size = new Vector2(0.7f, 0.7f);
                sr.sortingOrder = 4;

                var col = go.AddComponent<BoxCollider2D>();
                col.size = new Vector2(0.5f, 0.5f);
                col.offset = new Vector2(0f, -0.1f);
                col.isTrigger = true;

                go.AddComponent<Hazard>().instantKill = true;
            }
        }

        private void CheckpointFlag(Vector3 pos)
        {
            var go = new GameObject("Checkpoint");
            go.transform.position = pos;

            var pole = new GameObject("Pole");
            pole.transform.SetParent(go.transform, false);
            var psr = pole.AddComponent<SpriteRenderer>();
            psr.sprite = PrimitiveFactory.SolidSprite(_theme.accent);
            psr.drawMode = SpriteDrawMode.Sliced;
            psr.size = new Vector2(0.12f, 2.2f);
            psr.sortingOrder = 3;

            var flag = new GameObject("Flag");
            flag.transform.SetParent(go.transform, false);
            flag.transform.localPosition = new Vector3(0.4f, 0.8f, 0f);
            var fsr = flag.AddComponent<SpriteRenderer>();
            fsr.drawMode = SpriteDrawMode.Sliced;
            fsr.size = new Vector2(0.7f, 0.5f);
            fsr.sortingOrder = 4;

            var col = go.AddComponent<BoxCollider2D>();
            col.size = new Vector2(1f, 2.2f);
            col.isTrigger = true;

            var cp = go.AddComponent<Checkpoint>();
            cp.flagRenderer = fsr;
            cp.inactiveSprite = PrimitiveFactory.SolidSprite(_theme.platform);
            cp.activeSprite = PrimitiveFactory.SolidSprite(_theme.coin);
            fsr.sprite = cp.inactiveSprite;
        }

        private void GoalFlag(Vector3 pos)
        {
            var go = new GameObject("Goal");
            go.transform.position = pos;

            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.SolidSprite(_theme.goal);
            sr.drawMode = SpriteDrawMode.Sliced;
            sr.size = new Vector2(1.2f, 3f);
            sr.sortingOrder = 3;

            AddDecal(go.transform, new Vector2(0f, 1.8f), new Vector2(0.6f, 0.6f), _theme.accent, 4);

            var col = go.AddComponent<BoxCollider2D>();
            col.size = new Vector2(1.2f, 3f);
            col.isTrigger = true;

            var goal = go.AddComponent<LevelGoal>();
            goal.bodyRenderer = sr;

            // The exit is the brightest thing in the level — a beacon in the dark.
            if (_theme.focalGlow)
                AtmosphereController.AddFocalGlow(go.transform, _theme.goal, 4.5f);
        }

        private void DeathPlane(Vector3 center, float width)
        {
            var go = new GameObject("DeathZone");
            go.transform.position = center;
            var col = go.AddComponent<BoxCollider2D>();
            col.size = new Vector2(width, 4f);
            col.isTrigger = true;
            go.AddComponent<DeathZone>();
        }

        // --------------------------------------------------------------- backdrop
        private void BuildBackdrop()
        {
            var mover = new GameObject("Backdrop").AddComponent<Parallax>();
            var layers = new System.Collections.Generic.List<Parallax.Layer>();

            float span = _level.worldMax.x - _level.worldMin.x;
            int perLayer = Mathf.Clamp(Mathf.CeilToInt(span / 8f) + 2, 6, 24);

            for (int layer = 0; layer < _theme.backdropTints.Length; layer++)
            {
                var layerRoot = new GameObject($"BGLayer{layer}");
                layerRoot.transform.position = Vector3.zero;

                for (int i = 0; i < perLayer; i++)
                {
                    var go = new GameObject("Shape");
                    go.transform.SetParent(layerRoot.transform, false);
                    float x = _level.worldMin.x + i * 8f + layer * 3f;
                    float y = _level.worldMin.y + 10f + layer * 1.2f;
                    go.transform.position = new Vector3(x, y, 5f + layer);

                    var sr = go.AddComponent<SpriteRenderer>();
                    sr.sprite = BackdropSprite(_theme.backdropShape, _theme.backdropTints[layer]);
                    sr.drawMode = SpriteDrawMode.Sliced;
                    float s = 10f - layer * 1.5f;
                    sr.size = new Vector2(s, s);
                    sr.sortingOrder = -50 + layer;
                }

                layers.Add(new Parallax.Layer
                {
                    transform = layerRoot.transform,
                    parallaxFactor = 0.2f + layer * 0.2f
                });
            }

            mover.layers = layers.ToArray();
            if (Camera.main != null) mover.cameraTransform = Camera.main.transform;
        }

        private Sprite BackdropSprite(int shape, Color color)
        {
            switch (shape)
            {
                case 1: return PrimitiveFactory.SpikeSprite(color);   // jagged peaks
                case 2: return PrimitiveFactory.SolidSprite(color);   // floating blocks
                default: return PrimitiveFactory.CircleSprite(color); // rounded hills
            }
        }

        // --------------------------------------------------------------- intro UI
        private void ShowIntroCard()
        {
            LevelIntroCard.Show(_theme.displayName, _theme.mood,
                ObjectiveManager.Instance != null ? ObjectiveManager.Instance.Description : "Reach the exit",
                _theme.accent, _theme.skyTop);
        }
    }
}
