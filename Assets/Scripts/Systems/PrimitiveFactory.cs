using UnityEngine;

namespace EverySingleDay.Systems
{
    /// <summary>
    /// Generates simple solid-colour sprites at runtime so the game is fully
    /// playable with zero imported art. Swap these for real artwork later by
    /// assigning sprites in the inspector.
    /// </summary>
    public static class PrimitiveFactory
    {
        private const int PixelsPerUnit = 32;

        /// <summary>Solid rectangle sprite, 1x1 unit by default (tiled via scale).</summary>
        public static Sprite SolidSprite(Color color)
        {
            var tex = new Texture2D(PixelsPerUnit, PixelsPerUnit, TextureFormat.RGBA32, false)
            {
                filterMode = FilterMode.Point,
                wrapMode = TextureWrapMode.Clamp
            };
            var pixels = new Color[PixelsPerUnit * PixelsPerUnit];
            for (int i = 0; i < pixels.Length; i++) pixels[i] = color;
            tex.SetPixels(pixels);
            tex.Apply();
            return Sprite.Create(tex, new Rect(0, 0, PixelsPerUnit, PixelsPerUnit),
                new Vector2(0.5f, 0.5f), PixelsPerUnit);
        }

        /// <summary>Filled circle sprite (used for coins, the player, particles).</summary>
        public static Sprite CircleSprite(Color color)
        {
            int size = PixelsPerUnit;
            var tex = new Texture2D(size, size, TextureFormat.RGBA32, false)
            {
                filterMode = FilterMode.Bilinear,
                wrapMode = TextureWrapMode.Clamp
            };
            float r = size / 2f;
            var pixels = new Color[size * size];
            for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                float dx = x - r + 0.5f;
                float dy = y - r + 0.5f;
                bool inside = dx * dx + dy * dy <= (r - 0.5f) * (r - 0.5f);
                pixels[y * size + x] = inside ? color : Color.clear;
            }
            tex.SetPixels(pixels);
            tex.Apply();
            return Sprite.Create(tex, new Rect(0, 0, size, size),
                new Vector2(0.5f, 0.5f), PixelsPerUnit);
        }

        /// <summary>
        /// Soft radial glow sprite (opaque core fading to transparent edge).
        /// Used for focal halos and light bloom in the negative-space themes —
        /// a cheap way to make lit elements pop against darkness.
        /// </summary>
        public static Sprite GlowSprite(Color color)
        {
            int size = PixelsPerUnit * 2; // extra resolution for a smooth falloff
            var tex = new Texture2D(size, size, TextureFormat.RGBA32, false)
            {
                filterMode = FilterMode.Bilinear,
                wrapMode = TextureWrapMode.Clamp
            };
            float r = size / 2f;
            var pixels = new Color[size * size];
            for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                float dx = (x - r + 0.5f) / r;
                float dy = (y - r + 0.5f) / r;
                float d = Mathf.Sqrt(dx * dx + dy * dy);       // 0 centre -> 1 edge
                float a = Mathf.Clamp01(1f - d);
                a = a * a;                                     // quadratic falloff
                pixels[y * size + x] = new Color(color.r, color.g, color.b, color.a * a);
            }
            tex.SetPixels(pixels);
            tex.Apply();
            return Sprite.Create(tex, new Rect(0, 0, size, size),
                new Vector2(0.5f, 0.5f), PixelsPerUnit);
        }

        /// <summary>
        /// Vertical two-stop gradient sprite (top colour -> bottom colour). Used
        /// for atmospheric skies and fog bands that fade geometry into darkness.
        /// </summary>
        public static Sprite GradientSprite(Color top, Color bottom)
        {
            int w = 4, h = PixelsPerUnit;
            var tex = new Texture2D(w, h, TextureFormat.RGBA32, false)
            {
                filterMode = FilterMode.Bilinear,
                wrapMode = TextureWrapMode.Clamp
            };
            var pixels = new Color[w * h];
            for (int y = 0; y < h; y++)
            {
                Color c = Color.Lerp(bottom, top, y / (float)(h - 1));
                for (int x = 0; x < w; x++) pixels[y * w + x] = c;
            }
            tex.SetPixels(pixels);
            tex.Apply();
            return Sprite.Create(tex, new Rect(0, 0, w, h), new Vector2(0.5f, 0.5f), PixelsPerUnit);
        }

        /// <summary>Upward-pointing triangle sprite for spikes.</summary>
        public static Sprite SpikeSprite(Color color)
        {
            int size = PixelsPerUnit;
            var tex = new Texture2D(size, size, TextureFormat.RGBA32, false)
            {
                filterMode = FilterMode.Point,
                wrapMode = TextureWrapMode.Clamp
            };
            var pixels = new Color[size * size];
            for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                float t = y / (float)size;            // 0 bottom -> 1 top
                float halfWidth = (1f - t) * (size / 2f);
                bool inside = Mathf.Abs(x - size / 2f) <= halfWidth;
                pixels[y * size + x] = inside ? color : Color.clear;
            }
            tex.SetPixels(pixels);
            tex.Apply();
            return Sprite.Create(tex, new Rect(0, 0, size, size),
                new Vector2(0.5f, 0.5f), PixelsPerUnit);
        }
    }
}
