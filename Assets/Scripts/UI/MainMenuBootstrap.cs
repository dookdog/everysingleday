using UnityEngine;
using UnityEngine.UI;
using UnityEngine.SceneManagement;
using EverySingleDay.Systems;

namespace EverySingleDay.UI
{
    /// <summary>
    /// Builds a complete title / main-menu screen procedurally — animated
    /// parallax-ish backdrop, game title, Play / Quit buttons and the persistent
    /// high score — with zero imported art or authored UI. Mirrors the
    /// zero-asset approach of <see cref="GameBootstrap"/>.
    ///
    /// "Play" loads the gameplay scene by name (falling back to build index +1,
    /// then to spawning a GameBootstrap inline if no gameplay scene exists), so
    /// it works whether or not saved scenes have been generated in the editor.
    /// </summary>
    public class MainMenuBootstrap : MonoBehaviour
    {
        [Header("Scene Flow")]
        [Tooltip("Name of the gameplay scene to load on Play. If it isn't in " +
                 "Build Settings, the menu falls back gracefully.")]
        public string gameplaySceneName = "Main";

        public bool buildOnStart = true;

        private static readonly Color SkyTop = new Color(0.18f, 0.28f, 0.52f);
        private static readonly Color SkyBottom = new Color(0.42f, 0.66f, 0.88f);
        private static readonly Color TitleColor = new Color(1f, 0.85f, 0.3f);
        private static readonly Color AccentColor = new Color(0.30f, 0.90f, 0.45f);

        private Font _font;

        /// <summary>
        /// Auto-spawn so pressing Play on an empty "menu" scene still works. Only
        /// activates when the active scene looks like a menu scene, to avoid
        /// colliding with the gameplay GameBootstrap.
        /// </summary>
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void AutoSpawn()
        {
            var sceneName = SceneManager.GetActiveScene().name;
            if (!sceneName.ToLowerInvariant().Contains("menu")) return;
            if (FindObjectOfType<MainMenuBootstrap>() != null) return;
            if (FindObjectOfType<GameBootstrap>() != null) return;

            var go = new GameObject("MainMenu (auto)");
            go.AddComponent<MainMenuBootstrap>();
        }

        private void Start()
        {
            if (buildOnStart) Build();
        }

        public void Build()
        {
            EnsureAudio();
            EnsureEventSystem();
            BuildCamera();
            BuildBackdrop();
            BuildUI();
        }

        private void EnsureAudio()
        {
            if (AudioManager.Instance == null)
            {
                var go = new GameObject("AudioManager");
                go.AddComponent<AudioManager>();
            }
            // Start menu music (uses the synthesized loop if none assigned).
            AudioManager.PlayMusic(AudioManager.Instance != null && AudioManager.Instance.backgroundMusic != null
                ? AudioManager.Instance.backgroundMusic
                : SfxLibrary.Music);
        }

        private void EnsureEventSystem()
        {
            if (FindObjectOfType<UnityEngine.EventSystems.EventSystem>() == null)
            {
                var es = new GameObject("EventSystem");
                es.AddComponent<UnityEngine.EventSystems.EventSystem>();
                es.AddComponent<UnityEngine.EventSystems.StandaloneInputModule>();
            }
        }

        private void BuildCamera()
        {
            Camera cam = Camera.main;
            if (cam == null)
            {
                var go = new GameObject("Main Camera") { tag = "MainCamera" };
                cam = go.AddComponent<Camera>();
            }
            cam.orthographic = true;
            cam.orthographicSize = 6f;
            cam.clearFlags = CameraClearFlags.SolidColor;
            cam.backgroundColor = SkyBottom;
            cam.transform.position = new Vector3(0f, 0f, -10f);
        }

        private void BuildBackdrop()
        {
            // Soft hills drifting slowly for a touch of life behind the menu.
            Color[] tints =
            {
                new Color(0.30f, 0.45f, 0.68f),
                new Color(0.35f, 0.58f, 0.55f),
                new Color(0.45f, 0.68f, 0.42f),
            };
            var mover = new GameObject("BackdropDrift").AddComponent<MenuBackdropDrift>();
            for (int layer = 0; layer < tints.Length; layer++)
            {
                for (int i = 0; i < 8; i++)
                {
                    var go = new GameObject("Hill");
                    go.transform.SetParent(mover.transform, false);
                    float x = i * 5f - 16f + layer * 2f;
                    float y = -6f + layer * 1.1f;
                    go.transform.position = new Vector3(x, y, 5f + layer);
                    var sr = go.AddComponent<SpriteRenderer>();
                    sr.sprite = PrimitiveFactory.CircleSprite(tints[layer]);
                    sr.drawMode = SpriteDrawMode.Sliced;
                    float s = 8f - layer * 1.2f;
                    sr.size = new Vector2(s, s);
                    sr.sortingOrder = -10 + layer;
                }
            }
        }

        private void BuildUI()
        {
            _font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            if (_font == null) _font = Resources.GetBuiltinResource<Font>("Arial.ttf");

            var canvasGo = new GameObject("Menu Canvas");
            var canvas = canvasGo.AddComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            var scaler = canvasGo.AddComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1920, 1080);
            canvasGo.AddComponent<GraphicRaycaster>();

            // Title (with a drop shadow for depth).
            MakeText(canvas.transform, "TitleShadow", new Vector2(0.5f, 0.5f),
                new Vector2(6, 254), TextAnchor.MiddleCenter, 130,
                new Color(0, 0, 0, 0.4f)).text = "EVERY SINGLE DAY";
            var title = MakeText(canvas.transform, "Title", new Vector2(0.5f, 0.5f),
                new Vector2(0, 260), TextAnchor.MiddleCenter, 130, TitleColor);
            title.text = "EVERY SINGLE DAY";

            var subtitle = MakeText(canvas.transform, "Subtitle", new Vector2(0.5f, 0.5f),
                new Vector2(0, 160), TextAnchor.MiddleCenter, 42, Color.white);
            subtitle.text = "a platformer";

            // High score.
            int high = PlayerPrefs.GetInt("esd_highscore", 0);
            var hs = MakeText(canvas.transform, "HighScore", new Vector2(0.5f, 0.5f),
                new Vector2(0, 70), TextAnchor.MiddleCenter, 34, new Color(1, 1, 1, 0.8f));
            hs.text = high > 0 ? $"BEST  {high:n0}" : "";

            // Buttons.
            MakeButton(canvas.transform, "PLAY", new Vector2(0, -40), AccentColor, OnPlay);
            MakeButton(canvas.transform, "QUIT", new Vector2(0, -160),
                new Color(0.8f, 0.3f, 0.3f), OnQuit);

            // Controls hint.
            var hint = MakeText(canvas.transform, "Hint", new Vector2(0.5f, 0),
                new Vector2(0, 40), TextAnchor.LowerCenter, 26, new Color(1, 1, 1, 0.6f));
            hint.text = "Move: A/D or ←/→     Jump: Space (double-jump!)     " +
                        "Press ENTER to play";

            // Pulse the title a little.
            title.gameObject.AddComponent<UIPulse>();
        }

        private void Update()
        {
            // Keyboard shortcut to start.
            if (Input.GetKeyDown(KeyCode.Return) || Input.GetKeyDown(KeyCode.KeypadEnter))
                OnPlay();
        }

        private void OnPlay()
        {
            AudioManager.Play(SfxLibrary.UiClick);
            Time.timeScale = 1f;

            // 1) Named scene if it's in Build Settings.
            if (!string.IsNullOrEmpty(gameplaySceneName) &&
                Application.CanStreamedLevelBeLoaded(gameplaySceneName))
            {
                SceneManager.LoadScene(gameplaySceneName);
                return;
            }

            // 2) Next build index, if any.
            int next = SceneManager.GetActiveScene().buildIndex + 1;
            if (next < SceneManager.sceneCountInBuildSettings)
            {
                SceneManager.LoadScene(next);
                return;
            }

            // 3) Fallback: build the game inline in a fresh scene so Play always
            //    works, even before any gameplay scene asset has been generated.
            SceneManager.LoadScene(SceneManager.GetActiveScene().buildIndex); // clear menu
            var go = new GameObject("Bootstrap (from menu)");
            DontDestroyOnLoad(go);
            go.AddComponent<DeferredGameBuild>();
        }

        private void OnQuit()
        {
            AudioManager.Play(SfxLibrary.UiClick);
#if UNITY_EDITOR
            UnityEditor.EditorApplication.isPlaying = false;
#else
            Application.Quit();
#endif
        }

        // ----------------------------------------------------------- UI factory
        private Text MakeText(Transform parent, string name, Vector2 anchor,
            Vector2 pos, TextAnchor align, int size, Color color)
        {
            var go = new GameObject(name);
            go.transform.SetParent(parent, false);
            var txt = go.AddComponent<Text>();
            txt.font = _font;
            txt.fontSize = size;
            txt.fontStyle = FontStyle.Bold;
            txt.alignment = align;
            txt.color = color;
            txt.horizontalOverflow = HorizontalWrapMode.Overflow;
            txt.verticalOverflow = VerticalWrapMode.Overflow;

            var rt = txt.rectTransform;
            rt.anchorMin = anchor;
            rt.anchorMax = anchor;
            rt.pivot = new Vector2(0.5f, 0.5f);
            rt.anchoredPosition = pos;
            rt.sizeDelta = new Vector2(1400, 200);
            return txt;
        }

        private Button MakeButton(Transform parent, string label, Vector2 pos,
            Color color, UnityEngine.Events.UnityAction onClick)
        {
            var go = new GameObject($"Button_{label}");
            go.transform.SetParent(parent, false);

            var img = go.AddComponent<Image>();
            img.color = color;
            var rt = img.rectTransform;
            rt.anchorMin = new Vector2(0.5f, 0.5f);
            rt.anchorMax = new Vector2(0.5f, 0.5f);
            rt.pivot = new Vector2(0.5f, 0.5f);
            rt.anchoredPosition = pos;
            rt.sizeDelta = new Vector2(380, 96);

            var btn = go.AddComponent<Button>();
            var colors = btn.colors;
            colors.highlightedColor = Color.Lerp(color, Color.white, 0.25f);
            colors.pressedColor = Color.Lerp(color, Color.black, 0.2f);
            colors.fadeDuration = 0.08f;
            btn.colors = colors;
            btn.onClick.AddListener(onClick);

            var txt = MakeText(go.transform, "Label", new Vector2(0.5f, 0.5f),
                Vector2.zero, TextAnchor.MiddleCenter, 48, Color.white);
            txt.text = label;
            txt.rectTransform.sizeDelta = rt.sizeDelta;

            return btn;
        }
    }

    /// <summary>Slowly drifts the menu backdrop for subtle motion.</summary>
    public class MenuBackdropDrift : MonoBehaviour
    {
        public float speed = 0.25f;
        private void Update() => transform.position += Vector3.left * speed * Time.deltaTime;
    }

    /// <summary>Gentle scale pulse for the menu title.</summary>
    public class UIPulse : MonoBehaviour
    {
        public float amount = 0.04f;
        public float speed = 2f;
        private Vector3 _base;
        private void Start() => _base = transform.localScale;
        private void Update()
        {
            float s = 1f + Mathf.Sin(Time.time * speed) * amount;
            transform.localScale = _base * s;
        }
    }

    /// <summary>
    /// Survives the scene reload triggered by the menu's inline fallback, then
    /// spawns a GameBootstrap once and removes itself. Only used when no
    /// gameplay scene asset exists in Build Settings.
    /// </summary>
    public class DeferredGameBuild : MonoBehaviour
    {
        private void OnEnable() => SceneManager.sceneLoaded += OnLoaded;
        private void OnDisable() => SceneManager.sceneLoaded -= OnLoaded;

        private void OnLoaded(Scene scene, LoadSceneMode mode)
        {
            if (FindObjectOfType<GameBootstrap>() == null)
            {
                var go = new GameObject("Bootstrap");
                go.AddComponent<GameBootstrap>();
            }
            Destroy(gameObject);
        }
    }
}
