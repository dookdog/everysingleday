using UnityEngine;

namespace EverySingleDay.Enemies
{
    /// <summary>
    /// Stationary turret that fires projectiles at a fixed interval when the
    /// player is within range and line of sight.
    /// </summary>
    public class TurretEnemy : MonoBehaviour
    {
        public Projectile projectilePrefab;
        public Transform firePoint;
        public float fireInterval = 1.8f;
        public float detectionRange = 10f;
        public LayerMask sightBlockers;
        public AudioClip fireClip;

        private float _timer;
        private Transform _player;

        private void Start()
        {
            var pc = FindObjectOfType<Player.PlayerController>();
            if (pc != null) _player = pc.transform;
            if (firePoint == null) firePoint = transform;
        }

        private void Update()
        {
            if (_player == null || projectilePrefab == null) return;

            float dist = Vector2.Distance(transform.position, _player.position);
            if (dist > detectionRange) return;

            // Optional line-of-sight check.
            if (sightBlockers.value != 0)
            {
                Vector2 dirToPlayer = (_player.position - firePoint.position);
                var hit = Physics2D.Raycast(firePoint.position, dirToPlayer.normalized,
                    dirToPlayer.magnitude, sightBlockers);
                if (hit.collider != null) return;
            }

            _timer -= Time.deltaTime;
            if (_timer <= 0f)
            {
                Fire();
                _timer = fireInterval;
            }
        }

        private void Fire()
        {
            Vector2 dir = (_player.position - firePoint.position).normalized;
            var proj = Instantiate(projectilePrefab, firePoint.position, Quaternion.identity);
            proj.Launch(dir);
            Systems.AudioManager.Play(fireClip);
        }

        private void OnDrawGizmosSelected()
        {
            Gizmos.color = new Color(1f, 0.5f, 0f, 0.4f);
            Gizmos.DrawWireSphere(transform.position, detectionRange);
        }
    }
}
