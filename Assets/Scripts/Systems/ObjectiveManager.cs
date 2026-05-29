using UnityEngine;

namespace EverySingleDay.Systems
{
    /// <summary>
    /// Tracks the active level's objective and decides when the goal becomes
    /// "armed" (touching it completes the level). For ReachGoal the goal is
    /// always armed; for the others the player must first collect all/enough
    /// gems, defeat all enemies, or survive a timer.
    ///
    /// Gameplay objects report progress through the static counters here, so
    /// they stay decoupled (a collectible doesn't need to know the objective).
    /// </summary>
    public class ObjectiveManager : MonoBehaviour
    {
        public static ObjectiveManager Instance { get; private set; }

        public ObjectiveType Objective { get; private set; }
        public bool GoalArmed { get; private set; }

        public int GemsCollected { get; private set; }
        public int GemsRequired { get; private set; }
        public int EnemiesDefeated { get; private set; }
        public int EnemiesRequired { get; private set; }
        public float SurviveRemaining { get; private set; }

        public string Description { get; private set; } = "Reach the exit";
        public System.Action OnObjectiveChanged;

        private bool _surviveActive;

        private void Awake()
        {
            Instance = this;
        }

        private void OnDestroy()
        {
            if (Instance == this) Instance = null;
        }

        public void Configure(LevelData data)
        {
            Objective = data.objective;
            GemsCollected = 0;
            EnemiesDefeated = 0;
            GemsRequired = 0;
            EnemiesRequired = 0;
            _surviveActive = false;
            GoalArmed = false;

            switch (Objective)
            {
                case ObjectiveType.ReachGoal:
                    GoalArmed = true;
                    Description = "Reach the exit";
                    break;
                case ObjectiveType.CollectAllGems:
                    GemsRequired = data.GemCount;
                    Description = $"Collect all {GemsRequired} gems, then exit";
                    break;
                case ObjectiveType.CollectQuota:
                    GemsRequired = data.quota;
                    Description = $"Collect {GemsRequired} gems, then exit";
                    break;
                case ObjectiveType.DefeatAllEnemies:
                    EnemiesRequired = data.enemies.Count;
                    Description = $"Defeat all {EnemiesRequired} enemies, then exit";
                    break;
                case ObjectiveType.Survive:
                    SurviveRemaining = data.surviveTime;
                    _surviveActive = true;
                    Description = $"Survive {Mathf.RoundToInt(data.surviveTime)}s, then exit";
                    break;
            }

            OnObjectiveChanged?.Invoke();
        }

        private void Update()
        {
            if (!_surviveActive) return;
            if (GameManager.Instance != null && GameManager.Instance.State != GameState.Playing) return;

            SurviveRemaining -= Time.deltaTime;
            if (SurviveRemaining <= 0f)
            {
                SurviveRemaining = 0f;
                _surviveActive = false;
                ArmGoal();
            }
            OnObjectiveChanged?.Invoke();
        }

        // --- progress reports from gameplay objects ---
        public void ReportGemCollected()
        {
            GemsCollected++;
            if ((Objective == ObjectiveType.CollectAllGems || Objective == ObjectiveType.CollectQuota)
                && GemsCollected >= GemsRequired)
                ArmGoal();
            OnObjectiveChanged?.Invoke();
        }

        public void ReportEnemyDefeated()
        {
            EnemiesDefeated++;
            if (Objective == ObjectiveType.DefeatAllEnemies && EnemiesDefeated >= EnemiesRequired)
                ArmGoal();
            OnObjectiveChanged?.Invoke();
        }

        private void ArmGoal()
        {
            if (GoalArmed) return;
            GoalArmed = true;
            Description = "Reach the exit!";
            AudioManager.Play(SfxLibrary.Checkpoint);
            OnObjectiveChanged?.Invoke();
        }

        /// <summary>Called by the goal trigger; completes the level only if armed.</summary>
        public bool TryCompleteAtGoal()
        {
            if (!GoalArmed) return false;
            GameManager.Instance?.CompleteLevel();
            return true;
        }

        /// <summary>Short HUD string of current progress, or empty when armed.</summary>
        public string ProgressText()
        {
            if (GoalArmed && Objective != ObjectiveType.ReachGoal) return "EXIT OPEN!";
            switch (Objective)
            {
                case ObjectiveType.CollectAllGems:
                case ObjectiveType.CollectQuota:
                    return $"GEMS  {GemsCollected}/{GemsRequired}";
                case ObjectiveType.DefeatAllEnemies:
                    return $"FOES  {EnemiesDefeated}/{EnemiesRequired}";
                case ObjectiveType.Survive:
                    return $"SURVIVE  {Mathf.CeilToInt(SurviveRemaining)}s";
                default:
                    return "";
            }
        }
    }
}
