using UnityEngine;

namespace EverySingleDay.Level
{
    /// <summary>
    /// Damaging surface (spikes, lava, saw blade). Hurts the player on contact.
    /// Set <see cref="instantKill"/> for hazards that ignore i-frames and health.
    /// Works with either trigger or solid colliders.
    /// </summary>
    public class Hazard : MonoBehaviour
    {
        public int damage = 1;
        public bool instantKill = false;
        public bool shakeCameraOnHit = true;

        private void OnTriggerEnter2D(Collider2D other) => TryHurt(other.gameObject);
        private void OnTriggerStay2D(Collider2D other) => TryHurt(other.gameObject);
        private void OnCollisionEnter2D(Collision2D c) => TryHurt(c.gameObject);
        private void OnCollisionStay2D(Collision2D c) => TryHurt(c.gameObject);

        private void TryHurt(GameObject go)
        {
            if (!go.CompareTag("Player")) return;
            var health = go.GetComponent<Player.PlayerHealth>();
            if (health == null) return;

            if (instantKill)
                health.Kill();
            else
                health.TakeDamage(damage, transform.position);

            if (shakeCameraOnHit)
                CameraSystem.CameraShake.Trigger();
        }
    }
}
