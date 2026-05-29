using System.Collections;
using UnityEngine;
using UnityEngine.UI;

namespace EverySingleDay.UI
{
    /// <summary>
    /// A brief title card shown at the start of each generated level: the theme
    /// name, its mood, and the run's objective. Fades out after a couple of
    /// seconds. Built entirely in code (no art), themed by the level's colours.
    /// </summary>
    public class LevelIntroCard : MonoBehaviour
    {
        public static void Show(string title, string mood, string objective,
            Color accent, Color backdrop)
        {
            var go = new GameObject("LevelIntroCard");
            var card = go.AddComponent<LevelIntroCard>();
            card.Build(title, mood, objective, accent, backdrop);
        }

        private CanvasGroup _group;

        private void Build(string title, string mood, string objective,
            Color accent, Color backdrop)
        {
            var canvas = gameObject.AddComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            canvas.sortingOrder = 50;
            var scaler = gameObject.AddComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1920, 1080);

            _group = gameObject.AddComponent<CanvasGroup>();

            var font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            if (font == null) font = Resources.GetBuiltinResource<Font>("Arial.ttf");

            // Dim band behind the text.
            var band = new GameObject("Band");
            band.transform.SetParent(transform, false);
            var bandImg = band.AddComponent<Image>();
            bandImg.color = new Color(backdrop.r, backdrop.g, backdrop.b, 0.55f);
            var brt = bandImg.rectTransform;
            brt.anchorMin = new Vector2(0f, 0.4f);
            brt.anchorMax = new Vector2(1f, 0.62f);
            brt.offsetMin = Vector2.zero; brt.offsetMax = Vector2.zero;

            Text(font, title, 96, accent, new Vector2(0, 70), FontStyle.Bold);
            Text(font, $"“{mood}”", 40, Color.white, new Vector2(0, -10), FontStyle.Italic);
            Text(font, objective.ToUpperInvariant(), 38, accent, new Vector2(0, -80), FontStyle.Bold);

            StartCoroutine(FadeRoutine());
        }

        private void Text(Font font, string content, int size, Color color,
            Vector2 pos, FontStyle style)
        {
            var go = new GameObject("Text");
            go.transform.SetParent(transform, false);
            var txt = go.AddComponent<Text>();
            txt.font = font;
            txt.text = content;
            txt.fontSize = size;
            txt.fontStyle = style;
            txt.alignment = TextAnchor.MiddleCenter;
            txt.color = color;
            txt.horizontalOverflow = HorizontalWrapMode.Overflow;
            txt.verticalOverflow = VerticalWrapMode.Overflow;
            var rt = txt.rectTransform;
            rt.anchorMin = new Vector2(0.5f, 0.5f);
            rt.anchorMax = new Vector2(0.5f, 0.5f);
            rt.pivot = new Vector2(0.5f, 0.5f);
            rt.anchoredPosition = pos;
            rt.sizeDelta = new Vector2(1600, 160);
        }

        private IEnumerator FadeRoutine()
        {
            // Hold, then fade out. Uses unscaled time so it works even if the
            // game starts paused for any reason.
            _group.alpha = 1f;
            yield return new WaitForSecondsRealtime(2.2f);
            float t = 0f;
            while (t < 0.6f)
            {
                t += Time.unscaledDeltaTime;
                _group.alpha = 1f - (t / 0.6f);
                yield return null;
            }
            Destroy(gameObject);
        }
    }
}
