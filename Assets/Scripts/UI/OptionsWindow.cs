using System;
using UnityEngine;
using UnityEngine.UI;
using EverySingleDay.Systems;

namespace EverySingleDay.UI
{
    /// <summary>
    /// A self-contained Options window: a titled panel of tickboxes bound to
    /// <see cref="GameSettings"/>, plus a Close button. Built entirely in code
    /// (no art / prefabs). Call <see cref="Open"/> to show it as a modal overlay;
    /// it saves each toggle immediately and destroys itself on close.
    /// </summary>
    public class OptionsWindow : MonoBehaviour
    {
        private Action _onClose;
        private Font _font;

        /// <summary>Show the Options window on the given canvas (modal overlay).</summary>
        public static OptionsWindow Open(Transform canvasParent, Action onClose = null)
        {
            var go = new GameObject("OptionsWindow");
            go.transform.SetParent(canvasParent, false);
            var win = go.AddComponent<OptionsWindow>();
            win._onClose = onClose;
            win.Build();
            return win;
        }

        private void Build()
        {
            _font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            if (_font == null) _font = Resources.GetBuiltinResource<Font>("Arial.ttf");

            // Full-screen dim to make it modal (blocks clicks behind it).
            var dim = AddImage(transform, new Color(0f, 0f, 0f, 0.6f));
            Stretch(dim.rectTransform);
            dim.gameObject.AddComponent<Button>(); // swallow clicks outside the panel

            // Centered panel.
            var panel = AddImage(transform, new Color(0.12f, 0.14f, 0.20f, 0.98f));
            var prt = panel.rectTransform;
            prt.anchorMin = prt.anchorMax = new Vector2(0.5f, 0.5f);
            prt.pivot = new Vector2(0.5f, 0.5f);
            prt.sizeDelta = new Vector2(760, 760);

            // Title.
            var title = MakeText(panel.transform, "OPTIONS", 56, new Color(1f, 0.85f, 0.3f),
                new Vector2(0, 320), TextAnchor.MiddleCenter, FontStyle.Bold);
            title.rectTransform.sizeDelta = new Vector2(700, 80);

            // Tickboxes — the actual "boxes you can tick".
            float y = 210f;
            const float row = 78f;
            AddCheckbox(panel.transform, "Music", y, GameSettings.MusicEnabled,
                v => GameSettings.MusicEnabled = v); y -= row;
            AddCheckbox(panel.transform, "Sound Effects", y, GameSettings.SfxEnabled,
                v => GameSettings.SfxEnabled = v); y -= row;
            AddCheckbox(panel.transform, "Screen Shake", y, GameSettings.ScreenShakeEnabled,
                v => GameSettings.ScreenShakeEnabled = v); y -= row;
            AddCheckbox(panel.transform, "Show Control Hints", y, GameSettings.ShowHints,
                v => GameSettings.ShowHints = v); y -= row;
            AddCheckbox(panel.transform, "Show Timer", y, GameSettings.ShowTimer,
                v => GameSettings.ShowTimer = v); y -= row;
            AddCheckbox(panel.transform, "Hard Mode", y, GameSettings.HardMode,
                v => GameSettings.HardMode = v); y -= row;

            // Close button.
            var close = MakeButton(panel.transform, "CLOSE", new Vector2(0, -320),
                new Color(0.30f, 0.75f, 0.45f), Close);
            close.GetComponent<RectTransform>().sizeDelta = new Vector2(300, 84);
        }

        // ------------------------------------------------------------- widgets
        private void AddCheckbox(Transform parent, string label, float y, bool initial,
            Action<bool> onToggle)
        {
            // Row container.
            var rowGo = new GameObject($"Row_{label}");
            rowGo.transform.SetParent(parent, false);
            var rrt = rowGo.AddComponent<RectTransform>();
            rrt.anchorMin = rrt.anchorMax = new Vector2(0.5f, 0.5f);
            rrt.pivot = new Vector2(0.5f, 0.5f);
            rrt.anchoredPosition = new Vector2(0, y);
            rrt.sizeDelta = new Vector2(640, 64);

            // The box.
            var box = AddImage(rowGo.transform, new Color(0.20f, 0.23f, 0.30f));
            var brt = box.rectTransform;
            brt.anchorMin = brt.anchorMax = new Vector2(0f, 0.5f);
            brt.pivot = new Vector2(0f, 0.5f);
            brt.anchoredPosition = new Vector2(0, 0);
            brt.sizeDelta = new Vector2(56, 56);

            // The tick (checkmark), shown when on.
            var tick = MakeText(box.transform, "✔", 48, new Color(0.4f, 1f, 0.6f),
                Vector2.zero, TextAnchor.MiddleCenter, FontStyle.Bold);
            Stretch(tick.rectTransform);
            tick.gameObject.SetActive(initial);

            // The label.
            var text = MakeText(rowGo.transform, label, 36, Color.white,
                new Vector2(80, 0), TextAnchor.MiddleLeft, FontStyle.Normal);
            text.rectTransform.anchorMin = text.rectTransform.anchorMax = new Vector2(0f, 0.5f);
            text.rectTransform.pivot = new Vector2(0f, 0.5f);
            text.rectTransform.sizeDelta = new Vector2(540, 60);

            // Whole row is clickable.
            var btn = rowGo.AddComponent<Button>();
            var img = rowGo.AddComponent<Image>();       // transparent hit area
            img.color = new Color(1, 1, 1, 0.001f);
            bool state = initial;
            btn.onClick.AddListener(() =>
            {
                state = !state;
                tick.gameObject.SetActive(state);
                onToggle(state);
                AudioManager.Play(SfxLibrary.UiClick);
            });
        }

        private void Close()
        {
            AudioManager.Play(SfxLibrary.UiClick);
            _onClose?.Invoke();
            Destroy(gameObject);
        }

        // ---------------------------------------------------------- UI helpers
        private Image AddImage(Transform parent, Color color)
        {
            var go = new GameObject("Image");
            go.transform.SetParent(parent, false);
            var img = go.AddComponent<Image>();
            img.color = color;
            return img;
        }

        private static void Stretch(RectTransform rt)
        {
            rt.anchorMin = Vector2.zero; rt.anchorMax = Vector2.one;
            rt.offsetMin = Vector2.zero; rt.offsetMax = Vector2.zero;
        }

        private Text MakeText(Transform parent, string content, int size, Color color,
            Vector2 pos, TextAnchor align, FontStyle style)
        {
            var go = new GameObject("Text");
            go.transform.SetParent(parent, false);
            var txt = go.AddComponent<Text>();
            txt.font = _font;
            txt.text = content;
            txt.fontSize = size;
            txt.fontStyle = style;
            txt.alignment = align;
            txt.color = color;
            txt.horizontalOverflow = HorizontalWrapMode.Overflow;
            txt.verticalOverflow = VerticalWrapMode.Overflow;
            var rt = txt.rectTransform;
            rt.anchorMin = rt.anchorMax = new Vector2(0.5f, 0.5f);
            rt.pivot = new Vector2(0.5f, 0.5f);
            rt.anchoredPosition = pos;
            rt.sizeDelta = new Vector2(400, 60);
            return txt;
        }

        private Button MakeButton(Transform parent, string label, Vector2 pos,
            Color color, UnityEngine.Events.UnityAction onClick)
        {
            var go = new GameObject($"Button_{label}");
            go.transform.SetParent(parent, false);
            var img = go.AddComponent<Image>();
            img.color = color;
            var rt = img.rectTransform;
            rt.anchorMin = rt.anchorMax = new Vector2(0.5f, 0.5f);
            rt.pivot = new Vector2(0.5f, 0.5f);
            rt.anchoredPosition = pos;
            rt.sizeDelta = new Vector2(300, 84);

            var btn = go.AddComponent<Button>();
            var colors = btn.colors;
            colors.highlightedColor = Color.Lerp(color, Color.white, 0.25f);
            colors.pressedColor = Color.Lerp(color, Color.black, 0.2f);
            btn.colors = colors;
            btn.onClick.AddListener(onClick);

            var txt = MakeText(go.transform, label, 40, Color.white, Vector2.zero,
                TextAnchor.MiddleCenter, FontStyle.Bold);
            txt.rectTransform.sizeDelta = rt.sizeDelta;
            return btn;
        }
    }
}
