using UnityEngine;

namespace Caineng.Playground.Gameplay
{
    public sealed class GrayboxArenaBootstrap : MonoBehaviour
    {
        private void Awake()
        {
            if (GameObject.Find("Graybox Arena") != null) return;
            var root = new GameObject("Graybox Arena");
            CreateBlock(root.transform, "Cyan Platform", new Vector3(-5f, 0.35f, 3f), new Vector3(5f, 0.7f, 4f), new Color(0.1f, 0.85f, 1f));
            CreateBlock(root.transform, "Magenta Platform", new Vector3(5f, 1.1f, 5f), new Vector3(4f, 0.7f, 4f), new Color(1f, 0.15f, 0.65f));
            CreateBlock(root.transform, "Yellow Ramp", new Vector3(0f, 0.8f, 7f), new Vector3(4f, 0.5f, 6f), new Color(1f, 0.75f, 0.1f), new Vector3(15f, 0f, 0f));
            CreateBlock(root.transform, "North Wall", new Vector3(0f, 2f, 9f), new Vector3(18f, 4f, 0.5f), new Color(0.12f, 0.15f, 0.25f));
            CreateBlock(root.transform, "West Wall", new Vector3(-9f, 2f, 0f), new Vector3(0.5f, 4f, 18f), new Color(0.12f, 0.15f, 0.25f));
            CreateBlock(root.transform, "East Wall", new Vector3(9f, 2f, 0f), new Vector3(0.5f, 4f, 18f), new Color(0.12f, 0.15f, 0.25f));
            var player = GameObject.FindWithTag("Player");
            var respawn = player != null ? player.GetComponent<BubbleRespawnController>() : null;
            if (respawn != null) respawn.Configure(new Vector3(0f, 1f, 0f));
        }

        private static void CreateBlock(Transform parent, string name, Vector3 position, Vector3 scale, Color color, Vector3? rotation = null)
        {
            var block = GameObject.CreatePrimitive(PrimitiveType.Cube);
            block.name = name;
            block.transform.SetParent(parent);
            block.transform.position = position;
            block.transform.localScale = scale;
            block.transform.eulerAngles = rotation ?? Vector3.zero;
            var renderer = block.GetComponent<Renderer>();
            var shader = Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Standard");
            if (renderer != null && shader != null) renderer.material = new Material(shader) { color = color };
        }
    }
}
