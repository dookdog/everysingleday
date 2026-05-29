using UnityEngine;

namespace EverySingleDay.CameraSystem
{
    /// <summary>
    /// Smooth follow camera with horizontal look-ahead (the camera leads in the
    /// direction of travel so the player can see what's coming) and optional
    /// clamping to level bounds. Runs in LateUpdate so it tracks the player
    /// after physics has resolved.
    /// </summary>
    public class CameraFollow : MonoBehaviour
    {
        [Header("Target")]
        public Transform target;

        [Header("Follow")]
        public Vector2 offset = new Vector2(0f, 1.5f);
        public float smoothTime = 0.15f;

        [Header("Look Ahead")]
        public bool useLookAhead = true;
        public float lookAheadDistance = 2.5f;
        public float lookAheadSmoothing = 0.5f;

        [Header("Bounds")]
        public bool useBounds = false;
        public Vector2 minBounds;
        public Vector2 maxBounds;

        private Vector3 _velocity;
        private float _currentLookAhead;
        private float _lookAheadVel;
        private Camera _cam;

        private void Awake()
        {
            _cam = GetComponent<Camera>();
        }

        private void Start()
        {
            if (target == null)
            {
                var pc = FindObjectOfType<Player.PlayerController>();
                if (pc != null) target = pc.transform;
            }
            if (target != null)
                transform.position = DesiredPosition(0f);
        }

        private void LateUpdate()
        {
            if (target == null) return;

            float lookTarget = 0f;
            if (useLookAhead)
            {
                var pc = target.GetComponent<Player.PlayerController>();
                if (pc != null && Mathf.Abs(pc.Velocity.x) > 0.5f)
                    lookTarget = Mathf.Sign(pc.Velocity.x) * lookAheadDistance;
            }
            _currentLookAhead = Mathf.SmoothDamp(_currentLookAhead, lookTarget,
                ref _lookAheadVel, lookAheadSmoothing);

            Vector3 desired = DesiredPosition(_currentLookAhead);
            Vector3 smoothed = Vector3.SmoothDamp(transform.position, desired,
                ref _velocity, smoothTime);
            smoothed.z = transform.position.z; // keep camera depth

            if (useBounds)
                smoothed = ClampToBounds(smoothed);

            transform.position = smoothed;
        }

        private Vector3 DesiredPosition(float lookAhead)
        {
            return new Vector3(
                target.position.x + offset.x + lookAhead,
                target.position.y + offset.y,
                transform.position.z);
        }

        private Vector3 ClampToBounds(Vector3 pos)
        {
            float vertExtent = _cam != null && _cam.orthographic ? _cam.orthographicSize : 0f;
            float horzExtent = _cam != null ? vertExtent * _cam.aspect : 0f;

            pos.x = Mathf.Clamp(pos.x, minBounds.x + horzExtent, maxBounds.x - horzExtent);
            pos.y = Mathf.Clamp(pos.y, minBounds.y + vertExtent, maxBounds.y - vertExtent);
            return pos;
        }

        private void OnDrawGizmosSelected()
        {
            if (!useBounds) return;
            Gizmos.color = Color.magenta;
            Vector3 center = new Vector3((minBounds.x + maxBounds.x) / 2f,
                (minBounds.y + maxBounds.y) / 2f, 0f);
            Vector3 size = new Vector3(maxBounds.x - minBounds.x,
                maxBounds.y - minBounds.y, 1f);
            Gizmos.DrawWireCube(center, size);
        }
    }
}
