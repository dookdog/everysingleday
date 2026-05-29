using UnityEngine;

namespace EverySingleDay.Enemies
{
    /// <summary>
    /// Simple travelling projectile that damages the player on contact and
    /// self-destructs after a lifetime or when it hits solid ground.
    /// </summary>
    [RequireComponent(typeof(Rigidbody2D))]
    public class Projectile : MonoBehaviour
    {
        public float speed = 8f;
        public int damage = 1;
        public float lifetime = 4f;
        public LayerMask groundLayer;
        public GameObject hitEffect;

        private Rigidbody2D _rb;
        private Vector2 _direction = Vector2.left;

        private void Awake()
        {
            _rb = GetComponent<Rigidbody2D>();
            _rb.gravityScale = 0f;
        }

        public void Launch(Vector2 direction)
        {
            _direction = direction.normalized;
            Destroy(gameObject, lifetime);
        }

        private void Start()
        {
            // If launched without an explicit direction, fall back to facing.
            if (_direction == Vector2.zero)
                _direction = transform.localScale.x >= 0 ? Vector2.right : Vector2.left;
            Destroy(gameObject, lifetime);
        }

        private void FixedUpdate()
        {
            _rb.velocity = _direction * speed;
        }

        private void OnTriggerEnter2D(Collider2D other)
        {
            if (other.CompareTag("Player"))
            {
                var health = other.GetComponent<Player.PlayerHealth>();
                if (health != null)
                    health.TakeDamage(damage, transform.position);
                Explode();
            }
            else if (((1 << other.gameObject.layer) & groundLayer) != 0)
            {
                Explode();
            }
        }

        private void Explode()
        {
            if (hitEffect != null)
                Instantiate(hitEffect, transform.position, Quaternion.identity);
            Destroy(gameObject);
        }
    }
}
