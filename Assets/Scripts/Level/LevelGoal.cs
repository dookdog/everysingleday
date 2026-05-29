using UnityEngine;

namespace EverySingleDay.Level
{
    /// <summary>
    /// The end-of-level flag / portal. Completing it depends on the active
    /// objective: when an <see cref="Systems.ObjectiveManager"/> exists, the goal
    /// only finishes the level once the objective is satisfied ("armed"). With no
    /// objective manager it behaves as a simple reach-the-exit goal.
    /// </summary>
    [RequireComponent(typeof(Collider2D))]
    public class LevelGoal : MonoBehaviour
    {
        public AudioClip goalClip;
        public GameObject goalEffect;

        [Tooltip("Optional renderer dimmed while the goal is locked.")]
        public SpriteRenderer bodyRenderer;
        public Color lockedTint = new Color(0.5f, 0.5f, 0.5f, 0.6f);

        private bool _reached;
        private Color _armedColor;
        private bool _wasArmed;

        private void Awake()
        {
            GetComponent<Collider2D>().isTrigger = true;
            if (bodyRenderer != null) _armedColor = bodyRenderer.color;
        }

        private void Update()
        {
            // Reflect armed/locked state visually (pulse when open, dim when not).
            if (bodyRenderer == null) return;
            bool armed = Systems.ObjectiveManager.Instance == null ||
                         Systems.ObjectiveManager.Instance.GoalArmed;

            if (armed)
            {
                float pulse = 0.75f + Mathf.PingPong(Time.time * 1.5f, 0.25f);
                bodyRenderer.color = _armedColor * pulse;
            }
            else
            {
                bodyRenderer.color = lockedTint;
            }

            if (armed && !_wasArmed)
                _wasArmed = true;
        }

        private void OnTriggerEnter2D(Collider2D other)
        {
            if (_reached || !other.CompareTag("Player")) return;

            var om = Systems.ObjectiveManager.Instance;
            if (om != null && !om.GoalArmed)
            {
                // Not yet allowed to finish — nudge the player about what's left.
                Systems.AudioManager.Play(Systems.SfxLibrary.Hurt, 0.4f);
                return;
            }

            _reached = true;
            other.GetComponent<Player.PlayerController>()?.SetControlsLocked(true);
            Systems.AudioManager.Play(goalClip != null ? goalClip : Systems.SfxLibrary.Goal);
            if (goalEffect != null)
                Instantiate(goalEffect, transform.position, Quaternion.identity);

            if (om != null) om.TryCompleteAtGoal();
            else Systems.GameManager.Instance?.CompleteLevel();
        }
    }
}
