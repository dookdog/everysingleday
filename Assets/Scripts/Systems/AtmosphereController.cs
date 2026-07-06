using UnityEngine;

namespace EverySingleDay.Systems
{
    /// <summary>
    /// Screen-space atmosphere for the negative-space themes: a darkened vignette
    /// frame and an optional fog tint, both parented to the camera so they track
    /// it. Cheap (two sprites) and the core of the Hollow Knight / Limbo look —
    /// darkness and framing carry the mood so little geometry needs detail.
    /// </summary>
    public class AtmosphereController : MonoBehaviour
    {
        public static void Attach(Camera cam, LevelTheme theme)
        {
            if (cam == null) return;
            var go = new GameObject("Atmosphere");
            go.transform.SetParent(cam.transform, false);
            var ac = go.AddComponent<AtmosphereController>();
            ac.Build(cam, theme);
        }

        private void Build(Camera cam, LevelTheme theme)
        {
            float h = cam.orthographicSize * 2f;
            float w = h * cam.aspect;

            // Fog tint: a translucent full-screen quad in the fog colour.
            if (theme.fogDensity > 0.001f)
            {
                var fog = MakeQuad("Fog", theme.fogColor, w * 1.2f, h * 1.2f, 8f);
                var c = theme.fogColor; c.a = theme.fogDensity;
                fog.color = c;
                fog.sortingOrder = 90;
            }

            // Vignette: four dark edge glows framing the play area. Built from the
            // glow sprite so the darkening falls off smoothly toward the centre.
            if (theme.vignette > 0.001f)
                BuildVignette(w, h, theme.vignette);
        }

        private void BuildVignette(float w, float h, float strength)
        {
            var root = new GameObject("Vignette");
            root.transform.SetParent(transform, false);
            root.transform.localPosition = new Vector3(0f, 0f, 7f);

            Color edge = new Color(0f, 0f, 0f, Mathf.Clamp01(strength));
            // Big soft dark blobs anchored just off each corner leave a bright
            // centre and shaded edges — a fake-but-convincing vignette.
            float ex = w * 0.62f, ey = h * 0.62f, s = Mathf.Max(w, h) * 0.9f;
            AddGlowBlob(root.transform, new Vector2(-ex, -ey), s, edge);
            AddGlowBlob(root.transform, new Vector2(ex, -ey), s, edge);
            AddGlowBlob(root.transform, new Vector2(-ex, ey), s, edge);
            AddGlowBlob(root.transform, new Vector2(ex, ey), s, edge);
        }

        private void AddGlowBlob(Transform parent, Vector2 pos, float size, Color color)
        {
            var go = new GameObject("Edge");
            go.transform.SetParent(parent, false);
            go.transform.localPosition = new Vector3(pos.x, pos.y, 0f);
            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.GlowSprite(color);
            sr.drawMode = SpriteDrawMode.Sliced;
            sr.size = new Vector2(size, size);
            sr.sortingOrder = 91;
        }

        private SpriteRenderer MakeQuad(string name, Color color, float w, float h, float z)
        {
            var go = new GameObject(name);
            go.transform.SetParent(transform, false);
            go.transform.localPosition = new Vector3(0f, 0f, z);
            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = PrimitiveFactory.SolidSprite(color);
            sr.drawMode = SpriteDrawMode.Sliced;
            sr.size = new Vector2(w, h);
            return sr;
        }

        /// <summary>
        /// Attach a soft emissive halo behind a lit element (player, gem, goal).
        /// </summary>
        public static void AddFocalGlow(Transform target, Color color, float size)
        {
            var go = new GameObject("Glow");
            go.transform.SetParent(target, false);
            go.transform.localPosition = new Vector3(0f, 0f, 0.1f);
            var sr = go.AddComponent<SpriteRenderer>();
            var glow = color; glow.a = 0.5f;
            sr.sprite = PrimitiveFactory.GlowSprite(glow);
            sr.drawMode = SpriteDrawMode.Sliced;
            sr.size = new Vector2(size, size);
            sr.sortingOrder = 4; // behind the element (which sits higher)
            go.AddComponent<GlowPulse>();
        }
    }

    /// <summary>Subtle breathing pulse for focal glows.</summary>
    public class GlowPulse : MonoBehaviour
    {
        public float amount = 0.12f;
        public float speed = 2.2f;
        private Vector3 _base;
        private void Start() => _base = transform.localScale;
        private void Update()
        {
            float s = 1f + Mathf.Sin((Time.time + GetInstanceID() * 0.001f) * speed) * amount;
            transform.localScale = _base * s;
        }
    }
}
