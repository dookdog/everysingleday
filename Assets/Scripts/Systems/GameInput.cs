using UnityEngine;

namespace EverySingleDay.Systems
{
    /// <summary>
    /// Single source of truth for player input, merging hardware (keyboard /
    /// gamepad via the legacy Input axes) with on-screen touch controls. The
    /// player reads from here instead of UnityEngine.Input directly, so the exact
    /// same controller works on desktop and on an iPhone with no code change.
    ///
    /// Touch UI (see <see cref="UI.TouchControls"/>) pushes state in via the
    /// Set* methods; keyboard is polled live. Keyboard wins when both are active.
    /// </summary>
    public static class GameInput
    {
        private static bool _left, _right, _jumpHeld;
        private static bool _jumpQueued;   // edge, consumed by JumpPressed()

        // ---- touch setters (called by on-screen buttons) ----
        public static void SetLeft(bool v) => _left = v;
        public static void SetRight(bool v) => _right = v;

        public static void SetJump(bool held)
        {
            if (held && !_jumpHeld) _jumpQueued = true; // rising edge
            _jumpHeld = held;
        }

        /// <summary>Queue a single jump press (for a tap-style button).</summary>
        public static void QueueJump() => _jumpQueued = true;

        // ---- reads (used by PlayerController) ----
        public static float Horizontal()
        {
            float kb = Input.GetAxisRaw("Horizontal");
            if (Mathf.Abs(kb) > 0.01f) return kb;
            return (_right ? 1f : 0f) - (_left ? 1f : 0f);
        }

        public static bool JumpHeld()
        {
            return _jumpHeld || Input.GetButton("Jump");
        }

        /// <summary>True once per press (edge). Consumes the touch-queued flag.</summary>
        public static bool JumpPressed()
        {
            bool edge = _jumpQueued || Input.GetButtonDown("Jump");
            _jumpQueued = false;
            return edge;
        }

        /// <summary>Reset all virtual state (e.g. on scene reload / respawn).</summary>
        public static void Clear()
        {
            _left = _right = _jumpHeld = _jumpQueued = false;
        }
    }
}
