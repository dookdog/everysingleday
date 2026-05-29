using System.Collections;
using UnityEngine;

namespace EverySingleDay.Player
{
    /// <summary>
    /// Tracks player hit points, handles damage, invulnerability frames,
    /// knockback, death and respawn at the last checkpoint.
    /// </summary>
    [RequireComponent(typeof(PlayerController))]
    public class PlayerHealth : MonoBehaviour
    {
        [Header("Health")]
        public int maxHealth = 3;
        public float invulnerabilityTime = 1.2f;
        public Vector2 knockbackForce = new Vector2(8f, 10f);

        [Header("Feedback")]
        public SpriteRenderer spriteToFlash;
        public float flashInterval = 0.08f;
        public AudioClip hurtClip;
        public AudioClip deathClip;

        public int CurrentHealth { get; private set; }
        public bool IsInvulnerable { get; private set; }
        public bool IsDead { get; private set; }

        public System.Action<int, int> OnHealthChanged; // (current, max)
        public System.Action OnDamaged;
        public System.Action OnDied;

        private PlayerController _controller;
        private Vector3 _respawnPoint;

        private void Awake()
        {
            _controller = GetComponent<PlayerController>();
            CurrentHealth = maxHealth;
            _respawnPoint = transform.position;
        }

        private void Start()
        {
            OnHealthChanged?.Invoke(CurrentHealth, maxHealth);
        }

        public void SetRespawnPoint(Vector3 point) => _respawnPoint = point;

        /// <summary>
        /// Deal damage to the player. <paramref name="sourcePosition"/> determines
        /// the knockback direction. Pass instantKill to bypass i-frames (pits, spikes).
        /// </summary>
        public void TakeDamage(int amount, Vector3 sourcePosition, bool instantKill = false)
        {
            if (IsDead) return;
            if (IsInvulnerable && !instantKill) return;

            CurrentHealth = Mathf.Max(0, CurrentHealth - amount);
            OnHealthChanged?.Invoke(CurrentHealth, maxHealth);
            OnDamaged?.Invoke();

            if (CurrentHealth <= 0)
            {
                Die();
                return;
            }

            Systems.AudioManager.Play(hurtClip != null ? hurtClip : Systems.SfxLibrary.Hurt);
            CameraSystem.CameraShake.Trigger(0.18f, 0.25f);

            // Knockback away from the damage source.
            float dir = Mathf.Sign(transform.position.x - sourcePosition.x);
            if (dir == 0) dir = 1;
            _controller.ApplyKnockback(new Vector2(knockbackForce.x * dir, knockbackForce.y));

            StartCoroutine(InvulnerabilityRoutine());
        }

        /// <summary>Apply environmental kill (falling off the map, deadly hazard).</summary>
        public void Kill()
        {
            if (IsDead) return;
            CurrentHealth = 0;
            OnHealthChanged?.Invoke(CurrentHealth, maxHealth);
            Die();
        }

        public void Heal(int amount)
        {
            if (IsDead) return;
            CurrentHealth = Mathf.Min(maxHealth, CurrentHealth + amount);
            OnHealthChanged?.Invoke(CurrentHealth, maxHealth);
        }

        private void Die()
        {
            IsDead = true;
            Systems.AudioManager.Play(deathClip != null ? deathClip : Systems.SfxLibrary.Death);
            OnDied?.Invoke();
            _controller.SetControlsLocked(true);

            var gm = Systems.GameManager.Instance;
            if (gm != null)
                gm.OnPlayerDied(this);
            else
                StartCoroutine(RespawnRoutine(1.2f));
        }

        public void Respawn()
        {
            StopAllCoroutines();
            transform.position = _respawnPoint;
            CurrentHealth = maxHealth;
            IsDead = false;
            IsInvulnerable = false;
            if (spriteToFlash != null)
            {
                var c = spriteToFlash.color;
                c.a = 1f;
                spriteToFlash.color = c;
            }
            _controller.SetControlsLocked(false);
            OnHealthChanged?.Invoke(CurrentHealth, maxHealth);
        }

        private IEnumerator RespawnRoutine(float delay)
        {
            yield return new WaitForSeconds(delay);
            Respawn();
        }

        private IEnumerator InvulnerabilityRoutine()
        {
            IsInvulnerable = true;
            float elapsed = 0f;
            bool visible = true;

            while (elapsed < invulnerabilityTime)
            {
                if (spriteToFlash != null)
                {
                    visible = !visible;
                    var c = spriteToFlash.color;
                    c.a = visible ? 1f : 0.3f;
                    spriteToFlash.color = c;
                }
                yield return new WaitForSeconds(flashInterval);
                elapsed += flashInterval;
            }

            if (spriteToFlash != null)
            {
                var c = spriteToFlash.color;
                c.a = 1f;
                spriteToFlash.color = c;
            }
            IsInvulnerable = false;
        }
    }
}
