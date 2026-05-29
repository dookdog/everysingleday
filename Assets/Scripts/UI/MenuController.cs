using UnityEngine;
using TMPro;

namespace EverySingleDay.UI
{
    /// <summary>
    /// Drives the overlay panels (pause / level complete / game over) by
    /// reacting to GameManager state changes, and provides button handler
    /// methods to wire up in the inspector.
    /// </summary>
    public class MenuController : MonoBehaviour
    {
        [Header("Panels")]
        public GameObject pausePanel;
        public GameObject levelCompletePanel;
        public GameObject gameOverPanel;

        [Header("Result Text")]
        public TMP_Text levelCompleteScoreText;
        public TMP_Text gameOverScoreText;

        private Systems.GameManager _gm;

        private void Start()
        {
            _gm = Systems.GameManager.Instance;
            HideAll();
            if (_gm != null)
                _gm.OnStateChanged += HandleStateChanged;
        }

        private void OnDestroy()
        {
            if (_gm != null)
                _gm.OnStateChanged -= HandleStateChanged;
        }

        private void HandleStateChanged(Systems.GameState state)
        {
            HideAll();
            switch (state)
            {
                case Systems.GameState.Paused:
                    if (pausePanel) pausePanel.SetActive(true);
                    break;
                case Systems.GameState.LevelComplete:
                    if (levelCompletePanel) levelCompletePanel.SetActive(true);
                    if (levelCompleteScoreText && _gm != null)
                        levelCompleteScoreText.text = $"Score: {_gm.Score:n0}";
                    break;
                case Systems.GameState.GameOver:
                    if (gameOverPanel) gameOverPanel.SetActive(true);
                    if (gameOverScoreText && _gm != null)
                        gameOverScoreText.text = $"Score: {_gm.Score:n0}\nBest: {_gm.HighScore:n0}";
                    break;
            }
        }

        private void HideAll()
        {
            if (pausePanel) pausePanel.SetActive(false);
            if (levelCompletePanel) levelCompletePanel.SetActive(false);
            if (gameOverPanel) gameOverPanel.SetActive(false);
        }

        // --- Button handlers (wire these to UI Buttons) ---
        public void OnResume() { Click(); _gm?.Resume(); }
        public void OnRestart() { Click(); _gm?.ReloadLevel(); }
        public void OnNextLevel() { Click(); _gm?.LoadNextLevel(); }
        public void OnMainMenu() { Click(); _gm?.ReturnToMenu(); }

        private static void Click() => Systems.AudioManager.Play(Systems.SfxLibrary.UiClick);

        public void OnQuit()
        {
            Click();
#if UNITY_EDITOR
            UnityEditor.EditorApplication.isPlaying = false;
#else
            Application.Quit();
#endif
        }
    }
}
