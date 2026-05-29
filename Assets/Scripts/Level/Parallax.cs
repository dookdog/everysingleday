using UnityEngine;

namespace EverySingleDay.Level
{
    /// <summary>
    /// Multi-layer parallax background. Each layer scrolls a fraction of the
    /// camera's movement based on its depth, creating a sense of distance.
    /// Attach to a parent holding background layer SpriteRenderers, or set
    /// per-layer factors manually.
    /// </summary>
    public class Parallax : MonoBehaviour
    {
        [System.Serializable]
        public class Layer
        {
            public Transform transform;
            [Range(0f, 1f)] public float parallaxFactor = 0.5f;
            [Tooltip("Scroll vertically as well as horizontally.")]
            public bool affectY = false;
        }

        public Transform cameraTransform;
        public Layer[] layers;

        private Vector3 _lastCamPos;

        private void Start()
        {
            if (cameraTransform == null && Camera.main != null)
                cameraTransform = Camera.main.transform;
            if (cameraTransform != null)
                _lastCamPos = cameraTransform.position;
        }

        private void LateUpdate()
        {
            if (cameraTransform == null) return;

            Vector3 delta = cameraTransform.position - _lastCamPos;
            foreach (var layer in layers)
            {
                if (layer.transform == null) continue;
                float dy = layer.affectY ? delta.y * layer.parallaxFactor : 0f;
                layer.transform.position += new Vector3(delta.x * layer.parallaxFactor, dy, 0f);
            }
            _lastCamPos = cameraTransform.position;
        }
    }
}
