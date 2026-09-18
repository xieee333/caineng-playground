using Caineng.Playground.Core;

namespace Caineng.Playground.Surfaces
{
public struct SurfaceCell
{
    public SurfaceKind Kind { get; private set; }
    public float LastModifiedTime { get; private set; }

    public SurfaceCell(SurfaceKind kind, float lastModifiedTime)
    {
        Kind = kind;
        LastModifiedTime = lastModifiedTime;
    }

    public void Set(SurfaceKind kind, float modifiedTime)
    {
        Kind = kind;
        LastModifiedTime = modifiedTime;
    }
}
}
