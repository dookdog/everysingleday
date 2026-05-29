using UnityEngine;

namespace EverySingleDay.Level
{
    /// <summary>
    /// Activates when the player passes through, updating their respawn point.
    /// Swaps an optional sprite and plays a sound the first time it's reached.
    /// </summary>
    [RequireComponent(typeof(Collider2D))]
    public class Checkpoint : MonoBehaviour
    {
        [Header("Visuals")]
        public SpriteRenderer flagRenderer;
        public Sprite inactiveSprite;
        public Sprite activeSprite;

        [Header("Feedback")]
        public AudioClip activateClip;
        public GameObject activateEffect;

        public bool IsActivated { get; private set; }

        private void Awake()
        {
            GetComponent<Collider2D>().isTrigger = true;
            if (flagRenderer != null && inactiveSprite != null)
                flagRenderer.sprite = inactiveSprite;
        }

        private void OnTriggerEnter2D(Collider2D other)
        {
            if (IsActivated || !other.CompareTag("Player")) return;

            var health = other.GetComponent<Player.PlayerHealth>();
            if (health == null) return;

            health.SetRespawnPoint(transform.position);
            Activate();
        }

        private void Activate()
        {
            IsActivated = true;
            if (flagRenderer != null && activeSprite != null)
                flagRenderer.sprite = activeSprite;
            Systems.AudioManager.Play(activateClip);
            if (activateEffect != null)
                Instantiate(activateEffect, transform.position, Quaternion.identity);
        }
    }
}
