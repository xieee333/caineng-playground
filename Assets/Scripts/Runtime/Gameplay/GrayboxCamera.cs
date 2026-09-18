using UnityEngine;

namespace Caineng.Playground.Gameplay
{
    public sealed class GrayboxCamera : MonoBehaviour
    {
        [SerializeField] private Vector3 offset = new Vector3(0f, 9f, -11f);
        [SerializeField] private Vector3 lookOffset = new Vector3(0f, 1f, 0f);
        [SerializeField] private float followSharpness = 8f;

        private Transform target;

        private void Start()
        {
            var player = GameObject.Find("Player");
            target = player != null ? player.transform : null;
        }

        private void LateUpdate()
        {
            if (target == null)
            {
                return;
            }

            var desiredPosition = target.position + offset;
            var blend = 1f - Mathf.Exp(-followSharpness * Time.deltaTime);
            transform.position = Vector3.Lerp(transform.position, desiredPosition, blend);
            transform.LookAt(target.position + lookOffset);
        }
    }
}
