using System.Collections;
using Caineng.Playground.Gameplay;
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
        public IEnumerator BubbleRespawnLocksThenReturnsPlayer()
        {
            var player = CreatePlayer();
            var motor = player.GetComponent<PlayerMotor>();
            var respawn = player.AddComponent<BubbleRespawnController>();
            respawn.Configure(new Vector3(2f, 1f, 3f), 0.1f);
            respawn.Bubbleize();
            Assert.That(respawn.IsBubble, Is.True);
            Assert.That(motor.CurrentState, Is.EqualTo(PlayerMovementState.Bubble));
            yield return new WaitForSeconds(0.15f);
            Assert.That(respawn.IsBubble, Is.False);
            Assert.That(player.activeSelf, Is.True);
            Assert.That(Vector3.Distance(player.transform.position, new Vector3(2f, 1f, 3f)), Is.LessThan(0.02f));
            Object.Destroy(player);
        }

        [Test]
        public void BubbleRespawnDefaultRuleIsThreeSeconds()
        {
            var player = CreatePlayer();
            var respawn = player.AddComponent<BubbleRespawnController>();
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
            Assert.That(player.GetComponent<BubbleRespawnController>(), Is.Not.Null);
            Assert.That(GameObject.Find("Ground"), Is.Not.Null);
            Assert.That(GameObject.Find("BouncePad"), Is.Not.Null);
            Assert.That(GameObject.Find("Graybox Arena"), Is.Not.Null);
            Assert.That(GameObject.Find("Yellow Ramp"), Is.Not.Null);
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
