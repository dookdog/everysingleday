using UnityEngine;

namespace EverySingleDay.Level
{
    /// <summary>
    /// The end-of-level flag / portal. Reaching it locks player input, plays a
    /// celebration and tells the GameManager the level is complete.
    /// </summary>
    [RequireComponent(typeof(Collider2D))]
    public class LevelGoal : MonoBehaviour
    {
        public AudioClip goalClip;
        public GameObject goalEffect;

        private bool _reached;

        private void Awake() => GetComponent<Collider2D>().isTrigger = true;

        private void OnTriggerEnter2D(Collider2D other)
        {
            if (_reached || !other.CompareTag("Player")) return;
            _reached = true;

            other.GetComponent<Player.PlayerController>()?.SetControlsLocked(true);
            Systems.AudioManager.Play(goalClip != null ? goalClip : Systems.SfxLibrary.Goal);
            if (goalEffect != null)
                Instantiate(goalEffect, transform.position, Quaternion.identity);

            Systems.GameManager.Instance?.CompleteLevel();
        }
    }
}
