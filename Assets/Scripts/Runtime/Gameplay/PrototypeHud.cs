using UnityEngine;

namespace Caineng.Playground.Gameplay
{
    public sealed class PrototypeHud : MonoBehaviour
    {
        private GUIStyle style;

        private void OnGUI()
        {
            style ??= new GUIStyle(GUI.skin.label)
            {
                fontSize = 18,
                normal = { textColor = Color.white }
            };

            GUI.Label(
                new Rect(24f, 20f, 720f, 80f),
                "彩能游乐场 · 第一把枪灰盒\nWASD 移动　Space 跳跃",
                style);
        }
    }
}
