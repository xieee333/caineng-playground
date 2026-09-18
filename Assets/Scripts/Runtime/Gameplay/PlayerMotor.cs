using System;
using Caineng.Playground.Core;
using Caineng.Playground.Surfaces;
using UnityEngine;

namespace Caineng.Playground.Gameplay
{
    public enum PlayerMovementState { Grounded, Airborne, Submerged, Bubble }

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
        private bool bubbleLocked;

        public event Action<PlayerMovementState, PlayerMovementState> StateChanged;
        public PlayerMovementState CurrentState { get; private set; } = PlayerMovementState.Airborne;
        public Vector3 Velocity { get; private set; }
        public float BaseMoveSpeed => moveSpeed;

        private void Awake() => controller = GetComponent<CharacterController>();
        private void Update() => Simulate(Time.deltaTime);

        public void SetMoveInput(Vector2 input) => moveInput = Vector2.ClampMagnitude(input, 1f);
        public void RequestJump() => jumpRequested = true;
        public void SetSurface(SurfaceKind surface) => currentSurface = surface;

        public void SetBubbleState(bool isBubble)
        {
            bubbleLocked = isBubble;
            moveInput = Vector2.zero;
            verticalVelocity = 0f;
            SetState(isBubble ? PlayerMovementState.Bubble : PlayerMovementState.Airborne);
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
            if (bubbleLocked)
            {
                Velocity = Vector3.zero;
                SetState(PlayerMovementState.Bubble);
                jumpRequested = false;
                return;
            }

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
                if (grounded && verticalVelocity < 0f) verticalVelocity = -2f;
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

        private void SetState(PlayerMovementState next)
        {
            if (CurrentState == next) return;
            var previous = CurrentState;
            CurrentState = next;
            StateChanged?.Invoke(previous, next);
        }
    }
}
