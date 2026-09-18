using System;
using Caineng.Playground.Core;
using Caineng.Playground.Surfaces;
using UnityEngine;

namespace Caineng.Playground.Gameplay
{
    public enum PlayerMovementState { Grounded, Airborne, Submerged, Recalling }

    [RequireComponent(typeof(CharacterController))]
    public sealed class PlayerMotor : MonoBehaviour
    {
        [SerializeField] private float moveSpeed = 5f;
        [SerializeField] private float gravity = -22f;
        [SerializeField] private float jumpHeight = 1.5f;
        [SerializeField] private SurfaceKind currentSurface;

        private readonly ColorTraversalMotor traversal = new ColorTraversalMotor();
        private CharacterController controller;
        private Vector2 moveInput;
        private float verticalVelocity;
        private bool jumpRequested;
        private bool respawnLocked;
        private bool surfaceOverride;

        public event Action<PlayerMovementState, PlayerMovementState> StateChanged;
        public PlayerMovementState CurrentState { get; private set; } = PlayerMovementState.Airborne;
        public Vector3 Velocity { get; private set; }
        public float BaseMoveSpeed => moveSpeed;
        public SurfaceKind CurrentSurface => currentSurface;

        private void Awake() => controller = GetComponent<CharacterController>();
        private void Update() => Simulate(Time.deltaTime);

        public void SetMoveInput(Vector2 input) => moveInput = Vector2.ClampMagnitude(input, 1f);
        public void RequestJump() => jumpRequested = true;
        public void SetSurface(SurfaceKind surface)
        {
            currentSurface = surface;
            surfaceOverride = true;
        }

        public void ClearSurfaceOverride()
        {
            surfaceOverride = false;
        }

        public void SetRespawnState(bool isRespawning)
        {
            respawnLocked = isRespawning;
            moveInput = Vector2.zero;
            verticalVelocity = 0f;
            SetState(isRespawning ? PlayerMovementState.Recalling : PlayerMovementState.Airborne);
        }

        public void Teleport(Vector3 position)
        {
            var wasEnabled = controller.enabled;
            controller.enabled = false;
            transform.position = position;
            controller.enabled = wasEnabled;
            verticalVelocity = 0f;
        }

        private void Simulate(float deltaTime)
        {
            if (respawnLocked)
            {
                Velocity = Vector3.zero;
                SetState(PlayerMovementState.Recalling);
                jumpRequested = false;
                return;
            }

            if (!surfaceOverride) DetectSurfaceBelow();
            var result = traversal.Resolve(currentSurface, moveSpeed);
            var planar = new Vector3(moveInput.x, 0f, moveInput.y) * result.SpeedMultiplier;
            var grounded = controller.isGrounded;

            if (currentSurface == SurfaceKind.Liquid && result.CanSubmerge)
            {
                verticalVelocity = Mathf.MoveTowards(verticalVelocity, -1f, 8f * deltaTime);
                SetState(PlayerMovementState.Submerged);
            }
            else
            {
                if (grounded && currentSurface == SurfaceKind.Bounce)
                {
                    verticalVelocity = Mathf.Sqrt(jumpHeight * -2f * gravity) * 1.25f;
                    grounded = false;
                }
                else if (grounded && verticalVelocity < 0f)
                {
                    verticalVelocity = currentSurface == SurfaceKind.Float ? -0.35f : -2f;
                }
                if (grounded && jumpRequested)
                {
                    verticalVelocity = Mathf.Sqrt(jumpHeight * -2f * gravity);
                    grounded = false;
                }
                verticalVelocity += gravity * result.GravityScale * deltaTime;
                SetState(grounded ? PlayerMovementState.Grounded : PlayerMovementState.Airborne);
            }

            jumpRequested = false;
            Velocity = new Vector3(planar.x, verticalVelocity, planar.z);
            controller.Move(Velocity * deltaTime);
        }

        private void DetectSurfaceBelow()
        {
            var origin = transform.position + Vector3.up * 0.2f;
            var distance = controller.height * 0.75f + 0.5f;
            if (Physics.Raycast(origin, Vector3.down, out var hit, distance, Physics.DefaultRaycastLayers, QueryTriggerInteraction.Ignore))
            {
                var surface = hit.collider.GetComponentInParent<TraversalSurface>();
                currentSurface = surface != null ? surface.Kind : SurfaceKind.None;
            }
            else
            {
                currentSurface = SurfaceKind.None;
            }
        }

        private void SetState(PlayerMovementState next)
        {
            if (CurrentState == next) return;
            var previous = CurrentState;
            CurrentState = next;
            StateChanged?.Invoke(previous, next);
        }
    }
}
