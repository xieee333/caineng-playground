using System.Collections;
using UnityEngine;

namespace Caineng.Playground.Gameplay
{
    [RequireComponent(typeof(PlayerMotor))]
    public sealed class BubbleRespawnController : MonoBehaviour
    {
        [SerializeField] private float respawnDelay = 3f;
        [SerializeField] private Transform respawnPoint;
        [SerializeField] private float fallThreshold = -6f;
        private PlayerMotor motor;
        private Vector3 initialSpawnPoint;
        private Coroutine respawnRoutine;
        private GameObject bubbleVisual;

        public bool IsBubble { get; private set; }
        public float RespawnDelay => respawnDelay;
        private void Awake() { motor = GetComponent<PlayerMotor>(); initialSpawnPoint = transform.position; }
        private void Update() { if (!IsBubble && transform.position.y < fallThreshold) Bubbleize(); }
        public void Configure(Vector3 spawnPoint, float delay = 3f) { initialSpawnPoint = spawnPoint; respawnDelay = Mathf.Max(0f, delay); }

        public void Bubbleize()
        {
            if (IsBubble) return;
            IsBubble = true;
            motor.SetBubbleState(true);
            CreateBubbleVisual();
            respawnRoutine = StartCoroutine(RespawnAfterDelay());
        }

        public void RespawnNow()
        {
            if (respawnRoutine != null) { StopCoroutine(respawnRoutine); respawnRoutine = null; }
            CompleteRespawn();
        }

        private IEnumerator RespawnAfterDelay() { yield return new WaitForSeconds(respawnDelay); respawnRoutine = null; CompleteRespawn(); }

        private void CompleteRespawn()
        {
            motor.Teleport(respawnPoint != null ? respawnPoint.position : initialSpawnPoint);
            motor.SetBubbleState(false);
            IsBubble = false;
            if (bubbleVisual != null) { Destroy(bubbleVisual); bubbleVisual = null; }
        }

        private void CreateBubbleVisual()
        {
            bubbleVisual = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            bubbleVisual.name = "Bubble Visual";
            bubbleVisual.transform.SetParent(transform, false);
            bubbleVisual.transform.localScale = Vector3.one * 1.5f;
            var bubbleCollider = bubbleVisual.GetComponent<Collider>();
            if (bubbleCollider != null) Destroy(bubbleCollider);
        }
    }
}
