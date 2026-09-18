using Caineng.Playground.Surfaces;
using UnityEngine;

namespace Caineng.Playground.Gameplay
{
    public sealed class PrototypeHud : MonoBehaviour
    {
        private GUIStyle style;
        private GUIStyle detailStyle;
        private PlayerMotor player;
        private RespawnController respawn;
        private SprayEmitter spray;
        private BloomAbility bloom;

        private void Update()
        {
            if (player != null) return;

            player = GameObject.FindWithTag("Player")?.GetComponent<PlayerMotor>();
            if (player == null) return;

            respawn = player.GetComponent<RespawnController>();
            spray = player.GetComponent<SprayEmitter>();
            bloom = player.GetComponent<BloomAbility>();
        }

        private void OnGUI()
        {
            style ??= new GUIStyle(GUI.skin.label)
            {
                fontSize = 18,
                normal = { textColor = Color.white }
            };

            detailStyle ??= new GUIStyle(GUI.skin.label)
            {
                fontSize = 15,
                normal = { textColor = new Color(0.86f, 0.94f, 1f) }
            };

            GUI.Box(new Rect(18f, 14f, 720f, 86f), GUIContent.none);
            GUI.Label(
                new Rect(32f, 24f, 700f, 70f),
                "彩能游乐场 · 彩能折返灰盒\nWASD 移动　Space 跳跃　鼠标左键喷涂　跌落后 3 秒彩能折返",
                style);

            GUI.Box(new Rect(18f, 110f, 500f, 150f), GUIContent.none);
            var playerLine = player == null
                ? "玩家：等待场景加载"
                : $"状态：{player.CurrentState}    表面：{player.CurrentSurface}    速度：{player.Velocity.magnitude:0.00}";
            var respawnLine = respawn == null
                ? "折返：未连接"
                : respawn.IsRespawning
                    ? $"折返：进行中    剩余：{respawn.RespawnRemaining:0.0}s"
                    : "折返：就绪";
            var sprayLine = spray == null
                ? "喷涂：未连接"
                : spray.HasSprayed
                    ? $"喷涂：{spray.SprayCount} 次    最近命中：{spray.LastSprayPoint}"
                    : "喷涂：0 次    等待鼠标左键";
            var bloomLine = bloom == null
                ? "盛放：未连接"
                : bloom.IsActive
                    ? $"盛放：脉冲中    影响表面：{bloom.LastAffectedSurfaceCount}"
                    : $"盛放：冷却 {bloom.CooldownRemaining:0.0}s    按 Q 释放";
            GUI.Label(new Rect(32f, 120f, 470f, 150f), $"LIVE DIAGNOSTICS\n{playerLine}\n{respawnLine}\n{sprayLine}\n{bloomLine}", detailStyle);
        }
    }
}
