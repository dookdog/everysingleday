using System.Collections;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace EverySingleDay.Systems
{
    public enum GameState { Playing, Paused, LevelComplete, GameOver }

    /// <summary>
    /// Central run-state owner: score, coins, lives, level progression, pause,
    /// win/lose flow and high-score persistence. Implemented as a lightweight
    /// scene singleton so gameplay objects (enemies, pickups, hazards) can talk
    /// to it via <see cref="Instance"/> without hard references.
    /// </summary>
    public class GameManager : MonoBehaviour
    {
        public static GameManager Instance { get; private set; }

        [Header("Run Settings")]
        public int startingLives = 3;
        [Tooltip("Optional explicit next scene. If empty, loads the next build index.")]
        public string nextLevelScene = "";
        public float respawnDelay = 1.1f;

        [Header("State (read-only at runtime)")]
        public GameState State = GameState.Playing;

        public int Score { get; private set; }
        public int Coins { get; private set; }
        public int Lives { get; private set; }
        public int HighScore { get; private set; }

        // UI / systems subscribe to these.
        public System.Action<int> OnScoreChanged;
        public System.Action<int> OnCoinsChanged;
        public System.Action<int> OnLivesChanged;
        public System.Action<GameState> OnStateChanged;

        private const string HighScoreKey = "esd_highscore";

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;

            Lives = startingLives;
            HighScore = PlayerPrefs.GetInt(HighScoreKey, 0);
        }

        private void Start()
        {
            SetState(GameState.Playing);
            OnScoreChanged?.Invoke(Score);
            OnCoinsChanged?.Invoke(Coins);
            OnLivesChanged?.Invoke(Lives);
        }

        private void OnDestroy()
        {
            if (Instance == this) Instance = null;
        }

        private void Update()
        {
            if (Input.GetButtonDown("Cancel"))
                TogglePause();
        }

        // --- score / economy ---
        public void AddScore(int amount)
        {
            Score += amount;
            OnScoreChanged?.Invoke(Score);
            TrySaveHighScore();
        }

        public void AddCoin(int amount = 1)
        {
            Coins += amount;
            OnCoinsChanged?.Invoke(Coins);
            // Every 100 coins grants a life — a genre staple.
            if (Coins >= 100)
            {
                Coins -= 100;
                AddLife();
                OnCoinsChanged?.Invoke(Coins);
            }
        }

        public void AddLife()
        {
            Lives++;
            OnLivesChanged?.Invoke(Lives);
        }

        private void TrySaveHighScore()
        {
            if (Score > HighScore)
            {
                HighScore = Score;
                PlayerPrefs.SetInt(HighScoreKey, HighScore);
                PlayerPrefs.Save();
            }
        }

        // --- player lifecycle ---
        public void OnPlayerDied(Player.PlayerHealth player)
        {
            Lives--;
            OnLivesChanged?.Invoke(Lives);

            if (Lives <= 0)
                StartCoroutine(GameOverRoutine());
            else
                StartCoroutine(RespawnRoutine(player));
        }

        private IEnumerator RespawnRoutine(Player.PlayerHealth player)
        {
            yield return new WaitForSeconds(respawnDelay);
            if (player != null)
                player.Respawn();
        }

        private IEnumerator GameOverRoutine()
        {
            yield return new WaitForSeconds(respawnDelay);
            SetState(GameState.GameOver);
        }

        // --- level flow ---
        public void CompleteLevel()
        {
            if (State == GameState.LevelComplete) return;
            // Bonus for remaining lives.
            AddScore(Lives * 250);
            SetState(GameState.LevelComplete);
        }

        public void LoadNextLevel()
        {
            Time.timeScale = 1f;
            if (!string.IsNullOrEmpty(nextLevelScene))
            {
                SceneManager.LoadScene(nextLevelScene);
                return;
            }

            int next = SceneManager.GetActiveScene().buildIndex + 1;
            if (next < SceneManager.sceneCountInBuildSettings)
                SceneManager.LoadScene(next);
            else
                ReloadLevel(); // loop back if no further level exists
        }

        public void ReloadLevel()
        {
            Time.timeScale = 1f;
            SceneManager.LoadScene(SceneManager.GetActiveScene().buildIndex);
        }

        public void RestartGame()
        {
            Time.timeScale = 1f;
            SceneManager.LoadScene(0);
        }

        // --- pause ---
        public void TogglePause()
        {
            if (State == GameState.Playing) Pause();
            else if (State == GameState.Paused) Resume();
        }

        public void Pause()
        {
            if (State != GameState.Playing) return;
            Time.timeScale = 0f;
            SetState(GameState.Paused);
        }

        public void Resume()
        {
            if (State != GameState.Paused) return;
            Time.timeScale = 1f;
            SetState(GameState.Playing);
        }

        private void SetState(GameState state)
        {
            State = state;
            OnStateChanged?.Invoke(state);
        }
    }
}
