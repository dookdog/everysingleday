#if UNITY_EDITOR
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;
using EverySingleDay.Systems;

namespace EverySingleDay.EditorTools
{
    /// <summary>
    /// Editor convenience: generates a valid playable scene via Unity's own API
    /// (so the YAML is always correct) and registers it in Build Settings. Run
    /// once from the menu after opening the project, or just press Play in any
    /// scene — <see cref="GameBootstrap"/> auto-spawns either way.
    /// </summary>
    public static class SceneBuilder
    {
        private const string ScenePath = "Assets/Scenes/Main.unity";

        [MenuItem("Every Single Day/Create Playable Scene")]
        public static void CreatePlayableScene()
        {
            var scene = EditorSceneManager.NewScene(NewSceneSetup.DefaultGameObjects,
                NewSceneMode.Single);

            var bootstrap = new GameObject("Bootstrap");
            bootstrap.AddComponent<GameBootstrap>();

            System.IO.Directory.CreateDirectory("Assets/Scenes");
            EditorSceneManager.SaveScene(scene, ScenePath);
            AddSceneToBuildSettings(ScenePath);

            Debug.Log("[EverySingleDay] Created and saved playable scene at " + ScenePath +
                      ". Press Play to start!");
        }

        [MenuItem("Every Single Day/Open Playable Scene")]
        public static void OpenPlayableScene()
        {
            if (System.IO.File.Exists(ScenePath))
                EditorSceneManager.OpenScene(ScenePath);
            else
                CreatePlayableScene();
        }

        private static void AddSceneToBuildSettings(string path)
        {
            var scenes = new System.Collections.Generic.List<EditorBuildSettingsScene>(
                EditorBuildSettings.scenes);
            if (!scenes.Exists(s => s.path == path))
                scenes.Insert(0, new EditorBuildSettingsScene(path, true));
            EditorBuildSettings.scenes = scenes.ToArray();
        }
    }
}
#endif
