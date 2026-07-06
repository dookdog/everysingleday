using UnityEngine;

namespace EverySingleDay.CameraSystem
{
    /// <summary>
    /// Drop-in screen shake. Call <see cref="Shake"/> from gameplay events
    /// (damage, landing hard, explosions) for instant game feel.
    /// </summary>
    public class CameraShake : MonoBehaviour
    {
        public static CameraShake Instance { get; private set; }

        private float _duration;
        private float _magnitude;
        private float _elapsed;
        private Vector3 _restLocalPos;

        private void Awake()
        {
            Instance = this;
            _restLocalPos = transform.localPosition;
        }

        public static void Trigger(float duration = 0.15f, float magnitude = 0.2f)
        {
            if (!Systems.GameSettings.ScreenShakeEnabled) return;
            if (Instance != null) Instance.Shake(duration, magnitude);
        }

        public void Shake(float duration, float magnitude)
        {
            _duration = duration;
            _magnitude = magnitude;
            _elapsed = 0f;
            _restLocalPos = transform.localPosition;
        }

        private void LateUpdate()
        {
            if (_elapsed < _duration)
            {
                _elapsed += Time.unscaledDeltaTime;
                float damper = 1f - Mathf.Clamp01(_elapsed / _duration);
                Vector2 random = Random.insideUnitCircle * _magnitude * damper;
                transform.localPosition = _restLocalPos + (Vector3)random;
            }
            else if (_duration > 0f)
            {
                transform.localPosition = _restLocalPos;
                _duration = 0f;
            }
        }
    }
}
