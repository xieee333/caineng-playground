using System.Collections.Generic;
using Caineng.Playground.Core;
using Caineng.Playground.Surfaces;
using UnityEngine;

namespace Caineng.Playground.Gameplay
{
    [RequireComponent(typeof(PlayerMotor))]
    public sealed class BloomAbility : MonoBehaviour
    {
        [SerializeField] private float radius = 3.5f;
        [SerializeField] private float activeDuration = 0.8f;
        [SerializeField] private float cooldown = 5f;
        [SerializeField] private SurfaceKind bloomSurface = SurfaceKind.Bounce;

        private readonly Dictionary<TraversalSurface, SurfaceKind> affectedSurfaces = new Dictionary<TraversalSurface, SurfaceKind>();
        private float activeRemaining;
        private GameObject bloomVisual;

        public bool IsActive { get; private set; }
        public float CooldownRemaining { get; private set; }
        public int LastAffectedSurfaceCount { get; private set; }
        public float Radius => radius;

        private void Update()
        {
            if (CooldownRemaining > 0f) CooldownRemaining = Mathf.Max(0f, CooldownRemaining - Time.deltaTime);
            if (!IsActive) return;

            activeRemaining -= Time.deltaTime;
            AnimateBloomVisual();
            if (activeRemaining <= 0f) EndBloom();
        }

        public bool TryActivate()
        {
            if (IsActive || CooldownRemaining > 0f) return false;

            Physics.SyncTransforms();
            IsActive = true;
            activeRemaining = Mathf.Max(0.05f, activeDuration);
            CooldownRemaining = Mathf.Max(0f, cooldown);
            LastAffectedSurfaceCount = 0;
            affectedSurfaces.Clear();

            var colliders = Physics.OverlapSphere(transform.position, Mathf.Max(0.1f, radius), Physics.DefaultRaycastLayers, QueryTriggerInteraction.Ignore);
            foreach (var collider in colliders)
            {
                var surface = collider.GetComponentInParent<TraversalSurface>();
                if (surface == null || affectedSurfaces.ContainsKey(surface)) continue;

                affectedSurfaces.Add(surface, surface.Kind);
                surface.Configure(bloomSurface);
                LastAffectedSurfaceCount++;
            }

            CreateBloomVisual();
            return true;
        }

        private void EndBloom()
        {
            foreach (var pair in affectedSurfaces)
            {
                if (pair.Key != null) pair.Key.Configure(pair.Value);
            }

            affectedSurfaces.Clear();
            IsActive = false;
            if (bloomVisual != null)
            {
                Destroy(bloomVisual);
                bloomVisual = null;
            }
        }

        private void CreateBloomVisual()
        {
            bloomVisual = new GameObject("Bloom Pulse");
            bloomVisual.transform.SetParent(transform, false);

            for (var i = 0; i < 3; i++)
            {
                var pulse = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
                pulse.name = $"Bloom Pulse {i + 1}";
                pulse.transform.SetParent(bloomVisual.transform, false);
                pulse.transform.localPosition = new Vector3(0f, -0.85f + i * 0.08f, 0f);
                pulse.transform.localScale = new Vector3(0.35f, 0.02f, 0.35f);

                var collider = pulse.GetComponent<Collider>();
                if (collider != null) Destroy(collider);

                var renderer = pulse.GetComponent<Renderer>();
                var shader = Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Standard");
                if (renderer != null && shader != null)
                {
                    renderer.material = new Material(shader) { color = new Color(1f, 0.75f, 0.1f) };
                }
            }
        }

        private void AnimateBloomVisual()
        {
            if (bloomVisual == null) return;

            var progress = 1f - activeRemaining / Mathf.Max(0.05f, activeDuration);
            for (var i = 0; i < bloomVisual.transform.childCount; i++)
            {
                var pulse = bloomVisual.transform.GetChild(i);
                var scale = 0.35f + progress * (0.8f + i * 0.25f);
                pulse.localScale = new Vector3(scale, 0.02f, scale);
                pulse.localPosition = new Vector3(0f, -0.85f + i * 0.08f, 0f);
            }
        }

        private void OnDestroy()
        {
            foreach (var pair in affectedSurfaces)
            {
                if (pair.Key != null) pair.Key.Configure(pair.Value);
            }
        }
    }
}
