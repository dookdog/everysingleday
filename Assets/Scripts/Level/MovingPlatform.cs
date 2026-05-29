using UnityEngine;

namespace EverySingleDay.Level
{
    /// <summary>
    /// Moves between waypoints (ping-pong or looping) and carries any rigidbody
    /// resting on top by parenting it while in contact — so the player rides the
    /// platform naturally instead of sliding off.
    /// </summary>
    public class MovingPlatform : MonoBehaviour
    {
        public enum LoopMode { PingPong, Loop }

        [Header("Path")]
        [Tooltip("Local-space offsets relative to the platform's start position.")]
        public Vector2[] waypoints = { Vector2.zero, new Vector2(4f, 0f) };
        public LoopMode loopMode = LoopMode.PingPong;
        public float speed = 2.5f;
        [Tooltip("Pause time in seconds at each waypoint.")]
        public float waitTime = 0.4f;

        private Vector3 _origin;
        private int _index;
        private int _dir = 1;
        private float _waitCounter;
        private Vector3 _lastPos;

        private void Start()
        {
            _origin = transform.position;
            if (waypoints == null || waypoints.Length == 0)
                waypoints = new[] { Vector2.zero };
            transform.position = _origin + (Vector3)waypoints[0];
            _lastPos = transform.position;
        }

        private void FixedUpdate()
        {
            if (waypoints.Length < 2) return;

            if (_waitCounter > 0f)
            {
                _waitCounter -= Time.fixedDeltaTime;
                _lastPos = transform.position;
                return;
            }

            Vector3 targetPos = _origin + (Vector3)waypoints[_index];
            Vector3 newPos = Vector3.MoveTowards(transform.position, targetPos,
                speed * Time.fixedDeltaTime);
            transform.position = newPos;

            if (Vector3.Distance(transform.position, targetPos) < 0.01f)
                Advance();

            _lastPos = transform.position;
        }

        private void Advance()
        {
            _waitCounter = waitTime;

            if (loopMode == LoopMode.Loop)
            {
                _index = (_index + 1) % waypoints.Length;
            }
            else // PingPong
            {
                _index += _dir;
                if (_index >= waypoints.Length - 1) { _index = waypoints.Length - 1; _dir = -1; }
                else if (_index <= 0) { _index = 0; _dir = 1; }
            }
        }

        private void OnCollisionEnter2D(Collision2D collision)
        {
            if (RidingOnTop(collision))
                collision.transform.SetParent(transform);
        }

        private void OnCollisionExit2D(Collision2D collision)
        {
            if (collision.transform.parent == transform)
                collision.transform.SetParent(null);
        }

        private bool RidingOnTop(Collision2D collision)
        {
            foreach (var c in collision.contacts)
                if (c.normal.y < -0.5f) return true; // normal points down into platform
            return false;
        }

        private void OnDrawGizmosSelected()
        {
            if (waypoints == null || waypoints.Length == 0) return;
            Vector3 baseP = Application.isPlaying ? _origin : transform.position;
            Gizmos.color = Color.cyan;
            for (int i = 0; i < waypoints.Length; i++)
            {
                Vector3 p = baseP + (Vector3)waypoints[i];
                Gizmos.DrawWireSphere(p, 0.15f);
                if (i + 1 < waypoints.Length)
                    Gizmos.DrawLine(p, baseP + (Vector3)waypoints[i + 1]);
            }
        }
    }
}
