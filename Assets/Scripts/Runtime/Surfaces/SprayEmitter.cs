using System.Collections.Generic;
using Caineng.Playground.Core;
using UnityEngine;
using UnityEngine.InputSystem;

namespace Caineng.Playground.Surfaces
{
    public sealed class SprayEmitter : MonoBehaviour
    {
        [SerializeField] private Camera aimCamera;
        [SerializeField] private SurfaceKind sprayKind = SurfaceKind.Liquid;
        [SerializeField] private float maxRange = 40f;
        [SerializeField] private float cooldown = 0.08f;
        [SerializeField] private LayerMask hitMask = Physics.DefaultRaycastLayers;
        [SerializeField] private int gridWidth = 18;
        [SerializeField] private int gridDepth = 18;
        [SerializeField] private float cellSize = 1f;
        [SerializeField] private Vector3 gridOrigin = new Vector3(-9f, 0f, -9f);
        [SerializeField] private int maxVisualMarks = 120;
        [SerializeField] private float markSize = 0.16f;

        private readonly List<GameObject> visualMarks = new List<GameObject>();
        private PaintableSurfaceGrid grid;
        private SprayTool tool;
        private float cooldownRemaining;

        public PaintableSurfaceGrid Grid => grid;
        public SurfaceKind SprayKind => sprayKind;
        public int SprayCount { get; private set; }
        public Vector3 LastSprayPoint { get; private set; }
        public bool HasSprayed { get; private set; }

        private void Awake()
        {
            Initialize();
        }

        private void Update()
        {
            if (cooldownRemaining > 0f) cooldownRemaining -= Time.deltaTime;

            var mouse = Mouse.current;
            if (mouse == null || !mouse.leftButton.isPressed) return;

            TrySprayFromScreen(mouse.position.ReadValue());
        }

        public void SetAimCamera(Camera camera)
        {
            aimCamera = camera;
        }

        public void SetHitMask(LayerMask mask)
        {
            hitMask = mask;
        }

        public bool TrySprayFromScreen(Vector2 screenPoint)
        {
            if (aimCamera == null) aimCamera = Camera.main;
            if (aimCamera == null) return false;

            var ray = aimCamera.ScreenPointToRay(screenPoint);
            return TrySprayFromRay(ray);
        }

        public bool TrySprayFromRay(Ray ray)
        {
            if (cooldownRemaining > 0f) return false;
            if (!Physics.Raycast(ray, out var hit, maxRange, hitMask, QueryTriggerInteraction.Ignore)) return false;
            return SprayAt(hit);
        }

        public bool SprayAt(Vector3 worldPoint)
        {
            return ApplySpray(worldPoint, Vector3.up, null);
        }

        private bool SprayAt(RaycastHit hit)
        {
            var surface = hit.collider != null ? hit.collider.GetComponentInParent<TraversalSurface>() : null;
            return ApplySpray(hit.point, hit.normal, surface);
        }

        private bool ApplySpray(Vector3 worldPoint, Vector3 normal, TraversalSurface surface)
        {
            Initialize();
            if (cooldownRemaining > 0f) return false;
            if (!tool.Spray(worldPoint, Time.time)) return false;

            cooldownRemaining = Mathf.Max(0f, cooldown);
            LastSprayPoint = worldPoint;
            HasSprayed = true;
            SprayCount++;

            if (surface != null) surface.Configure(sprayKind);
            CreateVisualMark(worldPoint, normal);
            return true;
        }

        private void Initialize()
        {
            if (grid != null) return;

            grid = new PaintableSurfaceGrid(
                Mathf.Max(1, gridWidth),
                Mathf.Max(1, gridDepth),
                Mathf.Max(0.01f, cellSize),
                gridOrigin);
            tool = new SprayTool(grid, sprayKind);
        }

        private void CreateVisualMark(Vector3 point, Vector3 normal)
        {
            while (visualMarks.Count >= Mathf.Max(1, maxVisualMarks))
            {
                var oldest = visualMarks[0];
                visualMarks.RemoveAt(0);
                if (oldest != null) Destroy(oldest);
            }

            var mark = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            mark.name = "Spray Mark";
            mark.transform.SetParent(transform);
            mark.transform.position = point + normal.normalized * 0.03f;
            mark.transform.localScale = Vector3.one * Mathf.Max(0.02f, markSize);

            var collider = mark.GetComponent<Collider>();
            if (collider != null) Destroy(collider);

            var renderer = mark.GetComponent<Renderer>();
            var shader = Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Standard");
            if (renderer != null && shader != null)
            {
                renderer.material = new Material(shader) { color = ColorFor(sprayKind) };
            }

            visualMarks.Add(mark);
        }

        private static Color ColorFor(SurfaceKind kind)
        {
            switch (kind)
            {
                case SurfaceKind.Liquid: return new Color(0.1f, 0.85f, 1f);
                case SurfaceKind.Bounce: return new Color(1f, 0.75f, 0.1f);
                case SurfaceKind.Float: return new Color(1f, 0.15f, 0.65f);
                default: return Color.white;
            }
        }
    }
}
