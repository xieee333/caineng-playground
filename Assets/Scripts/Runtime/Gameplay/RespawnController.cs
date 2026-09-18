using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Caineng.Playground.Gameplay
{
    [RequireComponent(typeof(PlayerMotor))]
    public sealed class RespawnController : MonoBehaviour
    {
        [SerializeField] private float respawnDelay = 3f;
        [SerializeField] private Transform respawnPoint;
        [SerializeField] private float fallThreshold = -6f;

        private readonly List<Transform> rewindLayers = new List<Transform>();
        private Renderer[] playerRenderers;
        private PlayerMotor motor;
        private Vector3 initialSpawnPoint;
        private Coroutine respawnRoutine;
        private GameObject rewindVisual;

        public bool IsRespawning { get; private set; }
        public float RespawnDelay => respawnDelay;
        public float RespawnRemaining { get; private set; }

        private void Awake()
        {
            motor = GetComponent<PlayerMotor>();
            initialSpawnPoint = transform.position;
            playerRenderers = GetComponentsInChildren<Renderer>();
        }

        private void Update()
        {
            if (IsRespawning)
            {
                RespawnRemaining = Mathf.Max(0f, RespawnRemaining - Time.deltaTime);
                AnimateRewindVisual();
                return;
            }

            if (transform.position.y < fallThreshold) BeginRespawn();
        }

        public void Configure(Vector3 spawnPoint, float delay = 3f)
        {
            initialSpawnPoint = spawnPoint;
            respawnDelay = Mathf.Max(0f, delay);
        }

        public void BeginRespawn()
        {
            if (IsRespawning) return;

            IsRespawning = true;
            RespawnRemaining = respawnDelay;
            motor.SetRespawnState(true);
            CreateRewindVisual();
            respawnRoutine = StartCoroutine(RespawnAfterDelay());
        }

        public void RespawnNow()
        {
            if (respawnRoutine != null)
            {
                StopCoroutine(respawnRoutine);
                respawnRoutine = null;
            }

            CompleteRespawn();
        }

        private IEnumerator RespawnAfterDelay()
        {
            yield return new WaitForSeconds(respawnDelay);
            respawnRoutine = null;
            CompleteRespawn();
        }

        private void CompleteRespawn()
        {
            motor.Teleport(respawnPoint != null ? respawnPoint.position : initialSpawnPoint);
            motor.SetRespawnState(false);
            IsRespawning = false;
            RespawnRemaining = 0f;
            SetPlayerRenderersEnabled(true);

            if (rewindVisual != null)
            {
                Destroy(rewindVisual);
                rewindVisual = null;
            }

            rewindLayers.Clear();
        }

        private void CreateRewindVisual()
        {
            rewindVisual = new GameObject("Color Rewind Visual");
            rewindVisual.transform.SetParent(transform, false);
            SetPlayerRenderersEnabled(false);

            CreateRewindLayer("Cyan Echo", new Color(0.1f, 0.85f, 1f), -0.36f, 0f);
            CreateRewindLayer("Magenta Echo", new Color(1f, 0.15f, 0.65f), 0f, 18f);
            CreateRewindLayer("Yellow Echo", new Color(1f, 0.75f, 0.1f), 0.36f, -18f);
        }

        private void CreateRewindLayer(string layerName, Color color, float x, float angle)
        {
            var layer = GameObject.CreatePrimitive(PrimitiveType.Cube);
            layer.name = layerName;
            layer.transform.SetParent(rewindVisual.transform, false);
            layer.transform.localPosition = new Vector3(x, 0.1f, 0f);
            layer.transform.localScale = new Vector3(0.14f, 1.85f, 0.14f);
            layer.transform.localEulerAngles = new Vector3(0f, angle, 0f);

            var collider = layer.GetComponent<Collider>();
            if (collider != null) Destroy(collider);

            var renderer = layer.GetComponent<Renderer>();
            var shader = Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Standard");
            if (renderer != null && shader != null) renderer.material = new Material(shader) { color = color };

            rewindLayers.Add(layer.transform);
        }

        private void AnimateRewindVisual()
        {
            for (var i = 0; i < rewindLayers.Count; i++)
            {
                var layer = rewindLayers[i];
                if (layer == null) continue;

                var phase = Time.time * 5f + i * 1.7f;
                layer.localPosition = new Vector3((i - 1) * 0.36f + Mathf.Sin(phase) * 0.08f, 0.1f, 0f);
                layer.localScale = new Vector3(0.14f, 1.7f + Mathf.Sin(phase) * 0.14f, 0.14f);
            }
        }

        private void SetPlayerRenderersEnabled(bool enabled)
        {
            if (playerRenderers == null) return;
            foreach (var renderer in playerRenderers)
            {
                if (renderer != null) renderer.enabled = enabled;
            }
        }
    }
}
