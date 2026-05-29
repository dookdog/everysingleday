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
    /// One-stop scene builder. Drop this single component on an empty GameObject
    /// in an empty scene, press Play, and a complete platformer level — player,
    /// camera, HUD, enemies, coins, hazards, moving platforms, checkpoints and a
    /// goal — is constructed procedurally. No prefabs, art or scene authoring
    /// required, so the game runs the moment the project is opened.
    ///
    /// Everything it builds uses the reusable gameplay components in this
    /// project, so it doubles as living documentation of how to wire them up.
    /// </summary>
    public class GameBootstrap : MonoBehaviour
    {
        [Header("Build Options")]
        public bool buildOnStart = true;

        [Tooltip("If true and no GameBootstrap is present in the loaded scene, " +
                 "one is created automatically so the game runs from any empty scene.")]
        public static bool AutoSpawnIfMissing = true;

        /// <summary>
        /// Safety net: if you press Play in an empty scene (or one without a
        /// Bootstrap object), spawn one so the full game still builds. Skipped
        /// when a GameBootstrap already exists, to avoid building twice.
        /// </summary>
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void AutoSpawn()
        {
            if (!AutoSpawnIfMissing) return;
            if (FindObjectOfType<GameBootstrap>() != null) return;
            var go = new GameObject("Bootstrap (auto)");
            go.AddComponent<GameBootstrap>();
        }

        // Ground occupies user layer 8 so the player's ground/wall probes never
        // detect the player itself. Layer 8 always exists (even if unnamed).
        private const int GroundLayer = 8;
        private LayerMask GroundMask => 1 << GroundLayer;

        // Palette.
        private static readonly Color SkyColor = new Color(0.36f, 0.62f, 0.86f);
        private static readonly Color GroundColor = new Color(0.27f, 0.20f, 0.16f);
        private static readonly Color GrassColor = new Color(0.32f, 0.65f, 0.30f);
        private static readonly Color PlayerColor = new Color(0.95f, 0.85f, 0.30f);
        private static readonly Color EnemyColor = new Color(0.85f, 0.27f, 0.27f);
        private static readonly Color CoinColor = new Color(1f, 0.82f, 0.18f);
        private static readonly Color GemColor = new Color(0.40f, 0.85f, 0.95f);
        private static readonly Color SpikeColor = new Color(0.75f, 0.78f, 0.82f);
        private static readonly Color PlatformColor = new Color(0.55f, 0.40f, 0.70f);
        private static readonly Color CheckpointColor = new Color(0.95f, 0.55f, 0.20f);
        private static readonly Color GoalColor = new Color(0.30f, 0.90f, 0.45f);

        private PhysicsMaterial2D _frictionless;

        private void Start()
        {
            if (buildOnStart) Build();
        }

        public void Build()
        {
            _frictionless = new PhysicsMaterial2D("Slippery") { friction = 0f, bounciness = 0f };

            BuildSystems();
            var player = BuildPlayer(new Vector3(0f, 2f, 0f));
            BuildCamera(player.transform);
            BuildLevel();
            RuntimeHUD.Create();
        }

        // ---------------------------------------------------------------- systems
        private void BuildSystems()
        {
            if (GameManager.Instance == null)
            {
                var gmGo = new GameObject("GameManager");
                gmGo.AddComponent<GameManager>();
            }
            if (AudioManager.Instance == null)
            {
                var amGo = new GameObject("AudioManager");
                amGo.AddComponent<AudioManager>();
            }

            // EventSystem so UI buttons would work if added later.
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
            sr.sprite = PrimitiveFactory.SolidSprite(PlayerColor);
            sr.drawMode = SpriteDrawMode.Sliced;
            sr.size = new Vector2(1f, 1.4f);
            sr.sortingOrder = 10;

            // A friendly face so it reads as a character.
            AddDecal(go.transform, new Vector2(0.22f, 0.22f), new Vector2(0.18f, 0.2f), Color.black, 11);
            AddDecal(go.transform, new Vector2(-0.22f, 0.22f), new Vector2(0.18f, 0.2f), Color.black, 11);

            var rb = go.AddComponent<Rigidbody2D>();
            rb.gravityScale = 1f;

            var col = go.AddComponent<CapsuleCollider2D>();
            col.size = new Vector2(0.9f, 1.4f);
            col.direction = CapsuleDirection2D.Vertical;
            col.sharedMaterial = _frictionless;

            // Probes.
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

            var health = go.AddComponent<PlayerHealth>();
            health.spriteToFlash = sr;

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
            GameObject camGo;
            Camera cam = Camera.main;
            if (cam == null)
            {
                camGo = new GameObject("Main Camera") { tag = "MainCamera" };
                cam = camGo.AddComponent<Camera>();
            }
            else camGo = cam.gameObject;

            cam.orthographic = true;
            cam.orthographicSize = 6.5f;
            cam.backgroundColor = SkyColor;
            cam.clearFlags = CameraClearFlags.SolidColor;
            cam.transform.position = new Vector3(target.position.x, target.position.y, -10f);

            var follow = camGo.GetComponent<CameraFollow>() ?? camGo.AddComponent<CameraFollow>();
            follow.target = target;
            follow.offset = new Vector2(0f, 1f);

            if (camGo.GetComponent<CameraShake>() == null)
                camGo.AddComponent<CameraShake>();
        }

        // ------------------------------------------------------------------ level
        private void BuildLevel()
        {
            // --- ground platforms: (centerX, topY, width) ---
            Platform(4f, 0f, 14f);
            Platform(20f, 0f, 8f);
            Platform(30f, 1.5f, 6f);
            Platform(50f, 0f, 9f);
            Platform(60f, 2.5f, 5f);
            Platform(70f, 0f, 8f);
            Platform(80f, 3.5f, 6f);
            Platform(92f, 0f, 12f);

            // --- floating bonus blocks ---
            Platform(15f, 3.5f, 2f);
            Platform(45f, 3f, 2f);

            // --- moving platform bridging the big gap (x 36 -> 44 at y ~1.5) ---
            MovingBridge(new Vector3(36f, 1.5f, 0f), new Vector2(8f, 0f));

            // --- coins: a few inviting arcs ---
            CoinArc(new Vector2(11f, 1.2f), 4, 0.8f);
            CoinArc(new Vector2(15f, 4.5f), 1, 0f);   // reward on the high block
            CoinArc(new Vector2(28f, 2.7f), 3, 0.8f);
            CoinArc(new Vector2(40f, 3.5f), 3, 0.8f, true);  // gems over the moving gap
            CoinArc(new Vector2(64f, 2.5f), 3, 0.8f);
            CoinArc(new Vector2(80f, 5.0f), 4, 0.7f, true);  // gem stash up the stairs

            // --- enemies patrolling the platforms ---
            Enemy(new Vector3(20f, 1f, 0f));
            Enemy(new Vector3(50f, 1f, 0f));
            Enemy(new Vector3(70f, 1f, 0f));
            Enemy(new Vector3(92f, 1f, 0f));

            // --- a turret guarding the staircase ---
            Turret(new Vector3(86f, 5f, 0f));

            // --- spikes to dodge ---
            SpikeRow(new Vector3(56.5f, 0f, 0f), 3); // pit lip near the high platform
            SpikeRow(new Vector3(74f, 0f, 0f), 3);

            // --- checkpoint halfway ---
            CheckpointFlag(new Vector3(50f, 1.2f, 0f));

            // --- goal flag at the end ---
            GoalFlag(new Vector3(96f, 1.5f, 0f));

            // --- death plane below everything ---
            DeathPlane(new Vector3(50f, -14f, 0f), 220f);

            // --- decorative parallax hills in the far background ---
            BuildBackdrop();
        }

        private GameObject Platform(float centerX, float topY, float width)
        {
            float height = 2f;
            var go = new GameObject("Platform");
            go.layer = GroundLayer;
            go.transform.position = new Vector3(centerX, topY - height / 2f, 0f);

            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.SolidSprite(GroundColor);
            sr.drawMode = SpriteDrawMode.Tiled;
            sr.size = new Vector2(width, height);
            sr.sortingOrder = 0;

            var col = go.AddComponent<BoxCollider2D>();
            col.size = new Vector2(width, height);
            col.sharedMaterial = _frictionless;

            // Grass cap for a bit of polish.
            var grass = new GameObject("Grass");
            grass.transform.SetParent(go.transform, false);
            grass.transform.localPosition = new Vector3(0f, height / 2f - 0.15f, 0f);
            var gsr = grass.AddComponent<SpriteRenderer>();
            gsr.sprite = PrimitiveFactory.SolidSprite(GrassColor);
            gsr.drawMode = SpriteDrawMode.Tiled;
            gsr.size = new Vector2(width, 0.3f);
            gsr.sortingOrder = 1;

            return go;
        }

        private void MovingBridge(Vector3 pos, Vector2 travel)
        {
            var go = new GameObject("MovingPlatform");
            go.layer = GroundLayer;
            go.transform.position = pos;

            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.SolidSprite(PlatformColor);
            sr.drawMode = SpriteDrawMode.Tiled;
            sr.size = new Vector2(2.5f, 0.5f);
            sr.sortingOrder = 2;

            var col = go.AddComponent<BoxCollider2D>();
            col.size = new Vector2(2.5f, 0.5f);
            col.sharedMaterial = _frictionless;

            var mp = go.AddComponent<MovingPlatform>();
            mp.waypoints = new[] { Vector2.zero, travel };
            mp.speed = 2.2f;
            mp.waitTime = 0.5f;
        }

        private void CoinArc(Vector2 start, int count, float spacing, bool gem = false)
        {
            for (int i = 0; i < count; i++)
            {
                var pos = new Vector2(start.x + i * spacing, start.y);
                var go = new GameObject(gem ? "Gem" : "Coin");
                go.transform.position = pos;

                var sr = go.AddComponent<SpriteRenderer>();
                sr.sprite = PrimitiveFactory.CircleSprite(gem ? GemColor : CoinColor);
                sr.drawMode = SpriteDrawMode.Sliced;
                sr.size = gem ? new Vector2(0.6f, 0.6f) : new Vector2(0.5f, 0.5f);
                sr.sortingOrder = 5;

                var col = go.AddComponent<CircleCollider2D>();
                col.radius = 0.35f;
                col.isTrigger = true;

                var c = go.AddComponent<Collectible>();
                c.type = gem ? CollectibleType.Gem : CollectibleType.Coin;
                c.value = gem ? 50 : 10;
                c.bob = true;
                if (gem) c.spin = false;
            }
        }

        private void Enemy(Vector3 pos)
        {
            var go = new GameObject("Enemy");
            go.transform.position = pos;

            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.SolidSprite(EnemyColor);
            sr.drawMode = SpriteDrawMode.Sliced;
            sr.size = new Vector2(0.9f, 0.9f);
            sr.sortingOrder = 6;

            // angry eyes
            AddDecal(go.transform, new Vector2(0.2f, 0.1f), new Vector2(0.16f, 0.16f), Color.white, 7);
            AddDecal(go.transform, new Vector2(-0.2f, 0.1f), new Vector2(0.16f, 0.16f), Color.white, 7);

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
            sr.sprite = PrimitiveFactory.SolidSprite(new Color(0.4f, 0.4f, 0.45f));
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
            // An inactive template the turret clones at runtime.
            var go = new GameObject("ProjectileTemplate");
            go.SetActive(false);
            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.CircleSprite(new Color(1f, 0.5f, 0.2f));
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
                sr.sprite = PrimitiveFactory.SpikeSprite(SpikeColor);
                sr.drawMode = SpriteDrawMode.Sliced;
                sr.size = new Vector2(0.7f, 0.7f);
                sr.sortingOrder = 4;

                var col = go.AddComponent<BoxCollider2D>();
                col.size = new Vector2(0.5f, 0.5f);
                col.offset = new Vector2(0f, -0.1f);
                col.isTrigger = true;

                var hz = go.AddComponent<Hazard>();
                hz.instantKill = true;
            }
        }

        private void CheckpointFlag(Vector3 pos)
        {
            var go = new GameObject("Checkpoint");
            go.transform.position = pos;

            // pole
            var pole = new GameObject("Pole");
            pole.transform.SetParent(go.transform, false);
            pole.transform.localPosition = Vector3.zero;
            var psr = pole.AddComponent<SpriteRenderer>();
            psr.sprite = PrimitiveFactory.SolidSprite(new Color(0.8f, 0.8f, 0.8f));
            psr.drawMode = SpriteDrawMode.Sliced;
            psr.size = new Vector2(0.12f, 2.2f);
            psr.sortingOrder = 3;

            // flag
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
            cp.inactiveSprite = PrimitiveFactory.SolidSprite(new Color(0.6f, 0.6f, 0.6f));
            cp.activeSprite = PrimitiveFactory.SolidSprite(CheckpointColor);
            fsr.sprite = cp.inactiveSprite;
        }

        private void GoalFlag(Vector3 pos)
        {
            var go = new GameObject("Goal");
            go.transform.position = pos;

            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.SolidSprite(GoalColor);
            sr.drawMode = SpriteDrawMode.Sliced;
            sr.size = new Vector2(1.2f, 3f);
            sr.sortingOrder = 3;

            // a little star on top
            AddDecal(go.transform, new Vector2(0f, 1.8f), new Vector2(0.6f, 0.6f), Color.white, 4);

            var col = go.AddComponent<BoxCollider2D>();
            col.size = new Vector2(1.2f, 3f);
            col.isTrigger = true;

            go.AddComponent<LevelGoal>();
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

        private void BuildBackdrop()
        {
            // Three layers of soft hills for depth (purely cosmetic).
            Color[] tints =
            {
                new Color(0.30f, 0.52f, 0.75f),
                new Color(0.40f, 0.60f, 0.50f),
                new Color(0.50f, 0.68f, 0.42f),
            };
            for (int layer = 0; layer < tints.Length; layer++)
            {
                for (int i = 0; i < 14; i++)
                {
                    var go = new GameObject("Hill");
                    float x = i * 9f - 5f + layer * 3f;
                    float y = -4f + layer * 1.2f;
                    go.transform.position = new Vector3(x, y, 5f + layer);
                    var sr = go.AddComponent<SpriteRenderer>();
                    sr.sprite = PrimitiveFactory.CircleSprite(tints[layer]);
                    sr.drawMode = SpriteDrawMode.Sliced;
                    float s = 10f - layer * 1.5f;
                    sr.size = new Vector2(s, s);
                    sr.sortingOrder = -10 + layer;
                }
            }
        }
    }
}
