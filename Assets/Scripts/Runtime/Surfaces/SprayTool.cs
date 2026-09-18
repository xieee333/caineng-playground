using Caineng.Playground.Core;
using UnityEngine;

namespace Caineng.Playground.Surfaces
{
public sealed class SprayTool
{
    private readonly PaintableSurfaceGrid grid;
    private readonly SurfaceKind surfaceKind;

    public SprayTool(PaintableSurfaceGrid grid, SurfaceKind surfaceKind)
    {
        this.grid = grid;
        this.surfaceKind = surfaceKind;
    }

    public bool Spray(Vector3 worldPoint, float time = 0f)
    {
        return grid.ApplySpray(worldPoint, surfaceKind, time);
    }
}
}
