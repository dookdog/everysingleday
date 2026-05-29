using UnityEngine;

namespace EverySingleDay.Systems
{
    /// <summary>
    /// Destroys a spawned effect once its ParticleSystem finishes (or after a
    /// fallback lifetime). Attach to any one-shot VFX prefab.
    /// </summary>
    public class ParticleAutoDestroy : MonoBehaviour
    {
        public float fallbackLifetime = 3f;

        private void Start()
        {
            var ps = GetComponent<ParticleSystem>();
            float life = fallbackLifetime;
            if (ps != null)
                life = ps.main.duration + ps.main.startLifetime.constantMax;
            Destroy(gameObject, life);
        }
    }
}
