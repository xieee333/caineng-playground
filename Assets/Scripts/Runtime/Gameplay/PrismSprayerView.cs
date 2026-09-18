using Caineng.Playground.Surfaces;
using UnityEngine;

namespace Caineng.Playground.Gameplay
{
    public sealed class PrismSprayerView : MonoBehaviour
    {
        [SerializeField] private Camera viewCamera;
        [SerializeField] private Vector3 localPosition = new Vector3(0.48f, -0.32f, 0.75f);

        private SprayEmitter sprayEmitter;
        private Transform weaponRoot;
        private Vector3 basePosition;
        private float recoil;
        private int lastSprayCount;

        private void Awake()
        {
            sprayEmitter = GetComponent<SprayEmitter>();
            viewCamera = viewCamera != null ? viewCamera : Camera.main;
            if (viewCamera == null) return;

            CreateWeaponView();
        }

        private void Update()
        {
            if (weaponRoot == null) return;

            if (sprayEmitter != null && sprayEmitter.SprayCount > lastSprayCount)
            {
                lastSprayCount = sprayEmitter.SprayCount;
                recoil = 1f;
            }

            recoil = Mathf.MoveTowards(recoil, 0f, Time.deltaTime * 8f);
            weaponRoot.localPosition = basePosition + Vector3.back * (recoil * 0.06f);
            weaponRoot.localRotation = Quaternion.Euler(-recoil * 8f, Mathf.Sin(Time.time * 1.5f) * 1.5f, 0f);
        }

        private void CreateWeaponView()
        {
            var root = new GameObject("Prism Sprayer View");
            weaponRoot = root.transform;
            weaponRoot.SetParent(viewCamera.transform, false);
            weaponRoot.localPosition = localPosition;
            basePosition = localPosition;

            CreatePart("Prism Core", new Vector3(0f, 0f, 0.3f), new Vector3(0.26f, 0.22f, 0.62f), new Color(0.1f, 0.85f, 1f));
            CreatePart("Prism Magenta Rail", new Vector3(-0.16f, 0.02f, 0.26f), new Vector3(0.08f, 0.12f, 0.7f), new Color(1f, 0.15f, 0.65f));
            CreatePart("Prism Yellow Rail", new Vector3(0.16f, -0.02f, 0.26f), new Vector3(0.08f, 0.12f, 0.7f), new Color(1f, 0.75f, 0.1f));
            CreatePart("Prism Grip", new Vector3(0f, -0.22f, 0.05f), new Vector3(0.18f, 0.36f, 0.2f), new Color(0.12f, 0.15f, 0.25f));
        }

        private void CreatePart(string partName, Vector3 position, Vector3 scale, Color color)
        {
            var part = GameObject.CreatePrimitive(PrimitiveType.Cube);
            part.name = partName;
            part.transform.SetParent(weaponRoot, false);
            part.transform.localPosition = position;
            part.transform.localScale = scale;

            var collider = part.GetComponent<Collider>();
            if (collider != null) Destroy(collider);

            var renderer = part.GetComponent<Renderer>();
            var shader = Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Standard");
            if (renderer != null && shader != null) renderer.material = new Material(shader) { color = color };
        }

        private void OnDestroy()
        {
            if (weaponRoot != null) Destroy(weaponRoot.gameObject);
        }
    }
}
