#if UNITY_EDITOR
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;
using EverySingleDay.Systems;
using EverySingleDay.UI;

namespace EverySingleDay.EditorTools
{
    /// <summary>
    /// Editor convenience: generates valid playable scenes via Unity's own API
    /// (so the YAML is always correct) and registers them in Build Settings,
    /// with the main menu first so the game boots to the title screen. Run once
    /// from the menu after opening the project, or just press Play in any
    /// scene — the bootstraps auto-spawn either way.
    /// </summary>
    public static class SceneBuilder
    {
        private const string GameScenePath = "Assets/Scenes/Main.unity";
        private const string MenuScenePath = "Assets/Scenes/MainMenu.unity";

        [MenuItem("Every Single Day/Create All Scenes (Menu + Game)")]
        public static void CreateAllScenes()
        {
            CreateMenuSceneAsset();
            CreateGameSceneAsset();
            // Build order: menu first (index 0), then gameplay (index 1).
            SetBuildSettings(MenuScenePath, GameScenePath);
            Debug.Log("[EverySingleDay] Created MainMenu + Main scenes and set " +
                      "build order. Press Play (or build) to start at the title screen!");
        }

        [MenuItem("Every Single Day/Create Playable Scene (Game Only)")]
        public static void CreatePlayableScene()
        {
            CreateGameSceneAsset();
            AddSceneToBuildSettings(GameScenePath);
            Debug.Log("[EverySingleDay] Created and saved playable scene at " +
                      GameScenePath + ". Press Play to start!");
        }

        [MenuItem("Every Single Day/Open Main Menu Scene")]
        public static void OpenMenuScene()
        {
            if (!System.IO.File.Exists(MenuScenePath)) CreateMenuSceneAsset();
            EditorSceneManager.OpenScene(MenuScenePath);
        }

        [MenuItem("Every Single Day/Open Playable Scene")]
        public static void OpenPlayableScene()
        {
            if (!System.IO.File.Exists(GameScenePath)) CreateGameSceneAsset();
            EditorSceneManager.OpenScene(GameScenePath);
        }

        // ----------------------------------------------------------- creation
        private static void CreateGameSceneAsset()
        {
            var scene = EditorSceneManager.NewScene(NewSceneSetup.DefaultGameObjects,
                NewSceneMode.Single);
            new GameObject("Bootstrap").AddComponent<GameBootstrap>();
            System.IO.Directory.CreateDirectory("Assets/Scenes");
            EditorSceneManager.SaveScene(scene, GameScenePath);
        }

        private static void CreateMenuSceneAsset()
        {
            var scene = EditorSceneManager.NewScene(NewSceneSetup.DefaultGameObjects,
                NewSceneMode.Single);
            var go = new GameObject("MainMenu");
            var menu = go.AddComponent<MainMenuBootstrap>();
            menu.gameplaySceneName = "Main";
            System.IO.Directory.CreateDirectory("Assets/Scenes");
            EditorSceneManager.SaveScene(scene, MenuScenePath);
        }

        // ------------------------------------------------------- build settings
        private static void AddSceneToBuildSettings(string path)
        {
            var scenes = new System.Collections.Generic.List<EditorBuildSettingsScene>(
                EditorBuildSettings.scenes);
            if (!scenes.Exists(s => s.path == path))
                scenes.Insert(0, new EditorBuildSettingsScene(path, true));
            EditorBuildSettings.scenes = scenes.ToArray();
        }

        private static void SetBuildSettings(params string[] orderedPaths)
        {
            var list = new System.Collections.Generic.List<EditorBuildSettingsScene>();
            foreach (var p in orderedPaths)
                list.Add(new EditorBuildSettingsScene(p, true));
            EditorBuildSettings.scenes = list.ToArray();
        }
    }
}
#endif
