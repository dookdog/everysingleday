using System.Collections.Generic;
using UnityEngine;
using EverySingleDay.Level;

namespace EverySingleDay.Systems
{
    /// <summary>
    /// Builds a near foreground layer of pure silhouette shapes (black by
    /// default) that parallax faster than the camera and frame the top and
    /// bottom of the screen. Flat single-fill shapes with no internal detail —
    /// the Hollow Knight / Limbo trick: instant readability at near-zero cost.
    /// </summary>
    public static class ForegroundSilhouettes
    {
        public static void Build(LevelTheme theme, LevelData level, Transform camera)
        {
            var root = new GameObject("ForegroundSilhouettes");
            var parallax = root.AddComponent<Parallax>();
            parallax.cameraTransform = camera;

            var band = new GameObject("FGBand");
            band.transform.SetParent(root.transform, false);

            var rng = new System.Random(level.seed ^ 0x5EED);
            float span = level.worldMax.x - level.worldMin.x;
            int clusters = Mathf.Clamp(Mathf.CeilToInt(span / 10f) + 2, 4, 20);

            for (int i = 0; i < clusters; i++)
            {
                float x = level.worldMin.x + (float)rng.NextDouble() * span;
                bool bottom = rng.NextDouble() < 0.7; // mostly bottom framing
                float y = bottom ? level.worldMin.y + 10f + (float)rng.NextDouble() * 2f
                                 : level.worldMax.y - 2f - (float)rng.NextDouble() * 2f;
                AddCluster(band.transform, theme, new Vector2(x, y), bottom, rng);
            }

            parallax.layers = new[]
            {
                new Parallax.Layer { transform = band.transform, parallaxFactor = 1.35f }
            };
        }

        private static void AddCluster(Transform parent, LevelTheme theme,
            Vector2 pos, bool bottom, System.Random rng)
        {
            int shapes = rng.Next(2, 5);
            for (int i = 0; i < shapes; i++)
            {
                var go = new GameObject("Silhouette");
                go.transform.SetParent(parent, false);
                float ox = (float)(rng.NextDouble() - 0.5) * 4f;
                go.transform.position = new Vector3(pos.x + ox, pos.y, -2f); // in front

                var sr = go.AddComponent<SpriteRenderer>();
                sr.color = theme.silhouetteColor;
                sr.sortingOrder = 20; // above gameplay

                int kind = rng.Next(3);
                float w = 1.5f + (float)rng.NextDouble() * 4f;
                float h = 2f + (float)rng.NextDouble() * 6f;

                switch (kind)
                {
                    case 0: // jagged spire (stalactite/stalagmite)
                        sr.sprite = PrimitiveFactory.SpikeSprite(theme.silhouetteColor);
                        sr.drawMode = SpriteDrawMode.Sliced;
                        sr.size = new Vector2(w, h);
                        // point down if hanging from the top
                        go.transform.localScale = new Vector3(1f, bottom ? 1f : -1f, 1f);
                        break;
                    case 1: // rounded mass (foliage/rock)
                        sr.sprite = PrimitiveFactory.CircleSprite(theme.silhouetteColor);
                        sr.drawMode = SpriteDrawMode.Sliced;
                        sr.size = new Vector2(w * 1.4f, w * 1.4f);
                        break;
                    default: // slab (pillar/wall)
                        sr.sprite = PrimitiveFactory.SolidSprite(theme.silhouetteColor);
                        sr.drawMode = SpriteDrawMode.Sliced;
                        sr.size = new Vector2(w * 0.6f, h);
                        break;
                }
            }
        }
    }
}
