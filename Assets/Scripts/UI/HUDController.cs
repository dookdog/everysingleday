using UnityEngine;
using UnityEngine.UI;
using TMPro;

namespace EverySingleDay.UI
{
    /// <summary>
    /// In-game heads-up display. Subscribes to GameManager and PlayerHealth
    /// events and updates score / coins / lives text plus a row of heart icons.
    /// All references are optional so the HUD can be built up incrementally.
    /// </summary>
    public class HUDController : MonoBehaviour
    {
        [Header("Text")]
        public TMP_Text scoreText;
        public TMP_Text coinsText;
        public TMP_Text livesText;
        public TMP_Text highScoreText;

        [Header("Health Display")]
        public Transform heartsContainer;
        public Image heartPrefab;
        public Sprite fullHeart;
        public Sprite emptyHeart;

        private Image[] _hearts;
        private Player.PlayerHealth _playerHealth;

        private void Start()
        {
            var gm = Systems.GameManager.Instance;
            if (gm != null)
            {
                gm.OnScoreChanged += UpdateScore;
                gm.OnCoinsChanged += UpdateCoins;
                gm.OnLivesChanged += UpdateLives;

                UpdateScore(gm.Score);
                UpdateCoins(gm.Coins);
                UpdateLives(gm.Lives);
                if (highScoreText != null)
                    highScoreText.text = $"BEST {gm.HighScore:n0}";
            }

            _playerHealth = FindObjectOfType<Player.PlayerHealth>();
            if (_playerHealth != null)
            {
                BuildHearts(_playerHealth.maxHealth);
                _playerHealth.OnHealthChanged += UpdateHealth;
                UpdateHealth(_playerHealth.CurrentHealth, _playerHealth.maxHealth);
            }
        }

        private void OnDestroy()
        {
            var gm = Systems.GameManager.Instance;
            if (gm != null)
            {
                gm.OnScoreChanged -= UpdateScore;
                gm.OnCoinsChanged -= UpdateCoins;
                gm.OnLivesChanged -= UpdateLives;
            }
            if (_playerHealth != null)
                _playerHealth.OnHealthChanged -= UpdateHealth;
        }

        private void UpdateScore(int v) { if (scoreText) scoreText.text = $"SCORE {v:n0}"; }
        private void UpdateCoins(int v) { if (coinsText) coinsText.text = $"x {v}"; }
        private void UpdateLives(int v) { if (livesText) livesText.text = $"x {v}"; }

        private void BuildHearts(int max)
        {
            if (heartsContainer == null || heartPrefab == null) return;
            _hearts = new Image[max];
            for (int i = 0; i < max; i++)
                _hearts[i] = Instantiate(heartPrefab, heartsContainer);
        }

        private void UpdateHealth(int current, int max)
        {
            if (_hearts == null) return;
            for (int i = 0; i < _hearts.Length; i++)
            {
                if (_hearts[i] == null) continue;
                bool filled = i < current;
                if (fullHeart != null && emptyHeart != null)
                    _hearts[i].sprite = filled ? fullHeart : emptyHeart;
                else
                    _hearts[i].color = filled ? Color.red : new Color(0.3f, 0.1f, 0.1f);
            }
        }
    }
}
