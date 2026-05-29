using UnityEngine;
using UnityEngine.UI;

namespace EverySingleDay.UI
{
    /// <summary>
    /// Legacy-UI HUD built entirely from code by the bootstrap. Uses
    /// UnityEngine.UI.Text (no TextMeshPro essentials import required), so it
    /// always renders. Shows score, coins, lives, health pips and overlay
    /// panels for pause / win / game over, all driven by game events.
    /// </summary>
    public class RuntimeHUD : MonoBehaviour
    {
        private Text _scoreText;
        private Text _coinsText;
        private Text _livesText;
        private Text _healthText;
        private Text _objectiveText;
        private Text _centerBanner;
        private GameObject _bannerPanel;
        private Text _bannerSub;

        private Systems.GameManager _gm;
        private Player.PlayerHealth _health;
        private Font _font;

        public static RuntimeHUD Create()
        {
            var canvasGo = new GameObject("HUD Canvas");
            var canvas = canvasGo.AddComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            var scaler = canvasGo.AddComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1920, 1080);
            canvasGo.AddComponent<GraphicRaycaster>();

            var hud = canvasGo.AddComponent<RuntimeHUD>();
            hud.Build(canvas);
            return hud;
        }

        private void Build(Canvas canvas)
        {
            _font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            if (_font == null) _font = Resources.GetBuiltinResource<Font>("Arial.ttf");

            _scoreText = MakeText(canvas.transform, "Score", new Vector2(0, 1),
                new Vector2(40, -40), TextAnchor.UpperLeft, 40);
            _coinsText = MakeText(canvas.transform, "Coins", new Vector2(0, 1),
                new Vector2(40, -100), TextAnchor.UpperLeft, 36);
            _coinsText.color = new Color(1f, 0.85f, 0.2f);
            _livesText = MakeText(canvas.transform, "Lives", new Vector2(0, 1),
                new Vector2(40, -150), TextAnchor.UpperLeft, 36);
            _healthText = MakeText(canvas.transform, "Health", new Vector2(1, 1),
                new Vector2(-40, -40), TextAnchor.UpperRight, 40);
            _healthText.color = new Color(1f, 0.4f, 0.4f);

            // Objective progress, centred at the top.
            _objectiveText = MakeText(canvas.transform, "Objective", new Vector2(0.5f, 1),
                new Vector2(0, -44), TextAnchor.UpperCenter, 38);
            _objectiveText.color = new Color(0.85f, 0.95f, 1f);

            var hint = MakeText(canvas.transform, "Hint", new Vector2(0.5f, 0),
                new Vector2(0, 30), TextAnchor.LowerCenter, 24);
            hint.text = "Move: A/D or ←/→   Jump: Space (double-jump!)   Pause: Esc";
            hint.color = new Color(1f, 1f, 1f, 0.6f);

            BuildBanner(canvas);
        }

        private void BuildBanner(Canvas canvas)
        {
            _bannerPanel = new GameObject("Banner");
            _bannerPanel.transform.SetParent(canvas.transform, false);
            var img = _bannerPanel.AddComponent<Image>();
            img.color = new Color(0f, 0f, 0f, 0.65f);
            var rt = img.rectTransform;
            rt.anchorMin = Vector2.zero; rt.anchorMax = Vector2.one;
            rt.offsetMin = Vector2.zero; rt.offsetMax = Vector2.zero;

            _centerBanner = MakeText(_bannerPanel.transform, "BannerTitle",
                new Vector2(0.5f, 0.5f), new Vector2(0, 60), TextAnchor.MiddleCenter, 90);
            _bannerSub = MakeText(_bannerPanel.transform, "BannerSub",
                new Vector2(0.5f, 0.5f), new Vector2(0, -60), TextAnchor.MiddleCenter, 36);
            _bannerSub.text = "Press R to play again";

            _bannerPanel.SetActive(false);
        }

        private Text MakeText(Transform parent, string name, Vector2 anchor,
            Vector2 pos, TextAnchor align, int size)
        {
            var go = new GameObject(name);
            go.transform.SetParent(parent, false);
            var txt = go.AddComponent<Text>();
            txt.font = _font;
            txt.fontSize = size;
            txt.alignment = align;
            txt.color = Color.white;
            txt.horizontalOverflow = HorizontalWrapMode.Overflow;
            txt.verticalOverflow = VerticalWrapMode.Overflow;

            var rt = txt.rectTransform;
            rt.anchorMin = anchor;
            rt.anchorMax = anchor;
            rt.pivot = anchor;
            rt.anchoredPosition = pos;
            rt.sizeDelta = new Vector2(900, 80);
            return txt;
        }

        private void Start()
        {
            _gm = Systems.GameManager.Instance;
            _health = FindObjectOfType<Player.PlayerHealth>();

            if (_gm != null)
            {
                _gm.OnScoreChanged += UpdateScore;
                _gm.OnCoinsChanged += UpdateCoins;
                _gm.OnLivesChanged += UpdateLives;
                _gm.OnStateChanged += UpdateBanner;
                UpdateScore(_gm.Score);
                UpdateCoins(_gm.Coins);
                UpdateLives(_gm.Lives);
            }
            if (_health != null)
            {
                _health.OnHealthChanged += UpdateHealth;
                UpdateHealth(_health.CurrentHealth, _health.maxHealth);
            }

            var om = Systems.ObjectiveManager.Instance;
            if (om != null)
            {
                om.OnObjectiveChanged += UpdateObjective;
                UpdateObjective();
            }
        }

        private void OnDestroy()
        {
            if (_gm != null)
            {
                _gm.OnScoreChanged -= UpdateScore;
                _gm.OnCoinsChanged -= UpdateCoins;
                _gm.OnLivesChanged -= UpdateLives;
                _gm.OnStateChanged -= UpdateBanner;
            }
            if (_health != null)
                _health.OnHealthChanged -= UpdateHealth;
            var om = Systems.ObjectiveManager.Instance;
            if (om != null)
                om.OnObjectiveChanged -= UpdateObjective;
        }

        private void UpdateObjective()
        {
            if (_objectiveText == null) return;
            var om = Systems.ObjectiveManager.Instance;
            _objectiveText.text = om != null ? om.ProgressText() : "";
        }

        private void Update()
        {
            // Lightweight restart/continue handling for the bootstrap build.
            if (_gm == null) return;

            bool ended = _gm.State == Systems.GameState.GameOver ||
                         _gm.State == Systems.GameState.LevelComplete;

            if (_gm.State == Systems.GameState.GameOver && Input.GetKeyDown(KeyCode.R))
                _gm.ReloadLevel();
            if (_gm.State == Systems.GameState.LevelComplete && Input.GetKeyDown(KeyCode.R))
                _gm.LoadNextLevel();

            // Return to the title screen from any end state.
            if (ended && Input.GetKeyDown(KeyCode.M))
                _gm.ReturnToMenu();
        }

        private void UpdateScore(int v) => _scoreText.text = $"SCORE  {v:n0}";
        private void UpdateCoins(int v) => _coinsText.text = $"COINS  {v}";
        private void UpdateLives(int v) => _livesText.text = $"LIVES  {v}";

        private void UpdateHealth(int current, int max)
        {
            string hearts = "";
            for (int i = 0; i < max; i++) hearts += i < current ? "♥ " : "♡ ";
            _healthText.text = hearts.TrimEnd();
        }

        private void UpdateBanner(Systems.GameState state)
        {
            switch (state)
            {
                case Systems.GameState.LevelComplete:
                    Show("LEVEL COMPLETE!", "R: next challenge      M: main menu");
                    break;
                case Systems.GameState.GameOver:
                    Show("GAME OVER", "R: try again      M: main menu");
                    break;
                case Systems.GameState.Paused:
                    Show("PAUSED", "Press Esc to resume");
                    break;
                default:
                    _bannerPanel.SetActive(false);
                    break;
            }
        }

        private void Show(string title, string sub)
        {
            _centerBanner.text = title;
            _bannerSub.text = sub;
            _bannerPanel.SetActive(true);
        }
    }
}
