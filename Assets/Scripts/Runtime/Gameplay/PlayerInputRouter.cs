using UnityEngine;
using UnityEngine.InputSystem;

namespace Caineng.Playground.Gameplay
{
    [RequireComponent(typeof(PlayerMotor))]
    public sealed class PlayerInputRouter : MonoBehaviour
    {
        private PlayerMotor motor;
        private void Awake() => motor = GetComponent<PlayerMotor>();

        private void Update()
        {
            var keyboard = Keyboard.current;
            if (keyboard == null) { motor.SetMoveInput(Vector2.zero); return; }
            var input = Vector2.zero;
            if (keyboard.wKey.isPressed) input.y += 1f;
            if (keyboard.sKey.isPressed) input.y -= 1f;
            if (keyboard.dKey.isPressed) input.x += 1f;
            if (keyboard.aKey.isPressed) input.x -= 1f;
            motor.SetMoveInput(input);
            if (keyboard.spaceKey.wasPressedThisFrame) motor.RequestJump();
        }
    }
}
