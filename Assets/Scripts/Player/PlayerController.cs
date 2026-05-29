using UnityEngine;

namespace EverySingleDay.Player
{
    /// <summary>
    /// Responsive 2D platformer controller featuring the "game feel" tricks that
    /// separate a hit platformer from a clunky one: coyote time, jump buffering,
    /// variable jump height, fast-fall gravity, air control, double jump and
    /// wall sliding / wall jumping.
    ///
    /// Requires a Rigidbody2D (set to Dynamic, gravity scale handled here) and a
    /// CapsuleCollider2D on the same GameObject.
    /// </summary>
    [RequireComponent(typeof(Rigidbody2D))]
    public class PlayerController : MonoBehaviour
    {
        [Header("Movement")]
        [Tooltip("Top horizontal speed in units/second.")]
        public float moveSpeed = 9f;
        [Tooltip("How quickly the player reaches top speed on the ground.")]
        public float groundAcceleration = 90f;
        [Tooltip("How quickly the player stops on the ground.")]
        public float groundDeceleration = 100f;
        [Tooltip("Acceleration multiplier while airborne (0-1 = less control).")]
        [Range(0f, 1f)] public float airControl = 0.65f;

        [Header("Jumping")]
        [Tooltip("Peak jump height in world units for a full-held jump.")]
        public float jumpHeight = 3.6f;
        [Tooltip("Time to reach the apex of the jump.")]
        public float timeToApex = 0.38f;
        [Tooltip("Extra gravity multiplier while falling (snappier feel).")]
        public float fallGravityMultiplier = 1.9f;
        [Tooltip("Gravity multiplier when the jump button is released early.")]
        public float lowJumpMultiplier = 2.6f;
        [Tooltip("Number of jumps available (1 = single, 2 = double jump).")]
        public int maxJumps = 2;
        [Tooltip("Maximum downward speed (terminal velocity).")]
        public float maxFallSpeed = 24f;

        [Header("Assist Timers")]
        [Tooltip("Grace period after leaving a ledge during which you can still jump.")]
        public float coyoteTime = 0.12f;
        [Tooltip("How early a jump press is remembered before landing.")]
        public float jumpBufferTime = 0.12f;

        [Header("Wall Interaction")]
        public bool enableWallJump = true;
        public float wallSlideSpeed = 2.5f;
        public Vector2 wallJumpForce = new Vector2(12f, 16f);
        [Tooltip("How long horizontal input is locked out after a wall jump.")]
        public float wallJumpLockTime = 0.16f;

        [Header("Ground / Wall Checks")]
        public LayerMask groundLayer;
        public Transform groundCheck;
        public float groundCheckRadius = 0.18f;
        public Transform wallCheck;
        public float wallCheckDistance = 0.3f;

        // --- runtime state ---
        public bool IsGrounded { get; private set; }
        public bool IsWallSliding { get; private set; }
        public int FacingDirection { get; private set; } = 1;
        public Vector2 Velocity => _rb != null ? _rb.velocity : Vector2.zero;

        private Rigidbody2D _rb;
        private float _gravity;          // derived from jumpHeight + timeToApex
        private float _jumpVelocity;     // derived initial jump speed
        private float _baseGravityScale;

        private float _coyoteCounter;
        private float _jumpBufferCounter;
        private float _wallJumpLockCounter;
        private int _jumpsRemaining;
        private float _moveInput;
        private bool _jumpHeld;
        private bool _controlsLocked;

        // Events other systems hook into (animation, audio, particles).
        public System.Action OnJumped;
        public System.Action OnLanded;
        public System.Action OnDoubleJumped;
        public System.Action OnWallJumped;

        private void Awake()
        {
            _rb = GetComponent<Rigidbody2D>();

            // Kinematic-style derived physics so designers tune height + time,
            // not raw gravity/force numbers.
            _gravity = (2f * jumpHeight) / (timeToApex * timeToApex);
            _jumpVelocity = _gravity * timeToApex;
            _baseGravityScale = _gravity / Mathf.Abs(Physics2D.gravity.y);

            _rb.gravityScale = _baseGravityScale;
            _rb.freezeRotation = true;
            _rb.collisionDetectionMode = CollisionDetectionMode2D.Continuous;
            _rb.interpolation = RigidbodyInterpolation2D.Interpolate;
        }

        private void Update()
        {
            if (_controlsLocked)
            {
                _moveInput = 0f;
                return;
            }

            ReadInput();
            UpdateTimers();
            HandleJumpInput();
            HandleSpriteFlip();
        }

        private void FixedUpdate()
        {
            if (_controlsLocked)
                return;

            ProbeEnvironment();
            ApplyHorizontalMovement();
            ApplyWallSlide();
            ApplyBetterGravity();
            ClampFallSpeed();
        }

        private void ReadInput()
        {
            _moveInput = Input.GetAxisRaw("Horizontal");
            _jumpHeld = Input.GetButton("Jump");

            if (Input.GetButtonDown("Jump"))
                _jumpBufferCounter = jumpBufferTime;
        }

        private void UpdateTimers()
        {
            _jumpBufferCounter -= Time.deltaTime;
            _wallJumpLockCounter -= Time.deltaTime;

            if (IsGrounded)
            {
                _coyoteCounter = coyoteTime;
                _jumpsRemaining = maxJumps;
            }
            else
            {
                _coyoteCounter -= Time.deltaTime;
            }
        }

        private void HandleJumpInput()
        {
            if (_jumpBufferCounter <= 0f)
                return;

            // Ground / coyote jump.
            if (_coyoteCounter > 0f)
            {
                PerformJump(_jumpVelocity);
                _jumpsRemaining = maxJumps - 1;
                _coyoteCounter = 0f;
                OnJumped?.Invoke();
                return;
            }

            // Wall jump takes priority over double jump when sliding.
            if (IsWallSliding && enableWallJump)
            {
                int dir = -FacingDirection;
                _rb.velocity = new Vector2(wallJumpForce.x * dir, wallJumpForce.y);
                _wallJumpLockCounter = wallJumpLockTime;
                _jumpBufferCounter = 0f;
                FacingDirection = dir;
                OnWallJumped?.Invoke();
                return;
            }

            // Mid-air (double) jump.
            if (_jumpsRemaining > 0)
            {
                PerformJump(_jumpVelocity);
                _jumpsRemaining--;
                OnDoubleJumped?.Invoke();
            }
        }

        private void PerformJump(float velocity)
        {
            _rb.velocity = new Vector2(_rb.velocity.x, velocity);
            _jumpBufferCounter = 0f;
        }

        private void ProbeEnvironment()
        {
            bool wasGrounded = IsGrounded;

            IsGrounded = groundCheck != null &&
                         Physics2D.OverlapCircle(groundCheck.position, groundCheckRadius, groundLayer);

            if (IsGrounded && !wasGrounded && _rb.velocity.y <= 0.1f)
                OnLanded?.Invoke();
        }

        private void ApplyHorizontalMovement()
        {
            // Skip horizontal control briefly after a wall jump so the push sticks.
            if (_wallJumpLockCounter > 0f)
                return;

            float targetSpeed = _moveInput * moveSpeed;
            float accelRate;

            if (Mathf.Abs(targetSpeed) > 0.01f)
                accelRate = groundAcceleration;
            else
                accelRate = groundDeceleration;

            if (!IsGrounded)
                accelRate *= airControl;

            float speedDiff = targetSpeed - _rb.velocity.x;
            float movement = speedDiff * accelRate * Time.fixedDeltaTime;
            _rb.velocity = new Vector2(_rb.velocity.x + movement, _rb.velocity.y);
        }

        private void ApplyWallSlide()
        {
            IsWallSliding = false;
            if (!enableWallJump || IsGrounded || wallCheck == null)
                return;

            bool touchingWall = Physics2D.Raycast(wallCheck.position,
                Vector2.right * FacingDirection, wallCheckDistance, groundLayer);

            bool pushingIntoWall = Mathf.Abs(_moveInput) > 0.01f &&
                                   Mathf.Sign(_moveInput) == FacingDirection;

            if (touchingWall && pushingIntoWall && _rb.velocity.y < 0f)
            {
                IsWallSliding = true;
                _rb.velocity = new Vector2(_rb.velocity.x,
                    Mathf.Max(_rb.velocity.y, -wallSlideSpeed));
            }
        }

        private void ApplyBetterGravity()
        {
            if (_rb.velocity.y < 0f)
            {
                // Falling: heavier gravity for a snappy descent.
                _rb.gravityScale = _baseGravityScale * fallGravityMultiplier;
            }
            else if (_rb.velocity.y > 0f && !_jumpHeld)
            {
                // Rising but jump released: cut the jump short.
                _rb.gravityScale = _baseGravityScale * lowJumpMultiplier;
            }
            else
            {
                _rb.gravityScale = _baseGravityScale;
            }
        }

        private void ClampFallSpeed()
        {
            if (_rb.velocity.y < -maxFallSpeed)
                _rb.velocity = new Vector2(_rb.velocity.x, -maxFallSpeed);
        }

        private void HandleSpriteFlip()
        {
            if (_wallJumpLockCounter > 0f)
                return;

            if (_moveInput > 0.01f && FacingDirection != 1)
                Flip(1);
            else if (_moveInput < -0.01f && FacingDirection != -1)
                Flip(-1);
        }

        private void Flip(int dir)
        {
            FacingDirection = dir;
            Vector3 s = transform.localScale;
            s.x = Mathf.Abs(s.x) * dir;
            transform.localScale = s;
        }

        /// <summary>Apply an external bounce (e.g. after stomping an enemy).</summary>
        public void Bounce(float force)
        {
            _rb.velocity = new Vector2(_rb.velocity.x, force);
            _jumpsRemaining = maxJumps; // reward a stomp with refreshed jumps
        }

        /// <summary>Knockback used when taking damage.</summary>
        public void ApplyKnockback(Vector2 force)
        {
            _rb.velocity = force;
        }

        public void SetControlsLocked(bool locked)
        {
            _controlsLocked = locked;
            if (locked)
                _rb.velocity = Vector2.zero;
        }

        private void OnDrawGizmosSelected()
        {
            if (groundCheck != null)
            {
                Gizmos.color = Color.green;
                Gizmos.DrawWireSphere(groundCheck.position, groundCheckRadius);
            }
            if (wallCheck != null)
            {
                Gizmos.color = Color.cyan;
                Gizmos.DrawLine(wallCheck.position,
                    wallCheck.position + Vector3.right * FacingDirection * wallCheckDistance);
            }
        }
    }
}
