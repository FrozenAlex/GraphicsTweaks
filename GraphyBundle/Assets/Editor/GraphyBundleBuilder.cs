using System;
using System.IO;
using System.Linq;
using Newtonsoft.Json.Linq;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEditor.XR.Management;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using UnityEngine.UI;
using UnityEngine.XR.Management;
using UnityEngine.XR.OpenXR;
using Object = UnityEngine.Object;

/// <summary>Build and preview the editable Graphy prefab.</summary>
public static class GraphyBundleBuilder
{
    const string PrefabPath = "Assets/[Graphy] VR.prefab";
    const string NoGlowShaderPath = "Assets/Graphy/Shaders/UINoGlow.shader";
    const string NoGlowMaterialPath = "Assets/Generated/Materials/GraphyUINoGlow.mat";
    static string Root => Path.GetFullPath(Path.Combine(Application.dataPath, ".."));

    // Beat Saber bloom reads framebuffer alpha as glow, so every overlay graphic
    // uses either the no-glow UI shader or a Graphy graph shader.
    static bool NoGlow(Shader shader) =>
        shader != null && (shader.name == "GraphicsTweaks/Graphy UI No Glow" || shader.name.StartsWith("Graphy/Graph"));

    [MenuItem("GraphicsTweaks/Apply no-glow UI materials")]
    public static void ApplyNoGlow()
    {
        var shader = AssetDatabase.LoadAssetAtPath<Shader>(NoGlowShaderPath);
        if (shader == null) throw new FileNotFoundException("Missing no-glow UI shader", NoGlowShaderPath);
        var material = AssetDatabase.LoadAssetAtPath<Material>(NoGlowMaterialPath);
        if (material == null)
        {
            material = new Material(shader) { name = "GraphyUINoGlow" };
            AssetDatabase.CreateAsset(material, NoGlowMaterialPath);
        }
        var root = PrefabUtility.LoadPrefabContents(PrefabPath);
        try
        {
            foreach (var graphic in root.GetComponentsInChildren<Graphic>(true))
            {
                var assigned = new SerializedObject(graphic).FindProperty("m_Material").objectReferenceValue as Material;
                if (assigned == null)
                    graphic.material = material;
                else if (!NoGlow(assigned.shader))
                {
                    // Keep the material's own properties; only its blending changes.
                    if (assigned.shader.name != "UI/Default") throw new Exception($"Unexpected shader {assigned.shader.name} on {graphic.name}");
                    assigned.shader = shader;
                    EditorUtility.SetDirty(assigned);
                }
            }
            PrefabUtility.SaveAsPrefabAsset(root, PrefabPath);
        }
        finally
        {
            PrefabUtility.UnloadPrefabContents(root);
        }
        AssetDatabase.SaveAssets();
    }

    static void Validate(GameObject root)
    {
        foreach (var graphic in root.GetComponentsInChildren<Graphic>(true))
            if (!NoGlow(graphic.material.shader)) throw new Exception($"Glowing UI shader {graphic.material.shader.name} on {graphic.name}");
        if (root.GetComponentsInChildren<Transform>(true).Any(t => GameObjectUtility.GetMonoBehavioursWithMissingScriptCount(t.gameObject) != 0))
            throw new Exception("Missing prefab scripts");
        foreach (var text in root.GetComponentsInChildren<Text>(true))
            if (text.font == null || text.font.material == null) throw new Exception("Missing font/material: " + text.name);
        foreach (var component in root.GetComponentsInChildren<MonoBehaviour>(true).Where(c => c.GetType().Namespace?.StartsWith("Tayx.Graphy") == true))
        {
            var iterator = new SerializedObject(component).GetIterator();
            while (iterator.Next(true))
                if (iterator.propertyType == SerializedPropertyType.ObjectReference && iterator.objectReferenceValue == null
                    && iterator.name != "m_audioListener" && iterator.name != "m_CorrespondingSourceObject"
                    && iterator.name != "m_PrefabInstance" && iterator.name != "m_PrefabAsset")
                    throw new Exception($"Unbound Graphy reference: {component.name}.{iterator.propertyPath}");
        }
    }

    static void Frame(Camera camera, RectTransform rt)
    {
        var corners = new Vector3[4]; rt.GetWorldCorners(corners);
        var center = (corners[0] + corners[2]) * .5f;
        camera.transform.rotation = rt.rotation;
        camera.transform.position = center - rt.forward * 5;
        camera.orthographic = true; camera.orthographicSize = Vector3.Distance(corners[0], corners[1]) * .75f;
        camera.nearClipPlane = .01f; camera.farClipPlane = 100;
    }

    [MenuItem("GraphicsTweaks/Build Android FPS bundle")]
    public static void Build()
    {
        ApplyNoGlow();
        var prefab = AssetDatabase.LoadAssetAtPath<GameObject>(PrefabPath);
        if (prefab == null) throw new FileNotFoundException("Missing editable Graphy prefab", PrefabPath);
        Validate(prefab);
        ConfigurePipeline();
        ConfigureXR();
        PlayerSettings.SetUseDefaultGraphicsAPIs(BuildTarget.Android, false);
        PlayerSettings.SetGraphicsAPIs(BuildTarget.Android, new[] { GraphicsDeviceType.OpenGLES3 });
        string output = Path.Combine(Root, "Build/Android"); Directory.CreateDirectory(output);
        var manifest = BuildPipeline.BuildAssetBundles(output, new[] { new AssetBundleBuild {
            assetBundleName = "fpscounter.bund", assetNames = new[] { PrefabPath }
        } }, BuildAssetBundleOptions.ChunkBasedCompression | BuildAssetBundleOptions.StrictMode
            | BuildAssetBundleOptions.ForceRebuildAssetBundle, BuildTarget.Android);
        if (manifest == null) throw new Exception("Android asset bundle build failed");
        File.WriteAllText(Path.Combine(Root, "Build/build-result.json"), new JObject {
            ["unity"] = Application.unityVersion, ["target"] = "Android", ["graphicsApi"] = "OpenGLES3",
            ["prefab"] = PrefabPath, ["bundle"] = Path.Combine(output, "fpscounter.bund"),
            ["textCount"] = prefab.GetComponentsInChildren<Text>(true).Length,
            ["gpuReferenceBound"] = true, ["graphySource"] = "4.0.0 / 1068f3de2bc9f1e4a905150abd59f31e6dead679"
        }.ToString());
        Debug.Log("GRAPHICS_TWEAKS_BUNDLE_BUILD_SUCCESS");
    }

    static void ConfigurePipeline()
    {
        Directory.CreateDirectory("Assets/Settings");
        const string path = "Assets/Settings/GraphyPipeline.asset";
        var pipeline = AssetDatabase.LoadAssetAtPath<UniversalRenderPipelineAsset>(path);
        if (pipeline == null)
        {
            var renderer = ScriptableObject.CreateInstance<UniversalRendererData>();
            AssetDatabase.CreateAsset(renderer, "Assets/Settings/GraphyRenderer.asset");
            pipeline = UniversalRenderPipelineAsset.Create(renderer);
            pipeline.msaaSampleCount = 1;
            AssetDatabase.CreateAsset(pipeline, path);
        }
        GraphicsSettings.defaultRenderPipeline = pipeline;
        QualitySettings.renderPipeline = pipeline;
        AssetDatabase.SaveAssets();
    }

    static void ConfigureXR()
    {
        const string path = "Assets/Settings/GraphyXRSettings.asset";
        var settings = AssetDatabase.LoadAssetAtPath<XRGeneralSettingsPerBuildTarget>(path);
        if (settings == null)
        {
            settings = ScriptableObject.CreateInstance<XRGeneralSettingsPerBuildTarget>();
            AssetDatabase.CreateAsset(settings, path);
        }
        EditorBuildSettings.AddConfigObject(XRGeneralSettings.k_SettingsKey, settings, true);
        if (!settings.HasManagerSettingsForBuildTarget(BuildTargetGroup.Android))
            settings.CreateDefaultManagerSettingsForBuildTarget(BuildTargetGroup.Android);
        var general = settings.SettingsForBuildTarget(BuildTargetGroup.Android);
        // Needed for shader variant retention; the editor preview does not start XR.
        general.InitManagerOnStart = false;
        if (!general.Manager.activeLoaders.Any(loader => loader is OpenXRLoader))
        {
            var loader = ScriptableObject.CreateInstance<OpenXRLoader>();
            loader.name = "Graphy OpenXR shader build loader";
            AssetDatabase.AddObjectToAsset(loader, settings);
            if (!general.Manager.TryAddLoader(loader)) throw new Exception("Could not configure Android OpenXR loader");
        }
        EditorUtility.SetDirty(settings); EditorUtility.SetDirty(general); EditorUtility.SetDirty(general.Manager);
        AssetDatabase.SaveAssets();
    }

    [MenuItem("GraphicsTweaks/Export layout preview PNG")]
    public static void RenderPreview()
    {
        EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
        var root = (GameObject)PrefabUtility.InstantiatePrefab(AssetDatabase.LoadAssetAtPath<GameObject>(PrefabPath));
        // This is an editor layout preview, with illustrative values rather than device telemetry.
        foreach (var text in root.GetComponentsInChildren<Text>(true))
        {
            if (text.name == "fps_text_value") text.text = "120";
            if (text.name == "ms_text_value") text.text = "6.8";
            if (text.name == "gpu_ms_text_value") text.text = "5.2";
            if (text.name == "avg_fps_value") text.text = "119";
            if (text.name == "1%_fps_value") text.text = "117";
            if (text.name == "0.1%_fps_value") text.text = "115";
            if (text.name.Contains("system_memory_size_text")) text.text = "1024";
            if (text.name == "db_value_text") text.text = "-18";
        }
        var camera = new GameObject("Preview camera", typeof(Camera)).GetComponent<Camera>();
        camera.clearFlags = CameraClearFlags.SolidColor; camera.backgroundColor = new Color(.025f, .028f, .05f);
        Frame(camera, root.GetComponent<RectTransform>());
        GraphicsSettings.defaultRenderPipeline = null; QualitySettings.renderPipeline = null;
        Canvas.ForceUpdateCanvases();
        var target = new RenderTexture(1800, 720, 24);
        camera.targetTexture = target; camera.Render();
        RenderTexture.active = target;
        var image = new Texture2D(target.width, target.height, TextureFormat.RGB24, false);
        image.ReadPixels(new Rect(0, 0, target.width, target.height), 0, 0); image.Apply();
        Directory.CreateDirectory(Path.Combine(Root, "Build"));
        File.WriteAllBytes(Path.Combine(Root, "Build/layout-preview.png"), image.EncodeToPNG());
        RenderTexture.active = null; camera.targetTexture = null;
        Object.DestroyImmediate(image); Object.DestroyImmediate(target);
        Debug.Log("GRAPHICS_TWEAKS_PREVIEW_SUCCESS");
    }
}
