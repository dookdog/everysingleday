using System;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using EverySingleDay.Systems;

namespace EverySingleDay.UI
{
    /// <summary>
    /// On-screen touch controls built in code: a left/right D-pad, a jump button
    /// and a pause button, laid over the game on a screen-space canvas. They feed
    /// <see cref="GameInput"/> so they drive the exact same PlayerController the
    /// keyboard does — this is the input layer an iOS/Android build needs.
    ///
    /// By default it only appears on touch devices; set <see cref="forceShow"/>
    /// (or pass force:true) to test it with a mouse in the editor.
    /// </summary>
    public class TouchControls : MonoBehaviour
    {
        public static bool forceShow = false;

        public static TouchControls Create(bool force = false)
        {
            if (!force && !forceShow && !Application.isMobilePlatform && !Input.touchSupported)
                return null;

            var go = new GameObject("TouchControls");
            var canvas = go.AddComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            canvas.sortingOrder = 30; // above the HUD
            var scaler = go.AddComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1280, 720);
            scaler.matchWidthOrHeight = 0.5f;
            go.AddComponent<GraphicRaycaster>();

            var tc = go.AddComponent<TouchControls>();
            tc.Build(canvas);
            return tc;
        }

        private Font _font;

        private void Build(Canvas canvas)
        {
            _font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            if (_font == null) _font = Resources.GetBuiltinResource<Font>("Arial.ttf");

            // Left cluster: ◀ ▶
            MakeHoldButton(canvas.transform, "◀", new Vector2(130, 130),
                new Vector2(0, 0), new Vector2(0, 0),
                down => GameInput.SetLeft(down));
            MakeHoldButton(canvas.transform, "▶", new Vector2(130, 130),
                new Vector2(285, 0), new Vector2(0, 0),
                down => GameInput.SetRight(down));

            // Right: big JUMP
            MakeHoldButton(canvas.transform, "JUMP", new Vector2(190, 190),
                new Vector2(-70, 20), new Vector2(1, 0),
                down => GameInput.SetJump(down), isJump: true);

            // Top-right: pause
            MakeTapButton(canvas.transform, "II", new Vector2(96, 96),
                new Vector2(-40, -40), new Vector2(1, 1),
                () => GameManager.Instance?.TogglePause());
        }

        private GameObject MakeHoldButton(Transform parent, string label, Vector2 size,
            Vector2 anchoredPos, Vector2 anchor, Action<bool> onHold, bool isJump = false)
        {
            var go = NewButton(parent, label, size, anchoredPos, anchor, isJump);
            var hold = go.AddComponent<HoldButton>();
            hold.onStateChanged = onHold;
            return go;
        }

        private GameObject MakeTapButton(Transform parent, string label, Vector2 size,
            Vector2 anchoredPos, Vector2 anchor, Action onTap)
        {
            var go = NewButton(parent, label, size, anchoredPos, anchor, false);
            var hold = go.AddComponent<HoldButton>();
            hold.onStateChanged = down => { if (down) onTap?.Invoke(); };
            return go;
        }

        private GameObject NewButton(Transform parent, string label, Vector2 size,
            Vector2 anchoredPos, Vector2 anchor, bool isJump)
        {
            var go = new GameObject($"Btn_{label}");
            go.transform.SetParent(parent, false);

            var img = go.AddComponent<Image>();
            img.color = isJump ? new Color(0.44f, 0.89f, 1f, 0.16f)
                               : new Color(0.12f, 0.16f, 0.25f, 0.42f);
            var rt = img.rectTransform;
            rt.anchorMin = rt.anchorMax = anchor;
            rt.pivot = new Vector2(0.5f, 0.5f);
            rt.sizeDelta = size;
            // offset from the chosen screen corner, with margin
            float mx = 40f + size.x / 2f, my = 40f + size.y / 2f;
            rt.anchoredPosition = new Vector2(
                anchoredPos.x + (anchor.x < 0.5f ? mx : 0f),
                anchoredPos.y + (anchor.y < 0.5f ? my : 0f));

            var txt = new GameObject("t").AddComponent<Text>();
            txt.transform.SetParent(go.transform, false);
            txt.font = _font;
            txt.text = label;
            txt.alignment = TextAnchor.MiddleCenter;
            txt.fontStyle = FontStyle.Bold;
            txt.fontSize = isJump ? 40 : 56;
            txt.color = isJump ? new Color(0.6f, 0.95f, 1f) : new Color(1, 1, 1, 0.85f);
            var trt = txt.rectTransform;
            trt.anchorMin = Vector2.zero; trt.anchorMax = Vector2.one;
            trt.offsetMin = trt.offsetMax = Vector2.zero;

            return go;
        }

        private void OnDisable() => GameInput.Clear();
    }

    /// <summary>
    /// Reports press / release (including pointer-exit while held) as a bool to a
    /// callback. Used by every on-screen control so holding ◀ or JUMP works like
    /// a physical button.
    /// </summary>
    public class HoldButton : MonoBehaviour, IPointerDownHandler, IPointerUpHandler, IPointerExitHandler
    {
        public Action<bool> onStateChanged;
        private bool _down;

        public void OnPointerDown(PointerEventData e) => Set(true);
        public void OnPointerUp(PointerEventData e) => Set(false);
        public void OnPointerExit(PointerEventData e) { if (_down) Set(false); }

        private void Set(bool v)
        {
            if (_down == v) return;
            _down = v;
            onStateChanged?.Invoke(v);
        }

        private void OnDisable() { if (_down) Set(false); }
    }
}
