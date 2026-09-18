using System.Collections;
using Caineng.Playground.Gameplay;
using Caineng.Playground.Core;
using Caineng.Playground.Surfaces;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.TestTools;

namespace Caineng.Playground.Tests
{
    public sealed class PlayerTraversalPlayModeTests
    {
        [UnityTest]
        public IEnumerator PlayerMotorMovesFromInputCommand()
        {
            var player = CreatePlayer();
            var motor = player.GetComponent<PlayerMotor>();
            var start = player.transform.position;
            motor.SetMoveInput(Vector2.up);
            yield return null;
            yield return null;
            Assert.That(player.transform.position.z, Is.GreaterThan(start.z));
            Object.Destroy(player);
        }

        [UnityTest]
        public IEnumerator RespawnLocksThenReturnsPlayer()
        {
            var player = CreatePlayer();
            var motor = player.GetComponent<PlayerMotor>();
            var respawn = player.AddComponent<RespawnController>();
            respawn.Configure(new Vector3(2f, 1f, 3f), 0.1f);
            respawn.BeginRespawn();
            Assert.That(respawn.IsRespawning, Is.True);
            Assert.That(motor.CurrentState, Is.EqualTo(PlayerMovementState.Recalling));
            Assert.That(respawn.RespawnRemaining, Is.EqualTo(0.1f));
            Assert.That(player.transform.Find("Color Rewind Visual"), Is.Not.Null);
            yield return new WaitForSeconds(0.15f);
            Assert.That(respawn.IsRespawning, Is.False);
            Assert.That(player.activeSelf, Is.True);
            Assert.That(Vector3.Distance(player.transform.position, new Vector3(2f, 1f, 3f)), Is.LessThan(0.02f));
            Object.Destroy(player);
        }

        [Test]
        public void RespawnDefaultRuleIsThreeSeconds()
        {
            var player = CreatePlayer();
            var respawn = player.AddComponent<RespawnController>();
            Assert.That(respawn.RespawnDelay, Is.EqualTo(3f));
            Object.DestroyImmediate(player);
        }

        [UnityTest]
        public IEnumerator MainSceneContainsPlayableGraybox()
        {
            SceneManager.LoadScene("Main", LoadSceneMode.Single);
            yield return null;

            var player = GameObject.FindWithTag("Player");
            Assert.That(player, Is.Not.Null);
            Assert.That(player.GetComponent<PlayerMotor>(), Is.Not.Null);
            Assert.That(player.GetComponent<PlayerInputRouter>(), Is.Not.Null);
            Assert.That(player.GetComponent<RespawnController>(), Is.Not.Null);
            Assert.That(player.GetComponent<SprayEmitter>(), Is.Not.Null);
            Assert.That(player.GetComponent<BloomAbility>(), Is.Not.Null);
            Assert.That(GameObject.Find("Prototype HUD"), Is.Not.Null);
            Assert.That(GameObject.Find("Ground"), Is.Not.Null);
            Assert.That(GameObject.Find("BouncePad"), Is.Not.Null);
            Assert.That(GameObject.Find("Graybox Arena"), Is.Not.Null);
            Assert.That(GameObject.Find("Yellow Ramp"), Is.Not.Null);
            Assert.That(GameObject.Find("Yellow Ramp").GetComponent<TraversalSurface>().Kind, Is.EqualTo(SurfaceKind.Bounce));
        }

        [UnityTest]
        public IEnumerator PlayerDetectsLiquidTraversalSurface()
        {
            var surface = GameObject.CreatePrimitive(PrimitiveType.Cube);
            surface.transform.position = Vector3.zero;
            surface.transform.localScale = new Vector3(4f, 0.5f, 4f);
            surface.AddComponent<TraversalSurface>().Configure(SurfaceKind.Liquid);
            var player = CreatePlayer();
            player.transform.position = new Vector3(0f, 0.8f, 0f);

            yield return null;
            yield return null;

            Assert.That(player.GetComponent<PlayerMotor>().CurrentState, Is.EqualTo(PlayerMovementState.Submerged));
            Object.Destroy(player);
            Object.Destroy(surface);
        }

        [UnityTest]
        public IEnumerator SprayEmitterPaintsHitSurfaceAndLeavesVisibleMark()
        {
            var surface = GameObject.CreatePrimitive(PrimitiveType.Cube);
            surface.transform.position = Vector3.zero;
            surface.transform.localScale = new Vector3(4f, 0.5f, 4f);
            surface.layer = 30;
            var traversalSurface = surface.AddComponent<TraversalSurface>();
            traversalSurface.Configure(SurfaceKind.Float);

            var player = CreatePlayer();
            player.transform.position = new Vector3(0f, 1f, 5f);
            var emitter = player.AddComponent<SprayEmitter>();
            emitter.SetHitMask(1 << surface.layer);
            yield return null;

            var sprayed = emitter.TrySprayFromRay(new Ray(new Vector3(0f, 5f, 0f), Vector3.down));

            Assert.That(sprayed, Is.True);
            Assert.That(emitter.SprayCount, Is.EqualTo(1));
            Assert.That(traversalSurface.Kind, Is.EqualTo(SurfaceKind.Liquid));
            Assert.That(emitter.Grid.TryGetCell(emitter.LastSprayPoint, out var cell), Is.True);
            Assert.That(cell.Kind, Is.EqualTo(SurfaceKind.Liquid));
            Assert.That(player.transform.Find("Spray Mark"), Is.Not.Null);

            Object.Destroy(player);
            Object.Destroy(surface);
        }

        [UnityTest]
        public IEnumerator BounceSurfaceLaunchesPlayer()
        {
            var surface = GameObject.CreatePrimitive(PrimitiveType.Cube);
            surface.transform.position = new Vector3(20f, 0f, 20f);
            surface.transform.localScale = new Vector3(4f, 0.5f, 4f);
            surface.AddComponent<TraversalSurface>().Configure(SurfaceKind.Bounce);

            var player = CreatePlayer();
            yield return null;
            player.transform.position = new Vector3(20f, 1.3f, 20f);
            var motor = player.GetComponent<PlayerMotor>();
            motor.SetSurface(SurfaceKind.Bounce);
            Physics.SyncTransforms();

            var launched = false;
            for (var frame = 0; frame < 20; frame++)
            {
                yield return new WaitForSeconds(0.05f);
                if (motor.Velocity.y > 0f && motor.CurrentState == PlayerMovementState.Airborne)
                {
                    launched = true;
                    break;
                }
            }

            Assert.That(
                motor.CurrentSurface,
                Is.EqualTo(SurfaceKind.Bounce),
                $"Bounce surface was not detected. position={player.transform.position}, state={motor.CurrentState}, velocity={motor.Velocity}");
            Assert.That(
                launched,
                Is.True,
                $"Bounce did not launch. position={player.transform.position}, state={motor.CurrentState}, velocity={motor.Velocity}, surface={motor.CurrentSurface}");

            Object.Destroy(player);
            Object.Destroy(surface);
        }

        [UnityTest]
        public IEnumerator FloatSurfaceUsesSoftGrounding()
        {
            var surface = GameObject.CreatePrimitive(PrimitiveType.Cube);
            surface.transform.position = new Vector3(30f, 0f, 30f);
            surface.transform.localScale = new Vector3(4f, 0.5f, 4f);
            surface.AddComponent<TraversalSurface>().Configure(SurfaceKind.Float);

            var player = CreatePlayer();
            player.transform.position = new Vector3(30f, 1f, 30f);
            var motor = player.GetComponent<PlayerMotor>();

            yield return null;
            yield return null;

            Assert.That(motor.CurrentSurface, Is.EqualTo(SurfaceKind.Float));
            Assert.That(motor.Velocity.y, Is.GreaterThan(-1f));

            Object.Destroy(player);
            Object.Destroy(surface);
        }

        [UnityTest]
        public IEnumerator BloomAbilityTemporarilyConvertsNearbySurface()
        {
            var surface = GameObject.CreatePrimitive(PrimitiveType.Cube);
            surface.transform.position = new Vector3(50f, 0f, 50f);
            surface.transform.localScale = new Vector3(4f, 0.5f, 4f);
            var traversalSurface = surface.AddComponent<TraversalSurface>();
            traversalSurface.Configure(SurfaceKind.Liquid);

            var player = CreatePlayer();
            yield return null;
            player.transform.position = new Vector3(50f, 1f, 50f);
            var ability = player.AddComponent<BloomAbility>();
            Physics.SyncTransforms();

            Assert.That(ability.TryActivate(), Is.True);
            Assert.That(ability.IsActive, Is.True);
            Assert.That(ability.LastAffectedSurfaceCount, Is.EqualTo(1));
            Assert.That(traversalSurface.Kind, Is.EqualTo(SurfaceKind.Bounce));
            Assert.That(player.transform.Find("Bloom Pulse"), Is.Not.Null);

            yield return new WaitForSeconds(1f);

            Assert.That(ability.IsActive, Is.False);
            Assert.That(traversalSurface.Kind, Is.EqualTo(SurfaceKind.Liquid));
            Assert.That(ability.CooldownRemaining, Is.GreaterThan(0f));

            Object.Destroy(player);
            Object.Destroy(surface);
        }

        private static GameObject CreatePlayer()
        {
            var player = new GameObject("Test Player");
            player.transform.position = new Vector3(0f, 1f, 0f);
            player.AddComponent<CharacterController>();
            player.AddComponent<PlayerMotor>();
            return player;
        }
    }
}
