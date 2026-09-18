using Caineng.Playground.Core;
using UnityEngine;

namespace Caineng.Playground.Surfaces
{
public sealed class PaintableSurfaceGrid
{
    private readonly SurfaceCell[] cells;
    private readonly Vector3 origin;

    public int Width { get; }
    public int Depth { get; }
    public float CellSize { get; }

    public PaintableSurfaceGrid(int width, int depth, float cellSize, Vector3 origin)
    {
        if (width <= 0) throw new System.ArgumentOutOfRangeException(nameof(width));
        if (depth <= 0) throw new System.ArgumentOutOfRangeException(nameof(depth));
        if (cellSize <= 0f) throw new System.ArgumentOutOfRangeException(nameof(cellSize));

        Width = width;
        Depth = depth;
        CellSize = cellSize;
        this.origin = origin;
        cells = new SurfaceCell[width * depth];
    }

    public bool ApplySpray(Vector3 worldPoint, SurfaceKind kind, float modifiedTime = 0f)
    {
        if (!TryWorldToCell(worldPoint, out var x, out var z)) return false;

        var index = ToIndex(x, z);
        cells[index].Set(kind, modifiedTime);
        return true;
    }

    public SurfaceCell GetCell(int x, int z)
    {
        if (!IsInside(x, z)) return default;
        return cells[ToIndex(x, z)];
    }

    public bool TryGetCell(Vector3 worldPoint, out SurfaceCell cell)
    {
        if (!TryWorldToCell(worldPoint, out var x, out var z))
        {
            cell = default;
            return false;
        }

        cell = cells[ToIndex(x, z)];
        return true;
    }

    private bool TryWorldToCell(Vector3 worldPoint, out int x, out int z)
    {
        var local = worldPoint - origin;
        x = Mathf.FloorToInt(local.x / CellSize);
        z = Mathf.FloorToInt(local.z / CellSize);
        return IsInside(x, z);
    }

    private bool IsInside(int x, int z) => x >= 0 && x < Width && z >= 0 && z < Depth;

    private int ToIndex(int x, int z) => z * Width + x;
}
}
