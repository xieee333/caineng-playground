using Caineng.Playground.Core;
using UnityEngine;

namespace Caineng.Playground.Surfaces
{
    public sealed class TraversalSurface : MonoBehaviour
    {
        [SerializeField] private SurfaceKind kind;

        public SurfaceKind Kind => kind;

        public void Configure(SurfaceKind surfaceKind)
        {
            kind = surfaceKind;
        }
    }
}
