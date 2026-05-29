using UnityEngine;

namespace EverySingleDay.Enemies
{
    /// <summary>
    /// Classic platformer ground enemy. Walks back and forth, turning at walls
    /// and ledge edges. Damages the player on side contact, but can be defeated
    /// by stomping it from above (Mario-style), which bounces the player.
    /// </summary>
    [RequireComponent(typeof(Rigidbody2D))]
    public class PatrolEnemy : MonoBehaviour
    {
        [Header("Movement")]
        public float moveSpeed = 2.5f;
        public int startDirection = -1;

        [Header("Edge / Wall Detection")]
        public LayerMask groundLayer;
        public Transform groundCheck;
        public Transform wallCheck;
        public float groundCheckDistance = 0.6f;
        public float wallCheckDistance = 0.25f;

        [Header("Combat")]
        public int contactDamage = 1;
        [Tooltip("Upward velocity given to the player after a successful stomp.")]
        public float stompBounce = 16f;
        [Tooltip("Angle tolerance: how 'on top' the player must be to stomp.")]
        public float stompDotThreshold = 0.5f;

        [Header("Feedback")]
        public GameObject deathEffect;
        public AudioClip stompClip;
        public int scoreValue = 100;

        private Rigidbody2D _rb;
        private int _direction;
        private bool _dead;

        private void Awake()
        {
            _rb = GetComponent<Rigidbody2D>();
            _rb.freezeRotation = true;
            _direction = startDirection >= 0 ? 1 : -1;
            ApplyFacing();
        }

        private void FixedUpdate()
        {
            if (_dead) return;

            bool groundAhead = groundCheck != null &&
                Physics2D.Raycast(groundCheck.position, Vector2.down, groundCheckDistance, groundLayer);
            bool wallAhead = wallCheck != null &&
                Physics2D.Raycast(wallCheck.position, Vector2.right * _direction, wallCheckDistance, groundLayer);

            if (!groundAhead || wallAhead)
                Turn();

            _rb.velocity = new Vector2(_direction * moveSpeed, _rb.velocity.y);
        }

        private void Turn()
        {
            _direction *= -1;
            ApplyFacing();
        }

        private void ApplyFacing()
        {
            Vector3 s = transform.localScale;
            s.x = Mathf.Abs(s.x) * _direction;
            transform.localScale = s;
        }

        private void OnCollisionEnter2D(Collision2D collision)
        {
            HandlePlayerContact(collision);
        }

        private void OnCollisionStay2D(Collision2D collision)
        {
            HandlePlayerContact(collision);
        }

        private void HandlePlayerContact(Collision2D collision)
        {
            if (_dead) return;
            if (!collision.collider.CompareTag("Player")) return;

            var controller = collision.collider.GetComponent<Player.PlayerController>();
            var health = collision.collider.GetComponent<Player.PlayerHealth>();

            // Determine if the contact came from above (a stomp).
            bool stomped = false;
            foreach (var contact in collision.contacts)
            {
                // contact.normal points from this enemy toward the player.
                if (contact.normal.y < -stompDotThreshold)
                {
                    stomped = true;
                    break;
                }
            }

            if (stomped && controller != null)
            {
                controller.Bounce(stompBounce);
                Die();
            }
            else if (health != null)
            {
                health.TakeDamage(contactDamage, transform.position);
            }
        }

        private void Die()
        {
            _dead = true;
            Systems.AudioManager.Play(stompClip);
            Systems.GameManager.Instance?.AddScore(scoreValue);

            if (deathEffect != null)
                Instantiate(deathEffect, transform.position, Quaternion.identity);

            Destroy(gameObject);
        }

        private void OnDrawGizmosSelected()
        {
            if (groundCheck != null)
            {
                Gizmos.color = Color.yellow;
                Gizmos.DrawLine(groundCheck.position,
                    groundCheck.position + Vector3.down * groundCheckDistance);
            }
            if (wallCheck != null)
            {
                Gizmos.color = Color.red;
                int d = Application.isPlaying ? _direction : (startDirection >= 0 ? 1 : -1);
                Gizmos.DrawLine(wallCheck.position,
                    wallCheck.position + Vector3.right * d * wallCheckDistance);
            }
        }
    }
}
