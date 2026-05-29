using UnityEngine;

namespace EverySingleDay.Player
{
    /// <summary>
    /// Bridges the PlayerController state to an Animator and spawns simple
    /// feedback (particles / audio) on key events. All animator parameters are
    /// looked up by hash and guarded, so it degrades gracefully if a parameter
    /// is missing from the controller.
    /// </summary>
    [RequireComponent(typeof(PlayerController))]
    public class PlayerAnimator : MonoBehaviour
    {
        [Header("References")]
        public Animator animator;

        [Header("Audio (optional)")]
        public AudioClip jumpClip;
        public AudioClip landClip;
        public AudioClip doubleJumpClip;

        private PlayerController _controller;
        private Rigidbody2D _rb;

        private static readonly int SpeedHash = Animator.StringToHash("Speed");
        private static readonly int GroundedHash = Animator.StringToHash("IsGrounded");
        private static readonly int VerticalHash = Animator.StringToHash("VerticalVelocity");
        private static readonly int WallSlideHash = Animator.StringToHash("IsWallSliding");
        private static readonly int JumpTrigger = Animator.StringToHash("Jump");

        private void Awake()
        {
            _controller = GetComponent<PlayerController>();
            _rb = GetComponent<Rigidbody2D>();
            if (animator == null)
                animator = GetComponentInChildren<Animator>();
        }

        private void OnEnable()
        {
            _controller.OnJumped += HandleJump;
            _controller.OnDoubleJumped += HandleDoubleJump;
            _controller.OnLanded += HandleLand;
        }

        private void OnDisable()
        {
            _controller.OnJumped -= HandleJump;
            _controller.OnDoubleJumped -= HandleDoubleJump;
            _controller.OnLanded -= HandleLand;
        }

        private void Update()
        {
            if (animator == null) return;
            if (!animator.isActiveAndEnabled) return;

            SetFloat(SpeedHash, Mathf.Abs(_controller.Velocity.x));
            SetFloat(VerticalHash, _controller.Velocity.y);
            SetBool(GroundedHash, _controller.IsGrounded);
            SetBool(WallSlideHash, _controller.IsWallSliding);
        }

        private void HandleJump()
        {
            SetTrigger(JumpTrigger);
            Systems.AudioManager.Play(jumpClip);
        }

        private void HandleDoubleJump()
        {
            SetTrigger(JumpTrigger);
            Systems.AudioManager.Play(doubleJumpClip != null ? doubleJumpClip : jumpClip);
        }

        private void HandleLand()
        {
            Systems.AudioManager.Play(landClip);
        }

        // --- safe animator setters (no-op if parameter absent) ---
        private bool Has(int hash)
        {
            foreach (var p in animator.parameters)
                if (p.nameHash == hash) return true;
            return false;
        }

        private void SetFloat(int hash, float v) { if (Has(hash)) animator.SetFloat(hash, v); }
        private void SetBool(int hash, bool v) { if (Has(hash)) animator.SetBool(hash, v); }
        private void SetTrigger(int hash) { if (Has(hash)) animator.SetTrigger(hash); }
    }
}
